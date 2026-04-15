// Copyright Woogle. All Rights Reserved.

#include "WxBlueprintSnapshotExporter.h"
#include "WxBlueprintSnapshotSettings.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Components/ActorComponent.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Components/PanelWidget.h"
#include "Components/PanelSlot.h"
#include "MVVMWidgetBlueprintExtension_View.h"
#include "MVVMBlueprintView.h"
#include "MVVMBlueprintViewModelContext.h"
#include "MVVMBlueprintViewBinding.h"
#include "MVVMBlueprintViewConversionFunction.h"
#include "MVVMPropertyPath.h"
#include "Types/MVVMBindingMode.h"
#include "UObject/UnrealType.h"
#include "UObject/Class.h"
#include "UObject/Package.h"
#include "UObject/TextProperty.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "Misc/SecureHash.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "UObject/StructOnScope.h"
#include "Interfaces/IPluginManager.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "K2Node.h"
#include "K2Node_Event.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_Knot.h"
#include "K2Node_Self.h"
#include "K2Node_Literal.h"
#include "K2Node_DynamicCast.h"

namespace WxBlueprintSnapshotPrivate
{
	void SortJsonObjectRecursive(TSharedPtr<FJsonObject> Obj)
	{
		if (!Obj.IsValid())
		{
			return;
		}
		Obj->Values.KeySort(TLess<FString>());
		for (auto& Pair : Obj->Values)
		{
			if (Pair.Value.IsValid())
			{
				if (Pair.Value->Type == EJson::Object)
				{
					SortJsonObjectRecursive(Pair.Value->AsObject());
				}
				else if (Pair.Value->Type == EJson::Array)
				{
					const TArray<TSharedPtr<FJsonValue>>& Arr = Pair.Value->AsArray();
					for (const TSharedPtr<FJsonValue>& Elem : Arr)
					{
						if (Elem.IsValid() && Elem->Type == EJson::Object)
						{
							SortJsonObjectRecursive(Elem->AsObject());
						}
					}
				}
			}
		}
	}

	bool IsSkippableProperty(const FProperty* Property)
	{
		if (!Property)
		{
			return true;
		}
		return Property->HasAnyPropertyFlags(
			CPF_Transient
			| CPF_DuplicateTransient
			| CPF_NonPIEDuplicateTransient
			| CPF_Deprecated
			| CPF_EditorOnly);
	}

	void SetObjectFieldIfNonEmpty(FJsonObject& Root, const FString& Key, const TSharedPtr<FJsonObject>& Value)
	{
		if (Value.IsValid() && Value->Values.Num() > 0)
		{
			Root.SetObjectField(Key, Value.ToSharedRef());
		}
	}

	FString ExportPropertyValue(const FProperty* Property, const void* ValuePtr, const UObject* Parent)
	{
		FString Out;
		Property->ExportText_Direct(Out, ValuePtr, ValuePtr, const_cast<UObject*>(Parent), PPF_SimpleObjectText);
		return Out;
	}

	FString SanitizeForPath(const FString& In)
	{
		FString Out = In;
		Out.ReplaceInline(TEXT(":"), TEXT("_"));
		return Out;
	}

	TSharedPtr<FJsonValue> PropertyValueToJson(const FProperty* Property, const void* ValuePtr, const void* DefaultPtr, const UObject* Owner);

	TSharedPtr<FStructOnScope> MakeElementDefault(const FProperty* ElemProp)
	{
		if (const FStructProperty* ElemStruct = CastField<FStructProperty>(ElemProp))
		{
			return MakeShared<FStructOnScope>(ElemStruct->Struct);
		}
		return nullptr;
	}

	void StructToJsonObject(const UScriptStruct* Struct, const void* StructPtr, const void* DefaultStructPtr, const UObject* Owner, TSharedPtr<FJsonObject> Out)
	{
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			FProperty* Inner = *It;
			if (IsSkippableProperty(Inner))
			{
				continue;
			}
			const void* InnerPtr = Inner->ContainerPtrToValuePtr<void>(StructPtr);
			const void* InnerDefaultPtr = DefaultStructPtr ? Inner->ContainerPtrToValuePtr<void>(DefaultStructPtr) : nullptr;

			if (InnerDefaultPtr && Inner->Identical(InnerPtr, InnerDefaultPtr, PPF_DeepComparison | PPF_DeepCompareInstances))
			{
				continue;
			}

			TSharedPtr<FJsonValue> Value = PropertyValueToJson(Inner, InnerPtr, InnerDefaultPtr, Owner);
			// 모든 하위 필드가 기본값과 동일해 빈 오브젝트가 된 struct는 드롭한다.
			if (Value.IsValid() && Value->Type == EJson::Object && Value->AsObject()->Values.Num() == 0)
			{
				continue;
			}
			Out->SetField(Inner->GetName(), Value);
		}
	}

	TSharedPtr<FJsonValue> PropertyValueToJson(const FProperty* Property, const void* ValuePtr, const void* DefaultPtr, const UObject* Owner)
	{
		if (!Property || !ValuePtr)
		{
			return MakeShared<FJsonValueNull>();
		}

		if (const FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
		{
			return MakeShared<FJsonValueBoolean>(BoolProp->GetPropertyValue(ValuePtr));
		}
		if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
		{
			const int64 Value = EnumProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr);
			return MakeShared<FJsonValueString>(EnumProp->GetEnum()->GetNameStringByValue(Value));
		}
		if (const FByteProperty* ByteProp = CastField<FByteProperty>(Property))
		{
			if (ByteProp->Enum)
			{
				return MakeShared<FJsonValueString>(ByteProp->Enum->GetNameStringByValue(ByteProp->GetPropertyValue(ValuePtr)));
			}
			return MakeShared<FJsonValueNumber>(static_cast<double>(ByteProp->GetPropertyValue(ValuePtr)));
		}
		if (const FNumericProperty* NumProp = CastField<FNumericProperty>(Property))
		{
			if (NumProp->IsFloatingPoint())
			{
				return MakeShared<FJsonValueNumber>(NumProp->GetFloatingPointPropertyValue(ValuePtr));
			}
			return MakeShared<FJsonValueNumber>(static_cast<double>(NumProp->GetSignedIntPropertyValue(ValuePtr)));
		}
		if (const FStrProperty* StrProp = CastField<FStrProperty>(Property))
		{
			return MakeShared<FJsonValueString>(StrProp->GetPropertyValue(ValuePtr));
		}
		if (const FNameProperty* NameProp = CastField<FNameProperty>(Property))
		{
			return MakeShared<FJsonValueString>(NameProp->GetPropertyValue(ValuePtr).ToString());
		}
		if (const FTextProperty* TextProp = CastField<FTextProperty>(Property))
		{
			return MakeShared<FJsonValueString>(TextProp->GetPropertyValue(ValuePtr).ToString());
		}
		if (const FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(Property))
		{
			return MakeShared<FJsonValueString>(SoftProp->GetPropertyValue(ValuePtr).ToString());
		}
		if (const FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(Property))
		{
			UObject* Obj = ObjProp->GetObjectPropertyValue(ValuePtr);
			const bool bInstanced = Property->HasAnyPropertyFlags(CPF_InstancedReference | CPF_PersistentInstance)
				|| (ObjProp->PropertyClass && ObjProp->PropertyClass->HasAnyClassFlags(CLASS_DefaultToInstanced | CLASS_EditInlineNew));
			if (bInstanced && Obj)
			{
				TSharedPtr<FJsonObject> InnerJson = MakeShared<FJsonObject>();
				InnerJson->SetStringField(TEXT("class"), Obj->GetClass()->GetPathName());

				UObject* DefaultObj = nullptr;
				if (DefaultPtr)
				{
					DefaultObj = ObjProp->GetObjectPropertyValue(DefaultPtr);
				}
				const UObject* DefaultsForDelta = (DefaultObj && DefaultObj->GetClass() == Obj->GetClass())
					? DefaultObj
					: Obj->GetClass()->GetDefaultObject(false);

				TSharedPtr<FJsonObject> SubDelta = FWxBlueprintSnapshotExporter::BuildClassDefaults(Obj, DefaultsForDelta);
				if (SubDelta.IsValid() && SubDelta->Values.Num() > 0)
				{
					InnerJson->SetObjectField(TEXT("delta"), SubDelta.ToSharedRef());
				}
				return MakeShared<FJsonValueObject>(InnerJson);
			}
			return MakeShared<FJsonValueString>(Obj ? Obj->GetPathName() : FString());
		}
		if (const FInterfaceProperty* IfaceProp = CastField<FInterfaceProperty>(Property))
		{
			const FScriptInterface& Iface = IfaceProp->GetPropertyValue(ValuePtr);
			UObject* Obj = Iface.GetObject();
			return MakeShared<FJsonValueString>(Obj ? Obj->GetPathName() : FString());
		}
		if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
		{
			TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
			StructToJsonObject(StructProp->Struct, ValuePtr, DefaultPtr, Owner, Obj);
			return MakeShared<FJsonValueObject>(Obj);
		}
		if (const FArrayProperty* ArrProp = CastField<FArrayProperty>(Property))
		{
			FScriptArrayHelper Helper(ArrProp, ValuePtr);
			TSharedPtr<FStructOnScope> ElemDefault = MakeElementDefault(ArrProp->Inner);
			const void* ElemDefaultMem = ElemDefault.IsValid() ? ElemDefault->GetStructMemory() : nullptr;
			TArray<TSharedPtr<FJsonValue>> Arr;
			for (int32 i = 0; i < Helper.Num(); ++i)
			{
				Arr.Add(PropertyValueToJson(ArrProp->Inner, Helper.GetRawPtr(i), ElemDefaultMem, Owner));
			}
			return MakeShared<FJsonValueArray>(Arr);
		}
		if (const FSetProperty* SetProp = CastField<FSetProperty>(Property))
		{
			FScriptSetHelper Helper(SetProp, ValuePtr);
			TSharedPtr<FStructOnScope> ElemDefault = MakeElementDefault(SetProp->ElementProp);
			const void* ElemDefaultMem = ElemDefault.IsValid() ? ElemDefault->GetStructMemory() : nullptr;
			TArray<TSharedPtr<FJsonValue>> Arr;
			for (int32 i = 0; i < Helper.GetMaxIndex(); ++i)
			{
				if (!Helper.IsValidIndex(i))
				{
					continue;
				}
				Arr.Add(PropertyValueToJson(SetProp->ElementProp, Helper.GetElementPtr(i), ElemDefaultMem, Owner));
			}
			return MakeShared<FJsonValueArray>(Arr);
		}
		if (const FMapProperty* MapProp = CastField<FMapProperty>(Property))
		{
			FScriptMapHelper Helper(MapProp, ValuePtr);
			const FProperty* KeyProp = MapProp->KeyProp;
			TSharedPtr<FStructOnScope> ValueDefault = MakeElementDefault(MapProp->ValueProp);
			const void* ValueDefaultMem = ValueDefault.IsValid() ? ValueDefault->GetStructMemory() : nullptr;
			const bool bStringKey = KeyProp->IsA<FStrProperty>() || KeyProp->IsA<FNameProperty>() || KeyProp->IsA<FTextProperty>();
			if (bStringKey)
			{
				TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
				for (int32 i = 0; i < Helper.GetMaxIndex(); ++i)
				{
					if (!Helper.IsValidIndex(i))
					{
						continue;
					}
					const TSharedPtr<FJsonValue> KeyVal = PropertyValueToJson(KeyProp, Helper.GetKeyPtr(i), nullptr, Owner);
					Obj->SetField(KeyVal->AsString(), PropertyValueToJson(MapProp->ValueProp, Helper.GetValuePtr(i), ValueDefaultMem, Owner));
				}
				return MakeShared<FJsonValueObject>(Obj);
			}

			TSharedPtr<FStructOnScope> KeyDefault = MakeElementDefault(KeyProp);
			const void* KeyDefaultMem = KeyDefault.IsValid() ? KeyDefault->GetStructMemory() : nullptr;
			TArray<TSharedPtr<FJsonValue>> Arr;
			for (int32 i = 0; i < Helper.GetMaxIndex(); ++i)
			{
				if (!Helper.IsValidIndex(i))
				{
					continue;
				}
				TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
				Entry->SetField(TEXT("key"), PropertyValueToJson(KeyProp, Helper.GetKeyPtr(i), KeyDefaultMem, Owner));
				Entry->SetField(TEXT("value"), PropertyValueToJson(MapProp->ValueProp, Helper.GetValuePtr(i), ValueDefaultMem, Owner));
				Arr.Add(MakeShared<FJsonValueObject>(Entry));
			}
			return MakeShared<FJsonValueArray>(Arr);
		}

		FString Out;
		Property->ExportText_Direct(Out, ValuePtr, ValuePtr, const_cast<UObject*>(Owner), PPF_SimpleObjectText);
		return MakeShared<FJsonValueString>(Out);
	}

	// ===== Pseudo-code 그래프 렌더러 =====

	struct FPseudoLine
	{
		int32 Indent = 0;
		FString Text;
	};

	struct FPseudoEntry
	{
		FString Header;
		TArray<FPseudoLine> Body;
	};

	struct FRenderCtx
	{
		TArray<FPseudoLine>& Body;
		TSet<UEdGraphNode*>& Visited;
	};

	static UEdGraphPin* SkipKnots(UEdGraphPin* Pin);
	static FString RenderExpression(UEdGraphPin* OutputPin);
	static FString RenderDataInput(UEdGraphPin* InputPin);
	static void RenderExecChain(UEdGraphPin* ExecOutPin, int32 Indent, FRenderCtx& Ctx);
	static void RenderExecNode(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx);

	void Emit(FRenderCtx& Ctx, int32 Indent, FString Text)
	{
		Ctx.Body.Add({ Indent, MoveTemp(Text) });
	}

	bool IsExecPin(const UEdGraphPin* Pin)
	{
		return Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec;
	}

	UEdGraphPin* FindThenPin(UEdGraphNode* Node)
	{
		if (!Node)
		{
			return nullptr;
		}
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Output && IsExecPin(Pin))
			{
				return Pin;
			}
		}
		return nullptr;
	}

	UEdGraphPin* SkipKnots(UEdGraphPin* Pin)
	{
		if (!Pin)
		{
			return nullptr;
		}
		while (Pin && Cast<UK2Node_Knot>(Pin->GetOwningNode()))
		{
			UK2Node_Knot* Knot = Cast<UK2Node_Knot>(Pin->GetOwningNode());
			UEdGraphPin* Upstream = Pin->Direction == EGPD_Input ? Knot->GetInputPin() : Knot->GetOutputPin();
			if (!Upstream || Upstream->LinkedTo.Num() == 0)
			{
				return nullptr;
			}
			Pin = Upstream->LinkedTo[0];
		}
		return Pin;
	}

	FString FormatLiteralPin(const UEdGraphPin* Pin)
	{
		if (!Pin)
		{
			return TEXT("?");
		}
		if (Pin->DefaultObject)
		{
			return Pin->DefaultObject->GetName();
		}
		const FString Val = Pin->DefaultValue.IsEmpty() ? Pin->AutogeneratedDefaultValue : Pin->DefaultValue;
		if (Val.IsEmpty())
		{
			if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object
				|| Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class
				|| Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Interface
				|| Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_SoftObject
				|| Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_SoftClass)
			{
				return TEXT("None");
			}
			return TEXT("");
		}
		if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_String
			|| Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Text
			|| Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Name)
		{
			return FString::Printf(TEXT("\"%s\""), *Val);
		}
		return Val;
	}

	FString RenderDataInput(UEdGraphPin* InputPin)
	{
		if (!InputPin)
		{
			return TEXT("?");
		}
		if (InputPin->LinkedTo.Num() == 0)
		{
			return FormatLiteralPin(InputPin);
		}
		UEdGraphPin* Src = SkipKnots(InputPin->LinkedTo[0]);
		if (!Src)
		{
			return FormatLiteralPin(InputPin);
		}
		return RenderExpression(Src);
	}

	FString RenderCallArgs(UK2Node_CallFunction* Call)
	{
		TArray<FString> Args;
		for (UEdGraphPin* Pin : Call->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Input || IsExecPin(Pin))
			{
				continue;
			}
			if (Pin->PinName == UEdGraphSchema_K2::PN_Self)
			{
				continue;
			}
			// 숨김 핀(WorldContextObject 등)은 사용자가 건드리지 않으면 스킵
			if (Pin->bHidden && Pin->LinkedTo.Num() == 0)
			{
				continue;
			}
			// 연결도 없고 기본값이 autogenerated 그대로면 스킵 (Duration, Key, bPrintToScreen 등)
			if (Pin->LinkedTo.Num() == 0 && Pin->DefaultValue == Pin->AutogeneratedDefaultValue)
			{
				continue;
			}
			Args.Add(FString::Printf(TEXT("%s=%s"), *Pin->PinName.ToString(), *RenderDataInput(Pin)));
		}
		return FString::Join(Args, TEXT(", "));
	}

	FString RenderCallTarget(UK2Node_CallFunction* Call)
	{
		UEdGraphPin* SelfPin = Call->FindPin(UEdGraphSchema_K2::PN_Self);
		if (SelfPin && SelfPin->LinkedTo.Num() > 0)
		{
			return RenderDataInput(SelfPin) + TEXT(".");
		}
		if (Call->FunctionReference.IsSelfContext())
		{
			return FString();
		}
		if (UClass* MemberClass = Call->FunctionReference.GetMemberParentClass())
		{
			return MemberClass->GetName() + TEXT("::");
		}
		return FString();
	}

	FString RenderExpression(UEdGraphPin* OutputPin)
	{
		if (!OutputPin)
		{
			return TEXT("?");
		}
		UEdGraphNode* Node = OutputPin->GetOwningNode();
		if (!Node)
		{
			return TEXT("?");
		}

		if (Cast<UK2Node_Self>(Node))
		{
			return TEXT("Self");
		}
		if (UK2Node_VariableGet* VG = Cast<UK2Node_VariableGet>(Node))
		{
			return VG->GetVarNameString();
		}
		if (UK2Node_Literal* Lit = Cast<UK2Node_Literal>(Node))
		{
			if (UObject* Obj = Lit->GetObjectRef())
			{
				return Obj->GetName();
			}
			return Lit->GetNodeTitle(ENodeTitleType::ListView).ToString();
		}
		if (UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node))
		{
			const FString Target = RenderCallTarget(Call);
			const FString FnName = Call->GetFunctionName().ToString();
			const FString Args = RenderCallArgs(Call);
			FString Expr = FString::Printf(TEXT("%s%s(%s)"), *Target, *FnName, *Args);
			if (OutputPin->PinName != UEdGraphSchema_K2::PN_ReturnValue && !OutputPin->PinName.IsNone())
			{
				Expr += FString::Printf(TEXT(".%s"), *OutputPin->PinName.ToString());
			}
			return Expr;
		}
		if (UK2Node_DynamicCast* DCast = Cast<UK2Node_DynamicCast>(Node))
		{
			UEdGraphPin* ObjIn = DCast->GetCastSourcePin();
			const FString TypeName = DCast->TargetType ? DCast->TargetType->GetName() : TEXT("?");
			return FString::Printf(TEXT("Cast<%s>(%s)"), *TypeName, *RenderDataInput(ObjIn));
		}

		// 일반 K2Node fallback: NodeTitle + 필요시 출력 핀명
		FString Title = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
		Title.ReplaceInline(TEXT("\n"), TEXT(" "));
		if (!OutputPin->PinName.IsNone() && OutputPin->PinName != UEdGraphSchema_K2::PN_ReturnValue)
		{
			return FString::Printf(TEXT("%s.%s"), *Title, *OutputPin->PinName.ToString());
		}
		return Title;
	}

	void RenderExecChain(UEdGraphPin* ExecOutPin, int32 Indent, FRenderCtx& Ctx)
	{
		if (!ExecOutPin || ExecOutPin->LinkedTo.Num() == 0)
		{
			return;
		}
		UEdGraphPin* NextPin = SkipKnots(ExecOutPin->LinkedTo[0]);
		if (!NextPin)
		{
			return;
		}
		UEdGraphNode* NextNode = NextPin->GetOwningNode();
		if (!NextNode)
		{
			return;
		}
		if (Ctx.Visited.Contains(NextNode))
		{
			Emit(Ctx, Indent, FString::Printf(TEXT("goto %s"), *NextNode->GetName()));
			return;
		}
		Ctx.Visited.Add(NextNode);
		RenderExecNode(NextNode, Indent, Ctx);
	}

	bool TryRenderBranch(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		UK2Node_IfThenElse* Branch = Cast<UK2Node_IfThenElse>(Node);
		if (!Branch)
		{
			return false;
		}
		Emit(Ctx, Indent, FString::Printf(TEXT("if (%s):"), *RenderDataInput(Branch->GetConditionPin())));
		RenderExecChain(Branch->GetThenPin(), Indent + 1, Ctx);
		UEdGraphPin* ElsePin = Branch->GetElsePin();
		if (ElsePin && ElsePin->LinkedTo.Num() > 0)
		{
			Emit(Ctx, Indent, TEXT("else:"));
			RenderExecChain(ElsePin, Indent + 1, Ctx);
		}
		return true;
	}

	bool TryRenderSequence(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		UK2Node_ExecutionSequence* Seq = Cast<UK2Node_ExecutionSequence>(Node);
		if (!Seq)
		{
			return false;
		}
		int32 Idx = 0;
		for (UEdGraphPin* Pin : Seq->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Output && IsExecPin(Pin))
			{
				Emit(Ctx, Indent, FString::Printf(TEXT("# sequence[%d]"), Idx++));
				RenderExecChain(Pin, Indent, Ctx);
			}
		}
		return true;
	}

	bool TryRenderVariableSet(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		UK2Node_VariableSet* VS = Cast<UK2Node_VariableSet>(Node);
		if (!VS)
		{
			return false;
		}
		UEdGraphPin* ValPin = VS->FindPin(VS->GetVarName());
		Emit(Ctx, Indent, FString::Printf(TEXT("%s = %s"), *VS->GetVarNameString(), *RenderDataInput(ValPin)));
		RenderExecChain(FindThenPin(VS), Indent, Ctx);
		return true;
	}

	bool TryRenderCallFunction(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node);
		if (!Call)
		{
			return false;
		}
		Emit(Ctx, Indent, FString::Printf(TEXT("%s%s(%s)"),
			*RenderCallTarget(Call), *Call->GetFunctionName().ToString(), *RenderCallArgs(Call)));
		RenderExecChain(FindThenPin(Call), Indent, Ctx);
		return true;
	}

	bool TryRenderDynamicCast(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		UK2Node_DynamicCast* DCast = Cast<UK2Node_DynamicCast>(Node);
		if (!DCast)
		{
			return false;
		}
		const FString TypeName = DCast->TargetType ? DCast->TargetType->GetName() : TEXT("?");
		Emit(Ctx, Indent, FString::Printf(TEXT("As%s = Cast<%s>(%s)"), *TypeName, *TypeName, *RenderDataInput(DCast->GetCastSourcePin())));
		UEdGraphPin* Success = DCast->GetValidCastPin();
		UEdGraphPin* Failed = DCast->GetInvalidCastPin();
		if (Success && Success->LinkedTo.Num() > 0)
		{
			Emit(Ctx, Indent, FString::Printf(TEXT("if (As%s):"), *TypeName));
			RenderExecChain(Success, Indent + 1, Ctx);
		}
		if (Failed && Failed->LinkedTo.Num() > 0)
		{
			Emit(Ctx, Indent, TEXT("else:"));
			RenderExecChain(Failed, Indent + 1, Ctx);
		}
		return true;
	}

	bool TryRenderMacro(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		UK2Node_MacroInstance* Macro = Cast<UK2Node_MacroInstance>(Node);
		if (!Macro)
		{
			return false;
		}
		UEdGraph* MacroGraph = Macro->GetMacroGraph();
		const FString MacroName = MacroGraph ? MacroGraph->GetName() : TEXT("Macro");

		TArray<FString> Args;
		for (UEdGraphPin* Pin : Macro->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Input && !IsExecPin(Pin))
			{
				Args.Add(FString::Printf(TEXT("%s=%s"), *Pin->PinName.ToString(), *RenderDataInput(Pin)));
			}
		}

		UEdGraphPin* LoopBody = Macro->FindPin(TEXT("LoopBody"));
		UEdGraphPin* Completed = Macro->FindPin(TEXT("Completed"));
		if (LoopBody || Completed)
		{
			Emit(Ctx, Indent, FString::Printf(TEXT("%s(%s):"), *MacroName, *FString::Join(Args, TEXT(", "))));
			if (LoopBody)
			{
				RenderExecChain(LoopBody, Indent + 1, Ctx);
			}
			if (Completed)
			{
				RenderExecChain(Completed, Indent, Ctx);
			}
			return true;
		}

		Emit(Ctx, Indent, FString::Printf(TEXT("%s(%s)"), *MacroName, *FString::Join(Args, TEXT(", "))));
		RenderExecChain(FindThenPin(Macro), Indent, Ctx);
		return true;
	}

	bool TryRenderReturn(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		UK2Node_FunctionResult* Ret = Cast<UK2Node_FunctionResult>(Node);
		if (!Ret)
		{
			return false;
		}
		TArray<FString> Returns;
		for (UEdGraphPin* Pin : Ret->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Input && !IsExecPin(Pin))
			{
				Returns.Add(FString::Printf(TEXT("%s=%s"), *Pin->PinName.ToString(), *RenderDataInput(Pin)));
			}
		}
		Emit(Ctx, Indent, FString::Printf(TEXT("return %s"), *FString::Join(Returns, TEXT(", "))));
		return true;
	}

	void RenderFallbackNode(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		FString Title = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
		Title.ReplaceInline(TEXT("\n"), TEXT(" "));
		Emit(Ctx, Indent, Title);
		RenderExecChain(FindThenPin(Node), Indent, Ctx);
	}

	void RenderExecNode(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		if (TryRenderBranch(Node, Indent, Ctx)) return;
		if (TryRenderSequence(Node, Indent, Ctx)) return;
		if (TryRenderVariableSet(Node, Indent, Ctx)) return;
		if (TryRenderCallFunction(Node, Indent, Ctx)) return;
		if (TryRenderDynamicCast(Node, Indent, Ctx)) return;
		if (TryRenderMacro(Node, Indent, Ctx)) return;
		if (TryRenderReturn(Node, Indent, Ctx)) return;
		RenderFallbackNode(Node, Indent, Ctx);
	}

	bool BuildEntryHeader(UEdGraphNode* Entry, FString& OutHeader)
	{
		if (UK2Node_CustomEvent* Ce = Cast<UK2Node_CustomEvent>(Entry))
		{
			OutHeader = FString::Printf(TEXT("custom_event %s:"), *Ce->CustomFunctionName.ToString());
			return true;
		}
		if (UK2Node_Event* Ev = Cast<UK2Node_Event>(Entry))
		{
			OutHeader = FString::Printf(TEXT("event %s:"), *Ev->EventReference.GetMemberName().ToString());
			return true;
		}
		if (UK2Node_FunctionEntry* Fe = Cast<UK2Node_FunctionEntry>(Entry))
		{
			TArray<FString> Params;
			for (UEdGraphPin* Pin : Fe->Pins)
			{
				if (Pin && Pin->Direction == EGPD_Output && !IsExecPin(Pin))
				{
					Params.Add(Pin->PinName.ToString());
				}
			}
			const FName FnName = Fe->CustomGeneratedFunctionName.IsNone()
				? Fe->FunctionReference.GetMemberName()
				: Fe->CustomGeneratedFunctionName;
			OutHeader = FString::Printf(TEXT("function %s(%s):"), *FnName.ToString(), *FString::Join(Params, TEXT(", ")));
			return true;
		}
		return false;
	}

	bool RenderEntryNode(UEdGraphNode* Entry, FPseudoEntry& OutEntry)
	{
		if (!BuildEntryHeader(Entry, OutEntry.Header))
		{
			return false;
		}
		TSet<UEdGraphNode*> Visited;
		Visited.Add(Entry);
		FRenderCtx Ctx{ OutEntry.Body, Visited };
		RenderExecChain(FindThenPin(Entry), 1, Ctx);
		return true;
	}

	TArray<FPseudoEntry> RenderGraphPseudoCode(UEdGraph* Graph)
	{
		TArray<FPseudoEntry> Entries;
		if (!Graph)
		{
			return Entries;
		}

		TArray<UEdGraphNode*> EntryNodes;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (Node && (Cast<UK2Node_Event>(Node) || Cast<UK2Node_CustomEvent>(Node) || Cast<UK2Node_FunctionEntry>(Node)))
			{
				EntryNodes.Add(Node);
			}
		}

		EntryNodes.Sort([](UEdGraphNode& A, UEdGraphNode& B)
		{
			return A.GetNodeTitle(ENodeTitleType::ListView).ToString() < B.GetNodeTitle(ENodeTitleType::ListView).ToString();
		});

		for (UEdGraphNode* Node : EntryNodes)
		{
			FPseudoEntry Entry;
			if (RenderEntryNode(Node, Entry) && Entry.Body.Num() > 0)
			{
				Entries.Add(MoveTemp(Entry));
			}
		}
		return Entries;
	}

	void EmitGraphJson(UEdGraph* Graph, TSharedPtr<FJsonObject> Root)
	{
		TArray<FPseudoEntry> Entries = RenderGraphPseudoCode(Graph);
		if (Entries.Num() == 0)
		{
			return;
		}

		TArray<TSharedPtr<FJsonValue>> LineValues;
		for (const FPseudoEntry& Entry : Entries)
		{
			LineValues.Add(MakeShared<FJsonValueString>(Entry.Header));
			for (const FPseudoLine& Line : Entry.Body)
			{
				const FString Pad = FString::ChrN(Line.Indent * 2, TEXT(' '));
				LineValues.Add(MakeShared<FJsonValueString>(Pad + Line.Text));
			}
		}
		Root->SetArrayField(Graph->GetName(), LineValues);
	}

	FString MakeHashedFallbackPath(UBlueprint* Blueprint)
	{
		FSHA1 DirHash;
		const FString FullPath = Blueprint->GetPathName();
		DirHash.UpdateWithString(*FullPath, FullPath.Len());
		DirHash.Final();
		uint8 Hash[FSHA1::DigestSize];
		DirHash.GetHash(Hash);
		const FString ShortDirHash = BytesToHex(Hash, 6);

		const FString FallbackDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("BlueprintSnapshots_Hashed"), ShortDirHash);
		return FallbackDir / (Blueprint->GetName() + TEXT(".json"));
	}
}

bool FWxBlueprintSnapshotExporter::ExportBlueprint(UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		return false;
	}

	const UWxBlueprintSnapshotSettings* Settings = GetDefault<UWxBlueprintSnapshotSettings>();
	if (!Settings)
	{
		return false;
	}

	TSharedRef<FJsonObject> Root = BuildSnapshot(Blueprint, *Settings);

	FString LatestPath = ResolveLatestPath(Blueprint);
	if (LatestPath.IsEmpty())
	{
		return false;
	}

	// Windows MAX_PATH 가드: 경로가 과도하게 길면 BP 경로 해시로 폴더를 단축한다.
	const int32 MaxPathLen = 240;
	if (LatestPath.Len() > MaxPathLen)
	{
		LatestPath = WxBlueprintSnapshotPrivate::MakeHashedFallbackPath(Blueprint);
	}

	IFileManager& FileManager = IFileManager::Get();
	FileManager.MakeDirectory(*FPaths::GetPath(LatestPath), true);

	const FString Json = SerializeJson(Root);

	FString PreviousJson;
	if (FFileHelper::LoadFileToString(PreviousJson, *LatestPath))
	{
		if (PreviousJson.Equals(Json, ESearchCase::CaseSensitive))
		{
			return false;
		}
	}

	// Read-only (SCC 추적 등) 인 경우 해제 시도
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (PlatformFile.FileExists(*LatestPath) && PlatformFile.IsReadOnly(*LatestPath))
	{
		PlatformFile.SetReadOnly(*LatestPath, false);
	}

	if (!FFileHelper::SaveStringToFile(Json, *LatestPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogTemp, Warning, TEXT("[WxBlueprintSnapshot] Failed to write %s"), *LatestPath);
		return false;
	}

	return true;
}

TSharedRef<FJsonObject> FWxBlueprintSnapshotExporter::BuildSnapshot(UBlueprint* Blueprint, const UWxBlueprintSnapshotSettings& Settings)
{
	using WxBlueprintSnapshotPrivate::SetObjectFieldIfNonEmpty;

	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("blueprintPath"), Blueprint->GetPathName());
	Root->SetStringField(TEXT("parentClass"), Blueprint->ParentClass ? Blueprint->ParentClass->GetPathName() : TEXT(""));

	UBlueprintGeneratedClass* GeneratedClass = Cast<UBlueprintGeneratedClass>(Blueprint->GeneratedClass);
	if (GeneratedClass && Blueprint->ParentClass)
	{
		UObject* InstanceCDO = GeneratedClass->GetDefaultObject(false);
		UObject* ParentCDO = Blueprint->ParentClass->GetDefaultObject(false);
		if (InstanceCDO && ParentCDO)
		{
			SetObjectFieldIfNonEmpty(*Root, TEXT("classDefaults"), BuildClassDefaults(InstanceCDO, ParentCDO));
		}
	}

	if (Settings.bIncludeComponents)
	{
		if (USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript)
		{
			SetObjectFieldIfNonEmpty(*Root, TEXT("components"), BuildComponentsJson(SCS));
		}
	}

	if (Settings.bIncludeVariables)
	{
		SetObjectFieldIfNonEmpty(*Root, TEXT("variables"), BuildVariablesJson(Blueprint));
	}

	if (Settings.bIncludeInterfaces)
	{
		SetObjectFieldIfNonEmpty(*Root, TEXT("interfaces"), BuildInterfacesJson(Blueprint));
	}

	if (Settings.bIncludeGraphs)
	{
		SetObjectFieldIfNonEmpty(*Root, TEXT("graphs"), BuildGraphsJson(Blueprint));
	}

	if (UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(Blueprint))
	{
		if (Settings.bIncludeWidgetTree)
		{
			SetObjectFieldIfNonEmpty(*Root, TEXT("widgetTree"), BuildWidgetTreeJson(WidgetBlueprint->WidgetTree));
		}

		if (Settings.bIncludeMVVM)
		{
			SetObjectFieldIfNonEmpty(*Root, TEXT("mvvm"), BuildMvvmJson(WidgetBlueprint));
		}
	}

	return Root;
}

TSharedPtr<FJsonObject> FWxBlueprintSnapshotExporter::BuildWidgetTreeJson(UWidgetTree* WidgetTree)
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

	if (UWidget* RootWidget = WidgetTree->RootWidget)
	{
		Root->SetStringField(TEXT("root"), RootWidget->GetName());
	}

	TSharedPtr<FJsonObject> WidgetsMap = MakeShared<FJsonObject>();
	WidgetTree->ForEachWidget([&WidgetsMap](UWidget* Widget)
	{
		if (!Widget)
		{
			return;
		}
		TSharedPtr<FJsonObject> WidgetJson = FWxBlueprintSnapshotExporter::BuildWidgetJson(Widget);
		if (!WidgetJson.IsValid())
		{
			return;
		}
		FString Key = Widget->GetName();
		if (WidgetsMap->HasField(Key))
		{
			int32 Suffix = 2;
			FString Unique;
			do
			{
				Unique = FString::Printf(TEXT("%s#%d"), *Key, Suffix++);
			}
			while (WidgetsMap->HasField(Unique));
			Key = MoveTemp(Unique);
		}
		WidgetsMap->SetObjectField(Key, WidgetJson.ToSharedRef());
	});
	Root->SetObjectField(TEXT("widgets"), WidgetsMap.ToSharedRef());

	return Root;
}

TSharedPtr<FJsonObject> FWxBlueprintSnapshotExporter::BuildWidgetJson(UWidget* Widget)
{
	if (!Widget)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField(TEXT("class"), Widget->GetClass()->GetPathName());

	if (Widget->bIsVariable)
	{
		Json->SetBoolField(TEXT("isVariable"), true);
	}

	if (UWidget* Parent = Widget->GetParent())
	{
		Json->SetStringField(TEXT("parent"), Parent->GetName());
	}

	if (UPanelSlot* Slot = Widget->Slot)
	{
		TSharedPtr<FJsonObject> SlotJson = MakeShared<FJsonObject>();
		SlotJson->SetStringField(TEXT("class"), Slot->GetClass()->GetPathName());
		UObject* SlotDefaults = Slot->GetClass()->GetDefaultObject(false);
		TSharedPtr<FJsonObject> SlotDelta = BuildClassDefaults(Slot, SlotDefaults);
		if (SlotDelta.IsValid() && SlotDelta->Values.Num() > 0)
		{
			SlotJson->SetObjectField(TEXT("delta"), SlotDelta.ToSharedRef());
		}
		Json->SetObjectField(TEXT("slot"), SlotJson.ToSharedRef());
	}

	UObject* ClassDefaults = Widget->GetClass()->GetDefaultObject(false);
	TSharedPtr<FJsonObject> Delta = BuildClassDefaults(Widget, ClassDefaults);
	if (Delta.IsValid())
	{
		// Widget의 delta 안에 있는 "Slot" 필드는 WidgetTree 내부 경로 문자열로,
		// 최상위 slot/parent 정보와 중복되므로 드롭한다.
		Delta->RemoveField(TEXT("Slot"));
		if (Delta->Values.Num() > 0)
		{
			Json->SetObjectField(TEXT("delta"), Delta.ToSharedRef());
		}
	}

	return Json;
}

TSharedPtr<FJsonObject> FWxBlueprintSnapshotExporter::BuildMvvmJson(UWidgetBlueprint* WidgetBlueprint)
{
	if (!WidgetBlueprint)
	{
		return nullptr;
	}

	UMVVMWidgetBlueprintExtension_View* Extension = UWidgetBlueprintExtension::GetExtension<UMVVMWidgetBlueprintExtension_View>(WidgetBlueprint);
	if (!Extension)
	{
		return nullptr;
	}

	const UMVVMBlueprintView* View = Extension->GetBlueprintView();
	if (!View)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

	TSharedPtr<FJsonObject> ViewModelsMap = MakeShared<FJsonObject>();
	for (const FMVVMBlueprintViewModelContext& Ctx : View->GetViewModels())
	{
		TSharedPtr<FJsonObject> VmJson = MakeShared<FJsonObject>();
		VmJson->SetStringField(TEXT("class"), Ctx.GetViewModelClass() ? Ctx.GetViewModelClass()->GetPathName() : FString());
		VmJson->SetStringField(TEXT("creationType"), StaticEnum<EMVVMBlueprintViewModelContextCreationType>()->GetNameStringByValue(static_cast<int64>(Ctx.CreationType)));
		if (!Ctx.GlobalViewModelIdentifier.IsNone())
		{
			VmJson->SetStringField(TEXT("globalIdentifier"), Ctx.GlobalViewModelIdentifier.ToString());
		}
		if (!Ctx.ViewModelPropertyPath.IsEmpty())
		{
			VmJson->SetStringField(TEXT("propertyPath"), Ctx.ViewModelPropertyPath);
		}
		VmJson->SetStringField(TEXT("contextId"), Ctx.GetViewModelId().ToString(EGuidFormats::DigitsWithHyphens));
		VmJson->SetBoolField(TEXT("optional"), Ctx.bOptional);
		VmJson->SetBoolField(TEXT("createSetter"), Ctx.bCreateSetterFunction);
		VmJson->SetBoolField(TEXT("createGetter"), Ctx.bCreateGetterFunction);

		ViewModelsMap->SetObjectField(Ctx.GetViewModelName().ToString(), VmJson.ToSharedRef());
	}
	Root->SetObjectField(TEXT("viewModels"), ViewModelsMap.ToSharedRef());

	const UClass* SelfContext = WidgetBlueprint->GeneratedClass;
	struct FBindingEntry
	{
		FString SortKey;
		TSharedPtr<FJsonObject> Json;
	};
	TArray<FBindingEntry> Entries;
	for (const FMVVMBlueprintViewBinding& Binding : View->GetBindings())
	{
		const FString Source = Binding.SourcePath.GetPropertyPath(SelfContext);
		const FString Destination = Binding.DestinationPath.GetPropertyPath(SelfContext);

		TSharedPtr<FJsonObject> BJson = MakeShared<FJsonObject>();
		BJson->SetStringField(TEXT("source"), Source);
		BJson->SetStringField(TEXT("destination"), Destination);
		BJson->SetStringField(TEXT("bindingType"), StaticEnum<EMVVMBindingMode>()->GetNameStringByValue(static_cast<int64>(Binding.BindingType)));
		BJson->SetBoolField(TEXT("enabled"), Binding.bEnabled);

		if (UMVVMBlueprintViewConversionFunction* SrcToDst = Binding.Conversion.GetConversionFunction(true))
		{
			BJson->SetStringField(TEXT("conversionSourceToDestination"), SrcToDst->GetPathName());
		}
		if (UMVVMBlueprintViewConversionFunction* DstToSrc = Binding.Conversion.GetConversionFunction(false))
		{
			BJson->SetStringField(TEXT("conversionDestinationToSource"), DstToSrc->GetPathName());
		}

		Entries.Add({ FString::Printf(TEXT("%s|%s|%d"), *Source, *Destination, static_cast<int32>(Binding.BindingType)), BJson });
	}
	Entries.Sort([](const FBindingEntry& A, const FBindingEntry& B) { return A.SortKey < B.SortKey; });

	TArray<TSharedPtr<FJsonValue>> BindingsArr;
	BindingsArr.Reserve(Entries.Num());
	for (const FBindingEntry& Entry : Entries)
	{
		BindingsArr.Add(MakeShared<FJsonValueObject>(Entry.Json));
	}
	Root->SetArrayField(TEXT("bindings"), BindingsArr);

	if (ViewModelsMap->Values.Num() == 0 && BindingsArr.Num() == 0)
	{
		return nullptr;
	}
	return Root;
}

TSharedPtr<FJsonObject> FWxBlueprintSnapshotExporter::BuildGraphsJson(UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		WxBlueprintSnapshotPrivate::EmitGraphJson(Graph, Root);
	}
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		WxBlueprintSnapshotPrivate::EmitGraphJson(Graph, Root);
	}

	return Root;
}

TSharedPtr<FJsonObject> FWxBlueprintSnapshotExporter::BuildClassDefaults(const UObject* Instance, const UObject* Defaults)
{
	if (!Instance || !Defaults)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> Delta = MakeShared<FJsonObject>();
	const UClass* InstanceClass = Instance->GetClass();

	for (TFieldIterator<FProperty> It(InstanceClass); It; ++It)
	{
		FProperty* Property = *It;
		if (WxBlueprintSnapshotPrivate::IsSkippableProperty(Property))
		{
			continue;
		}
		if (!Property->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible | CPF_BlueprintAssignable))
		{
			continue;
		}

		const void* InstancePtr = Property->ContainerPtrToValuePtr<void>(Instance);
		const UClass* OwnerClass = Property->GetOwnerClass();
		const void* DefaultPtr = (OwnerClass && Defaults->GetClass()->IsChildOf(OwnerClass))
			? Property->ContainerPtrToValuePtr<void>(Defaults)
			: nullptr;

		bool bIdentical = false;
		if (DefaultPtr)
		{
			bIdentical = Property->Identical(InstancePtr, DefaultPtr, PPF_DeepComparison | PPF_DeepCompareInstances);
		}

		if (bIdentical)
		{
			continue;
		}

		// Text-level equality fallback: Identical() can flag Instanced subobjects as different
		// even when their exported values match (pointer-level compare).
		if (DefaultPtr)
		{
			const FString InstanceText = WxBlueprintSnapshotPrivate::ExportPropertyValue(Property, InstancePtr, Instance);
			const FString DefaultText = WxBlueprintSnapshotPrivate::ExportPropertyValue(Property, DefaultPtr, Defaults);
			if (InstanceText.Equals(DefaultText, ESearchCase::CaseSensitive))
			{
				continue;
			}
		}

		Delta->SetField(Property->GetName(), WxBlueprintSnapshotPrivate::PropertyValueToJson(Property, InstancePtr, DefaultPtr, Instance));
	}

	return Delta;
}

TSharedPtr<FJsonObject> FWxBlueprintSnapshotExporter::BuildComponentsJson(USimpleConstructionScript* SCS)
{
	if (!SCS)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	for (USCS_Node* Node : SCS->GetAllNodes())
	{
		TSharedPtr<FJsonObject> NodeJson = BuildScsNodeJson(Node);
		if (NodeJson.IsValid())
		{
			Root->SetObjectField(Node->GetVariableName().ToString(), NodeJson.ToSharedRef());
		}
	}
	return Root;
}

TSharedPtr<FJsonObject> FWxBlueprintSnapshotExporter::BuildScsNodeJson(USCS_Node* Node)
{
	if (!Node)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> NodeJson = MakeShared<FJsonObject>();
	NodeJson->SetStringField(TEXT("variableName"), Node->GetVariableName().ToString());

	UActorComponent* Template = Node->ComponentTemplate;
	if (Template)
	{
		NodeJson->SetStringField(TEXT("componentClass"), Template->GetClass()->GetPathName());

		UObject* ClassDefaults = Template->GetClass()->GetDefaultObject(false);
		TSharedPtr<FJsonObject> TemplateDelta = BuildClassDefaults(Template, ClassDefaults);
		if (TemplateDelta.IsValid() && TemplateDelta->Values.Num() > 0)
		{
			NodeJson->SetObjectField(TEXT("delta"), TemplateDelta.ToSharedRef());
		}
	}

	if (USCS_Node* Parent = Node->GetSCS() ? Node->GetSCS()->FindParentNode(Node) : nullptr)
	{
		NodeJson->SetStringField(TEXT("attachParent"), Parent->GetVariableName().ToString());
	}

	if (!Node->AttachToName.IsNone())
	{
		NodeJson->SetStringField(TEXT("attachSocket"), Node->AttachToName.ToString());
	}

	return NodeJson;
}

TSharedPtr<FJsonObject> FWxBlueprintSnapshotExporter::BuildVariablesJson(UBlueprint* Blueprint)
{
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	for (const FBPVariableDescription& Var : Blueprint->NewVariables)
	{
		FString TypeStr;
		if (UObject* SubCatObj = Var.VarType.PinSubCategoryObject.Get())
		{
			TypeStr = SubCatObj->GetName();
		}
		else
		{
			TypeStr = Var.VarType.PinCategory.ToString();
		}

		if (Var.VarType.IsArray())
		{
			TypeStr = FString::Printf(TEXT("%s[]"), *TypeStr);
		}
		else if (Var.VarType.IsSet())
		{
			TypeStr = FString::Printf(TEXT("Set<%s>"), *TypeStr);
		}
		else if (Var.VarType.IsMap())
		{
			TypeStr = FString::Printf(TEXT("Map<%s>"), *TypeStr);
		}

		TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("type"), TypeStr);
		Entry->SetStringField(TEXT("value"), Var.DefaultValue);
		Root->SetObjectField(Var.VarName.ToString(), Entry.ToSharedRef());
	}
	return Root;
}

TSharedPtr<FJsonObject> FWxBlueprintSnapshotExporter::BuildInterfacesJson(UBlueprint* Blueprint)
{
	TArray<TSharedPtr<FJsonValue>> Arr;
	for (const FBPInterfaceDescription& Desc : Blueprint->ImplementedInterfaces)
	{
		if (Desc.Interface)
		{
			Arr.Add(MakeShared<FJsonValueString>(Desc.Interface->GetPathName()));
		}
	}

	if (Arr.Num() == 0)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetArrayField(TEXT("implemented"), Arr);
	return Root;
}

FString FWxBlueprintSnapshotExporter::SerializeJson(TSharedRef<FJsonObject> RootObject)
{
	WxBlueprintSnapshotPrivate::SortJsonObjectRecursive(RootObject);

	FString Out;
	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Out);
	FJsonSerializer::Serialize(RootObject, Writer);
	return Out;
}

FString FWxBlueprintSnapshotExporter::ResolveLatestPath(UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		return FString();
	}

	UPackage* Package = Blueprint->GetOutermost();
	if (!Package)
	{
		return FString();
	}

	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("WxBlueprintSnapshot"));
	if (!Plugin.IsValid())
	{
		return FString();
	}

	// /Game/UI/Widget/WBP_Ability -> Game/UI/Widget/WBP_Ability.json
	FString PackagePath = Package->GetName();
	if (PackagePath.StartsWith(TEXT("/")))
	{
		PackagePath.RemoveAt(0);
	}

	return FPaths::Combine(Plugin->GetBaseDir(), TEXT("Snapshots"), PackagePath) + TEXT(".json");
}

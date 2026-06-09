// Copyright Woogle. All Rights Reserved.

#include "WxBlueprintSnapshotExporter.h"
#include "WxBlueprintSnapshotModule.h"
#include "WxBlueprintSnapshotSettings.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EdGraphSchema_K2.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Components/ActorComponent.h"
#include "WidgetBlueprint.h"
#include "UObject/Package.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"

namespace
{
	void SortJsonValueRecursive(const TSharedPtr<FJsonValue>& Value);

	void SortJsonObjectRecursive(TSharedPtr<FJsonObject> Obj)
	{
		if (!Obj.IsValid())
		{
			return;
		}
		Obj->Values.KeySort(TLess<FString>());
		for (auto& Pair : Obj->Values)
		{
			SortJsonValueRecursive(Pair.Value);
		}
	}

	void SortJsonValueRecursive(const TSharedPtr<FJsonValue>& Value)
	{
		if (!Value.IsValid())
		{
			return;
		}
		if (Value->Type == EJson::Object)
		{
			SortJsonObjectRecursive(Value->AsObject());
			return;
		}
		if (Value->Type == EJson::Array)
		{
			for (const TSharedPtr<FJsonValue>& Elem : Value->AsArray())
			{
				SortJsonValueRecursive(Elem);
			}
		}
	}

	// Top-level 정렬: BP 선언/SCS 순서가 의미 있는 키는 알파벳 정렬을 건너뛰고 입력 순서를 유지한다.
	// 단, 그 자식의 속성(예: 각 component 의 class/delta 멤버) 은 재귀적으로 알파벳 정렬해 diff 안정성을 살린다.
	void SortJsonRootObject(TSharedPtr<FJsonObject> Root)
	{
		if (!Root.IsValid())
		{
			return;
		}
		static const TSet<FString> KeepInsertOrder = {
			TEXT("variables"),    // BP Variables 패널 선언 순서
			TEXT("functions"),    // BP My Blueprint 함수 순서
			TEXT("macros"),       // BP My Blueprint 매크로 순서
			TEXT("components")    // SCS 트리 순회 순서 (parent → child)
		};
		Root->Values.KeySort(TLess<FString>());
		for (auto& Pair : Root->Values)
		{
			if (KeepInsertOrder.Contains(Pair.Key) && Pair.Value.IsValid() && Pair.Value->Type == EJson::Object)
			{
				// 이 키의 직속 자식 object 는 키 정렬을 건너뛴다. 손자(grandchild) 들은 재귀 정렬.
				TSharedPtr<FJsonObject> ChildObj = Pair.Value->AsObject();
				if (ChildObj.IsValid())
				{
					for (auto& ChildPair : ChildObj->Values)
					{
						SortJsonValueRecursive(ChildPair.Value);
					}
				}
				continue;
			}
			SortJsonValueRecursive(Pair.Value);
		}
	}

	void SetObjectFieldIfNonEmpty(FJsonObject& Root, const FString& Key, const TSharedPtr<FJsonObject>& Value)
	{
		if (Value.IsValid() && Value->Values.Num() > 0)
		{
			Root.SetObjectField(Key, Value.ToSharedRef());
		}
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

	UPackage* Package = Blueprint->GetOutermost();
	FString LatestPath = Package ? ResolveSnapshotPath(Package->GetName()) : FString();
	if (LatestPath.IsEmpty())
	{
		return false;
	}

	// Windows MAX_PATH 가드: 경로가 과도하게 길면 SaveStringToFile이 실패하므로, 조용히 폴백하지 않고 명시적으로 스킵한다.
	const int32 MaxPathLen = 240;
	if (LatestPath.Len() > MaxPathLen)
	{
		UE_LOG(LogWxBPSnapshot, Error,
			TEXT("Snapshot path exceeds %d chars (len=%d), skipping. BP=%s  Path=%s"),
			MaxPathLen, LatestPath.Len(), *Blueprint->GetPathName(), *LatestPath);
		return false;
	}

	IFileManager& FileManager = IFileManager::Get();
	FileManager.MakeDirectory(*FPaths::GetPath(LatestPath), true);

	const FString Json = SerializeJson(Root);

	// Read-only (SCC 추적 등) 인 경우 해제 시도. 저장 실패하면 원상 복구.
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	bool bClearedReadOnly = false;
	if (PlatformFile.FileExists(*LatestPath) && PlatformFile.IsReadOnly(*LatestPath))
	{
		PlatformFile.SetReadOnly(*LatestPath, false);
		bClearedReadOnly = true;
	}

	if (!FFileHelper::SaveStringToFile(Json, *LatestPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		if (bClearedReadOnly)
		{
			PlatformFile.SetReadOnly(*LatestPath, true);
		}
		UE_LOG(LogWxBPSnapshot, Warning, TEXT("Failed to write %s"), *LatestPath);
		return false;
	}

	return true;
}

FString FWxBlueprintSnapshotExporter::ResolveSnapshotPath(const FString& PackageName)
{
	if (PackageName.IsEmpty())
	{
		return FString();
	}

	const UWxBlueprintSnapshotSettings* Settings = GetDefault<UWxBlueprintSnapshotSettings>();
	if (!Settings)
	{
		return FString();
	}

	FString RootDir = Settings->OutputDirectory.Path;
	if (RootDir.IsEmpty())
	{
		TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("WxBlueprintSnapshot"));
		if (!Plugin.IsValid())
		{
			return FString();
		}
		RootDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Snapshots"));
	}
	else if (FPaths::IsRelative(RootDir))
	{
		RootDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), RootDir);
	}

	// /Game/UI/Widget/WBP_Ability -> Game/UI/Widget/WBP_Ability{Ext}
	FString PackagePath = PackageName;
	if (PackagePath.StartsWith(TEXT("/")))
	{
		PackagePath.RemoveAt(0);
	}

	return FPaths::Combine(RootDir, PackagePath) + Settings->FileExtension;
}

TSharedRef<FJsonObject> FWxBlueprintSnapshotExporter::BuildSnapshot(UBlueprint* Blueprint, const UWxBlueprintSnapshotSettings& Settings)
{
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
			// NewVariables / SCS 컴포넌트는 각각 `variables` / `components` 필드에 기록되므로 classDefaults 델타에서 중복 제외.
			TSet<FName> ExcludedNames;
			ExcludedNames.Reserve(Blueprint->NewVariables.Num());
			for (const FBPVariableDescription& Var : Blueprint->NewVariables)
			{
				ExcludedNames.Add(Var.VarName);
			}
			if (USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript)
			{
				for (USCS_Node* Node : SCS->GetAllNodes())
				{
					if (Node)
					{
						ExcludedNames.Add(Node->GetVariableName());
					}
				}
			}
			SetObjectFieldIfNonEmpty(*Root, TEXT("classDefaults"), BuildClassDefaults(InstanceCDO, ParentCDO, ExcludedNames));
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
		SetObjectFieldIfNonEmpty(*Root, TEXT("eventGraph"), BuildEventGraphJson(Blueprint));
		SetObjectFieldIfNonEmpty(*Root, TEXT("functions"), BuildFunctionsJson(Blueprint));
		SetObjectFieldIfNonEmpty(*Root, TEXT("macros"), BuildMacrosJson(Blueprint));
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

	if (USimpleConstructionScript* SCS = Node->GetSCS())
	{
		if (USCS_Node* Parent = SCS->FindParentNode(Node))
		{
			NodeJson->SetStringField(TEXT("attachParent"), Parent->GetVariableName().ToString());
		}
	}

	if (!Node->AttachToName.IsNone())
	{
		NodeJson->SetStringField(TEXT("attachSocket"), Node->AttachToName.ToString());
	}

	return NodeJson;
}

namespace
{
	bool IsObjectLikeCat(const FName& Cat)
	{
		return Cat == UEdGraphSchema_K2::PC_Object
			|| Cat == UEdGraphSchema_K2::PC_Class
			|| Cat == UEdGraphSchema_K2::PC_Interface
			|| Cat == UEdGraphSchema_K2::PC_SoftObject
			|| Cat == UEdGraphSchema_K2::PC_SoftClass;
	}

	FString FormatVarTerminalCPP(const FName& Cat, UObject* SubObj)
	{
		if (UClass* AsClass = Cast<UClass>(SubObj))
		{
			const FString ClassName = FString::Printf(TEXT("%s%s"), AsClass->GetPrefixCPP(), *AsClass->GetName());
			if (Cat == UEdGraphSchema_K2::PC_Class)       { return FString::Printf(TEXT("TSubclassOf<%s>"), *ClassName); }
			if (Cat == UEdGraphSchema_K2::PC_SoftObject)  { return FString::Printf(TEXT("TSoftObjectPtr<%s>"), *ClassName); }
			if (Cat == UEdGraphSchema_K2::PC_SoftClass)   { return FString::Printf(TEXT("TSoftClassPtr<%s>"), *ClassName); }
			if (Cat == UEdGraphSchema_K2::PC_Interface)   { return FString::Printf(TEXT("TScriptInterface<%s>"), *ClassName); }
			// PC_Object: raw pointer
			return FString::Printf(TEXT("%s*"), *ClassName);
		}
		if (UScriptStruct* AsStruct = Cast<UScriptStruct>(SubObj))
		{
			return FString::Printf(TEXT("F%s"), *AsStruct->GetName());
		}
		if (UEnum* AsEnum = Cast<UEnum>(SubObj))
		{
			return AsEnum->GetName();
		}

		if (Cat == UEdGraphSchema_K2::PC_Boolean) { return TEXT("bool"); }
		if (Cat == UEdGraphSchema_K2::PC_Int)     { return TEXT("int32"); }
		if (Cat == UEdGraphSchema_K2::PC_Int64)   { return TEXT("int64"); }
		if (Cat == UEdGraphSchema_K2::PC_Byte)    { return TEXT("uint8"); }
		if (Cat == UEdGraphSchema_K2::PC_Float)   { return TEXT("float"); }
		if (Cat == UEdGraphSchema_K2::PC_Double || Cat == UEdGraphSchema_K2::PC_Real)
		{
			return TEXT("double");
		}
		if (Cat == UEdGraphSchema_K2::PC_String)  { return TEXT("FString"); }
		if (Cat == UEdGraphSchema_K2::PC_Name)    { return TEXT("FName"); }
		if (Cat == UEdGraphSchema_K2::PC_Text)    { return TEXT("FText"); }
		if (IsObjectLikeCat(Cat))                 { return TEXT("UObject*"); }
		return Cat.ToString();
	}

	FString FormatVarTypeCPP(const FEdGraphPinType& PinType)
	{
		const FString Terminal = FormatVarTerminalCPP(PinType.PinCategory, PinType.PinSubCategoryObject.Get());
		if (PinType.IsArray())
		{
			return FString::Printf(TEXT("TArray<%s>"), *Terminal);
		}
		if (PinType.IsSet())
		{
			return FString::Printf(TEXT("TSet<%s>"), *Terminal);
		}
		if (PinType.IsMap())
		{
			const FString Value = FormatVarTerminalCPP(PinType.PinValueType.TerminalCategory, PinType.PinValueType.TerminalSubCategoryObject.Get());
			return FString::Printf(TEXT("TMap<%s, %s>"), *Terminal, *Value);
		}
		return Terminal;
	}

	// "0.000000" → "0", "1.500000" → "1.5". 정수형 표기는 그대로 둔다.
	FString TrimFloat(const FString& Value)
	{
		if (!Value.Contains(TEXT(".")))
		{
			return Value;
		}
		FString Out = Value;
		while (Out.Len() > 1 && Out.EndsWith(TEXT("0")))
		{
			Out.LeftChopInline(1, EAllowShrinking::No);
		}
		if (Out.EndsWith(TEXT(".")))
		{
			Out.LeftChopInline(1, EAllowShrinking::No);
		}
		return Out.IsEmpty() ? TEXT("0") : Out;
	}

	bool ParseStructField(const FString& StructText, const TCHAR* FieldName, FString& OutValue)
	{
		const FString Key = FString::Printf(TEXT("%s="), FieldName);
		const int32 Found = StructText.Find(Key);
		if (Found == INDEX_NONE)
		{
			return false;
		}
		const int32 Start = Found + Key.Len();
		const int32 EndComma = StructText.Find(TEXT(","), ESearchCase::CaseSensitive, ESearchDir::FromStart, Start);
		const int32 EndParen = StructText.Find(TEXT(")"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Start);
		int32 End = StructText.Len();
		if (EndComma != INDEX_NONE && (EndParen == INDEX_NONE || EndComma < EndParen))
		{
			End = EndComma;
		}
		else if (EndParen != INDEX_NONE)
		{
			End = EndParen;
		}
		OutValue = StructText.Mid(Start, End - Start);
		return true;
	}

	FString FormatStructValueCPP(const FString& Value, UScriptStruct* Struct)
	{
		const FString StructName = Struct ? FString::Printf(TEXT("F%s"), *Struct->GetName()) : TEXT("FStruct");

		if (Value.IsEmpty() || Value == TEXT("()"))
		{
			return FString::Printf(TEXT("%s()"), *StructName);
		}

		if (Struct)
		{
			const FString Name = Struct->GetName();
			if (Name == TEXT("Vector") || Name == TEXT("Vector3f"))
			{
				FString X, Y, Z;
				if (ParseStructField(Value, TEXT("X"), X) && ParseStructField(Value, TEXT("Y"), Y) && ParseStructField(Value, TEXT("Z"), Z))
				{
					return FString::Printf(TEXT("%s(%s, %s, %s)"), *StructName, *TrimFloat(X), *TrimFloat(Y), *TrimFloat(Z));
				}
			}
			else if (Name == TEXT("Vector2D") || Name == TEXT("Vector2f"))
			{
				FString X, Y;
				if (ParseStructField(Value, TEXT("X"), X) && ParseStructField(Value, TEXT("Y"), Y))
				{
					return FString::Printf(TEXT("%s(%s, %s)"), *StructName, *TrimFloat(X), *TrimFloat(Y));
				}
			}
			else if (Name == TEXT("Vector4"))
			{
				FString X, Y, Z, W;
				if (ParseStructField(Value, TEXT("X"), X) && ParseStructField(Value, TEXT("Y"), Y) && ParseStructField(Value, TEXT("Z"), Z) && ParseStructField(Value, TEXT("W"), W))
				{
					return FString::Printf(TEXT("%s(%s, %s, %s, %s)"), *StructName, *TrimFloat(X), *TrimFloat(Y), *TrimFloat(Z), *TrimFloat(W));
				}
			}
			else if (Name == TEXT("Rotator"))
			{
				FString P, Y, R;
				if (ParseStructField(Value, TEXT("Pitch"), P) && ParseStructField(Value, TEXT("Yaw"), Y) && ParseStructField(Value, TEXT("Roll"), R))
				{
					return FString::Printf(TEXT("%s(%s, %s, %s)"), *StructName, *TrimFloat(P), *TrimFloat(Y), *TrimFloat(R));
				}
			}
			else if (Name == TEXT("LinearColor"))
			{
				FString R, G, B, A;
				if (ParseStructField(Value, TEXT("R"), R) && ParseStructField(Value, TEXT("G"), G) && ParseStructField(Value, TEXT("B"), B) && ParseStructField(Value, TEXT("A"), A))
				{
					return FString::Printf(TEXT("%s(%s, %s, %s, %s)"), *StructName, *TrimFloat(R), *TrimFloat(G), *TrimFloat(B), *TrimFloat(A));
				}
			}
		}

		// 알려지지 않은 구조체는 BP 표기를 그대로 붙여 폴백. ex) FCustom(X=1,Y=2)
		return FString::Printf(TEXT("%s%s"), *StructName, *Value);
	}

	// 모든 값은 가능한 한 함수 캐스트 / ctor 형태로 — 값 자체가 타입을 self-explain.
	// 단 nullptr / NSLOCTEXT(...) / EEnum::Value 처럼 이미 타입이 자명하거나 캐스트가 어색한 경우는 그대로.
	FString FormatVarValueCPP(const FString& Value, const FEdGraphPinType& PinType)
	{
		if (PinType.IsContainer())
		{
			const FString FullType = FormatVarTypeCPP(PinType);
			if (Value.IsEmpty() || Value == TEXT("()"))
			{
				return FString::Printf(TEXT("%s{}"), *FullType);
			}
			// 비어있지 않은 컨테이너 BP 표기는 "(elem,elem)" — 그대로 type prefix 만 붙임.
			return FString::Printf(TEXT("%s%s"), *FullType, *Value);
		}

		const FName Cat = PinType.PinCategory;

		if (Cat == UEdGraphSchema_K2::PC_Boolean)
		{
			// bool 리터럴(true/false) 자체가 자명한 타입이라 함수 캐스트 생략.
			const bool bTrue = Value == TEXT("True") || Value == TEXT("true");
			return bTrue ? TEXT("true") : TEXT("false");
		}
		if (Cat == UEdGraphSchema_K2::PC_Int || Cat == UEdGraphSchema_K2::PC_Int64)
		{
			return Value.IsEmpty() ? TEXT("0") : Value;
		}
		if (Cat == UEdGraphSchema_K2::PC_Byte)
		{
			if (UEnum* AsEnum = Cast<UEnum>(PinType.PinSubCategoryObject.Get()))
			{
				if (Value.IsEmpty() || Value == TEXT("0"))
				{
					return FString::Printf(TEXT("%s(0)"), *AsEnum->GetName());
				}
				return Value.Contains(TEXT("::")) ? Value : FString::Printf(TEXT("%s::%s"), *AsEnum->GetName(), *Value);
			}
			return Value.IsEmpty() ? TEXT("0") : Value;
		}
		if (Cat == UEdGraphSchema_K2::PC_Float)
		{
			FString Trimmed = Value.IsEmpty() ? TEXT("0") : TrimFloat(Value);
			if (!Trimmed.Contains(TEXT(".")))
			{
				Trimmed.Append(TEXT("."));
			}
			Trimmed.Append(TEXT("f"));
			return Trimmed;
		}
		if (Cat == UEdGraphSchema_K2::PC_Double || Cat == UEdGraphSchema_K2::PC_Real)
		{
			FString Trimmed = Value.IsEmpty() ? TEXT("0") : TrimFloat(Value);
			if (!Trimmed.Contains(TEXT(".")))
			{
				Trimmed.Append(TEXT(".0"));
			}
			return Trimmed;
		}
		if (Cat == UEdGraphSchema_K2::PC_String)
		{
			FString Escaped = Value.ReplaceCharWithEscapedChar();
			return FString::Printf(TEXT("FString(\"%s\")"), *Escaped);
		}
		if (Cat == UEdGraphSchema_K2::PC_Name)
		{
			return (Value.IsEmpty() || Value == TEXT("None")) ? TEXT("NAME_None") : FString::Printf(TEXT("FName(\"%s\")"), *Value);
		}
		if (Cat == UEdGraphSchema_K2::PC_Text)
		{
			// FText 는 NSLOCTEXT/INVTEXT 가 이미 타입을 표현 — 그대로.
			return Value.IsEmpty() ? TEXT("FText::GetEmpty()") : Value;
		}
		if (IsObjectLikeCat(Cat))
		{
			const bool bIsNull = Value.IsEmpty() || Value == TEXT("None");
			if (Cat == UEdGraphSchema_K2::PC_Object)
			{
				// raw pointer: T*(nullptr) 은 invalid C++ — type 손실 감수.
				return bIsNull ? TEXT("nullptr") : Value;
			}
			// wrapper 타입 (TSubclassOf / TSoftObjectPtr / TSoftClassPtr / TScriptInterface) 은 함수 캐스트 가능.
			const FString FullType = FormatVarTypeCPP(PinType);
			return bIsNull
				? FString::Printf(TEXT("%s(nullptr)"), *FullType)
				: FString::Printf(TEXT("%s(%s)"), *FullType, *Value);
		}
		if (Cat == UEdGraphSchema_K2::PC_Struct)
		{
			return FormatStructValueCPP(Value, Cast<UScriptStruct>(PinType.PinSubCategoryObject.Get()));
		}
		if (UEnum* AsEnum = Cast<UEnum>(PinType.PinSubCategoryObject.Get()))
		{
			return Value.Contains(TEXT("::")) ? Value : FString::Printf(TEXT("%s::%s"), *AsEnum->GetName(), *Value);
		}
		return Value;
	}
}

TSharedPtr<FJsonObject> FWxBlueprintSnapshotExporter::BuildVariablesJson(UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		return nullptr;
	}

	UBlueprintGeneratedClass* GeneratedClass = Cast<UBlueprintGeneratedClass>(Blueprint->GeneratedClass);
	UObject* CDO = GeneratedClass ? GeneratedClass->GetDefaultObject(false) : nullptr;

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	for (const FBPVariableDescription& Var : Blueprint->NewVariables)
	{
		// CDO에서 직접 기본값 읽기. FBPVariableDescription::DefaultValue는 유저가 명시적으로 바꾼 경우만 채워진다.
		FString RawValue;
		if (CDO && GeneratedClass)
		{
			if (FProperty* Prop = GeneratedClass->FindPropertyByName(Var.VarName))
			{
				Prop->ExportTextItem_InContainer(RawValue, CDO, nullptr, CDO, PPF_None);
			}
		}
		if (RawValue.IsEmpty())
		{
			RawValue = Var.DefaultValue;
		}

		const FString ValueCPP = FormatVarValueCPP(RawValue, Var.VarType);
		Root->SetStringField(Var.VarName.ToString(), ValueCPP);
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
	SortJsonRootObject(RootObject);

	FString Out;
	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Out);
	FJsonSerializer::Serialize(RootObject, Writer);
	return Out;
}

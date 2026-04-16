// Copyright Woogle. All Rights Reserved.

#include "WxBlueprintSnapshotExporter.h"
#include "UObject/UnrealType.h"
#include "UObject/Class.h"
#include "UObject/TextProperty.h"
#include "UObject/StructOnScope.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

namespace
{
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

	FString ExportPropertyValue(const FProperty* Property, const void* ValuePtr, const UObject* Parent)
	{
		FString Out;
		Property->ExportText_Direct(Out, ValuePtr, ValuePtr, const_cast<UObject*>(Parent), PPF_SimpleObjectText);
		return Out;
	}

	// CDO delta 추출 시 PPF용 Owner와 사이클 가드용 visited Set을 한 번에 들고 다닌다.
	struct FExportCtx
	{
		const UObject* Owner = nullptr;
		TSet<const UObject*> InstanceVisited;
	};

	TSharedPtr<FJsonValue> PropertyValueToJson(const FProperty* Property, const void* ValuePtr, const void* DefaultPtr, FExportCtx& Ctx);
	TSharedPtr<FJsonObject> BuildClassDefaultsImpl(const UObject* Instance, const UObject* Defaults, FExportCtx& Ctx);

	TSharedPtr<FStructOnScope> MakeElementDefault(const FProperty* ElemProp)
	{
		if (const FStructProperty* ElemStruct = CastField<FStructProperty>(ElemProp))
		{
			return MakeShared<FStructOnScope>(ElemStruct->Struct);
		}
		return nullptr;
	}

	void StructToJsonObject(const UScriptStruct* Struct, const void* StructPtr, const void* DefaultStructPtr, TSharedPtr<FJsonObject> Out, FExportCtx& Ctx)
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

			TSharedPtr<FJsonValue> Value = PropertyValueToJson(Inner, InnerPtr, InnerDefaultPtr, Ctx);
			// 모든 하위 필드가 기본값과 동일해 빈 오브젝트가 된 struct는 드롭한다.
			if (Value.IsValid() && Value->Type == EJson::Object && Value->AsObject()->Values.Num() == 0)
			{
				continue;
			}
			Out->SetField(Inner->GetName(), Value);
		}
	}

	TSharedPtr<FJsonValue> PropertyValueToJson(const FProperty* Property, const void* ValuePtr, const void* DefaultPtr, FExportCtx& Ctx)
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
			const bool bInstanced = Property->HasAnyPropertyFlags(CPF_InstancedReference | CPF_PersistentInstance);
			// 첫 등장한 instanced subobject만 풀 dump. 동일 subobject가 다른 경로에서 재등장하거나
			// non-instanced 참조는 path 문자열로만 남아 결과 필드 타입이 object ↔ string으로 갈릴 수 있음.
			if (bInstanced && Obj && !Ctx.InstanceVisited.Contains(Obj))
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

				TSharedPtr<FJsonObject> SubDelta = BuildClassDefaultsImpl(Obj, DefaultsForDelta, Ctx);
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
			StructToJsonObject(StructProp->Struct, ValuePtr, DefaultPtr, Obj, Ctx);
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
				Arr.Add(PropertyValueToJson(ArrProp->Inner, Helper.GetRawPtr(i), ElemDefaultMem, Ctx));
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
				Arr.Add(PropertyValueToJson(SetProp->ElementProp, Helper.GetElementPtr(i), ElemDefaultMem, Ctx));
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
					const TSharedPtr<FJsonValue> KeyVal = PropertyValueToJson(KeyProp, Helper.GetKeyPtr(i), nullptr, Ctx);
					Obj->SetField(KeyVal->AsString(), PropertyValueToJson(MapProp->ValueProp, Helper.GetValuePtr(i), ValueDefaultMem, Ctx));
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
				Entry->SetField(TEXT("key"), PropertyValueToJson(KeyProp, Helper.GetKeyPtr(i), KeyDefaultMem, Ctx));
				Entry->SetField(TEXT("value"), PropertyValueToJson(MapProp->ValueProp, Helper.GetValuePtr(i), ValueDefaultMem, Ctx));
				Arr.Add(MakeShared<FJsonValueObject>(Entry));
			}
			return MakeShared<FJsonValueArray>(Arr);
		}

		FString Out;
		Property->ExportText_Direct(Out, ValuePtr, ValuePtr, const_cast<UObject*>(Ctx.Owner), PPF_SimpleObjectText);
		return MakeShared<FJsonValueString>(Out);
	}

	TSharedPtr<FJsonObject> BuildClassDefaultsImpl(const UObject* Instance, const UObject* Defaults, FExportCtx& Ctx)
	{
		if (!Instance || !Defaults)
		{
			return nullptr;
		}

		// 사이클 가드: 동일 instanced 서브오브젝트가 다시 등장해도 한 번만 dump.
		Ctx.InstanceVisited.Add(Instance);

		// 진입한 인스턴스를 PPF용 Owner로 임시 사용. 스코프 종료 시 이전 값으로 복귀.
		TGuardValue<const UObject*> OwnerGuard(Ctx.Owner, Instance);

		TSharedPtr<FJsonObject> Delta = MakeShared<FJsonObject>();
		const UClass* InstanceClass = Instance->GetClass();

		for (TFieldIterator<FProperty> It(InstanceClass); It; ++It)
		{
			FProperty* Property = *It;
			if (IsSkippableProperty(Property))
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

			if (DefaultPtr && Property->Identical(InstancePtr, DefaultPtr, PPF_DeepComparison | PPF_DeepCompareInstances))
			{
				continue;
			}

			// Identical()은 instanced 서브오브젝트를 포인터 비교로 다르다고 판정할 수 있어
			// ExportText 텍스트 비교를 한 번 더 해서 false-positive를 거른다.
			if (DefaultPtr)
			{
				const FString InstanceText = ExportPropertyValue(Property, InstancePtr, Instance);
				const FString DefaultText = ExportPropertyValue(Property, DefaultPtr, Defaults);
				if (InstanceText.Equals(DefaultText, ESearchCase::CaseSensitive))
				{
					continue;
				}
			}

			Delta->SetField(Property->GetName(), PropertyValueToJson(Property, InstancePtr, DefaultPtr, Ctx));
		}

		return Delta;
	}
}

TSharedPtr<FJsonObject> FWxBlueprintSnapshotExporter::BuildClassDefaults(const UObject* Instance, const UObject* Defaults)
{
	FExportCtx Ctx;
	return BuildClassDefaultsImpl(Instance, Defaults, Ctx);
}

// Copyright Woogle. All Rights Reserved.

#include "WxLevelSnapshotExporter.h"
#include "UObject/UnrealType.h"
#include "UObject/Class.h"
#include "UObject/TextProperty.h"
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
		// Delegate/멀티캐스트 델리게이트는 ExportText로 사람이 읽을 수 없는 형태가 되므로 스냅샷에서 제외.
		if (Property->IsA<FDelegateProperty>() || Property->IsA<FMulticastDelegateProperty>())
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

	struct FExportCtx
	{
		const UObject* Owner = nullptr;
		TSet<const UObject*> InstanceVisited;
	};

	TSharedPtr<FJsonValue> PropertyValueToJson(const FProperty* Property, const void* ValuePtr, FExportCtx& Ctx);
	TSharedPtr<FJsonObject> BuildPropertiesImpl(const UObject* Instance, FExportCtx& Ctx);

	// 빈 오브젝트(모든 필드가 프루닝되어 Values.Num()==0)는 struct/object 직접 필드 레벨에서만 생략한다.
	// 배열/Set/Map 내부에서는 인덱스·키 순서 의미가 있으므로 빈 엔트리도 보존한다.
	void StructToJsonObject(const UScriptStruct* Struct, const void* StructPtr, TSharedPtr<FJsonObject> Out, FExportCtx& Ctx)
	{
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			FProperty* Inner = *It;
			if (IsSkippableProperty(Inner))
			{
				continue;
			}
			const void* InnerPtr = Inner->ContainerPtrToValuePtr<void>(StructPtr);

			TSharedPtr<FJsonValue> Value = PropertyValueToJson(Inner, InnerPtr, Ctx);
			if (Value.IsValid() && Value->Type == EJson::Object && Value->AsObject()->Values.Num() == 0)
			{
				continue;
			}
			Out->SetField(Inner->GetName(), Value);
		}
	}

	TSharedPtr<FJsonValue> PropertyValueToJson(const FProperty* Property, const void* ValuePtr, FExportCtx& Ctx)
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
			if (bInstanced && Obj && !Ctx.InstanceVisited.Contains(Obj))
			{
				TSharedPtr<FJsonObject> SubProperties = BuildPropertiesImpl(Obj, Ctx);
				const bool bHasProperties = SubProperties.IsValid() && SubProperties->Values.Num() > 0;

				TSharedPtr<FJsonObject> InnerJson = MakeShared<FJsonObject>();
				InnerJson->SetStringField(TEXT("class"), Obj->GetClass()->GetPathName());
				if (bHasProperties)
				{
					InnerJson->SetObjectField(TEXT("properties"), SubProperties.ToSharedRef());
				}
				return MakeShared<FJsonValueObject>(InnerJson);
			}
			if (Obj && Obj->GetOuter() == Ctx.Owner)
			{
				return MakeShared<FJsonValueString>(Obj->GetName());
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
			StructToJsonObject(StructProp->Struct, ValuePtr, Obj, Ctx);
			return MakeShared<FJsonValueObject>(Obj);
		}
		if (const FArrayProperty* ArrProp = CastField<FArrayProperty>(Property))
		{
			FScriptArrayHelper Helper(ArrProp, ValuePtr);
			TArray<TSharedPtr<FJsonValue>> Arr;
			for (int32 i = 0; i < Helper.Num(); ++i)
			{
				Arr.Add(PropertyValueToJson(ArrProp->Inner, Helper.GetRawPtr(i), Ctx));
			}
			return MakeShared<FJsonValueArray>(Arr);
		}
		if (const FSetProperty* SetProp = CastField<FSetProperty>(Property))
		{
			FScriptSetHelper Helper(SetProp, ValuePtr);
			TArray<TSharedPtr<FJsonValue>> Arr;
			for (int32 i = 0; i < Helper.GetMaxIndex(); ++i)
			{
				if (!Helper.IsValidIndex(i))
				{
					continue;
				}
				Arr.Add(PropertyValueToJson(SetProp->ElementProp, Helper.GetElementPtr(i), Ctx));
			}
			return MakeShared<FJsonValueArray>(Arr);
		}
		if (const FMapProperty* MapProp = CastField<FMapProperty>(Property))
		{
			FScriptMapHelper Helper(MapProp, ValuePtr);
			const FProperty* KeyProp = MapProp->KeyProp;
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
					const TSharedPtr<FJsonValue> KeyVal = PropertyValueToJson(KeyProp, Helper.GetKeyPtr(i), Ctx);
					Obj->SetField(KeyVal->AsString(), PropertyValueToJson(MapProp->ValueProp, Helper.GetValuePtr(i), Ctx));
				}
				return MakeShared<FJsonValueObject>(Obj);
			}

			// FScriptMapHelper 순회 순서는 sparse array 내부 상태에 따라 실행마다 달라질 수 있어
			// diff 노이즈가 발생한다. key의 ExportText 문자열로 안정 정렬.
			TArray<TPair<FString, TSharedPtr<FJsonValue>>> Pairs;
			Pairs.Reserve(Helper.Num());
			for (int32 i = 0; i < Helper.GetMaxIndex(); ++i)
			{
				if (!Helper.IsValidIndex(i))
				{
					continue;
				}
				TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
				Entry->SetField(TEXT("key"), PropertyValueToJson(KeyProp, Helper.GetKeyPtr(i), Ctx));
				Entry->SetField(TEXT("value"), PropertyValueToJson(MapProp->ValueProp, Helper.GetValuePtr(i), Ctx));

				FString SortKey;
				KeyProp->ExportText_Direct(SortKey, Helper.GetKeyPtr(i), nullptr, const_cast<UObject*>(Ctx.Owner), PPF_SimpleObjectText);
				Pairs.Emplace(MoveTemp(SortKey), MakeShared<FJsonValueObject>(Entry));
			}
			Pairs.Sort([](const TPair<FString, TSharedPtr<FJsonValue>>& A, const TPair<FString, TSharedPtr<FJsonValue>>& B)
			{
				return A.Key < B.Key;
			});

			TArray<TSharedPtr<FJsonValue>> Arr;
			Arr.Reserve(Pairs.Num());
			for (TPair<FString, TSharedPtr<FJsonValue>>& Pair : Pairs)
			{
				Arr.Add(MoveTemp(Pair.Value));
			}
			return MakeShared<FJsonValueArray>(Arr);
		}

		FString Out;
		Property->ExportText_Direct(Out, ValuePtr, nullptr, const_cast<UObject*>(Ctx.Owner), PPF_SimpleObjectText);
		return MakeShared<FJsonValueString>(Out);
	}

	TSharedPtr<FJsonObject> BuildPropertiesImpl(const UObject* Instance, FExportCtx& Ctx)
	{
		if (!Instance)
		{
			return nullptr;
		}

		Ctx.InstanceVisited.Add(Instance);

		TGuardValue<const UObject*> OwnerGuard(Ctx.Owner, Instance);

		TSharedPtr<FJsonObject> Properties = MakeShared<FJsonObject>();
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

			TSharedPtr<FJsonValue> FieldValue = PropertyValueToJson(Property, InstancePtr, Ctx);
			if (FieldValue.IsValid() && FieldValue->Type == EJson::Object && FieldValue->AsObject()->Values.Num() == 0)
			{
				continue;
			}
			Properties->SetField(Property->GetName(), FieldValue);
		}

		return Properties;
	}
}

TSharedPtr<FJsonObject> FWxLevelSnapshotExporter::BuildProperties(const UObject* Instance)
{
	FExportCtx Ctx;
	return BuildPropertiesImpl(Instance, Ctx);
}

// Copyright Woogle. All Rights Reserved.

#include "WxStateTreeToolset.h"

#include "AssetToolsModule.h"
#include "Components/StateTreeComponentSchema.h"
#include "Dom/JsonObject.h"
#include "IAssetTools.h"
#include "JsonObjectConverter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Logging/TokenizedMessage.h"
#include "PropertyBindingPath.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "StateTree.h"
#include "StateTreeCompilerLog.h"
#include "StateTreeEditingSubsystem.h"
#include "StateTreeEditorData.h"
#include "StateTreeEditorPropertyBindings.h"
#include "StateTreeFactory.h"
#include "StateTreeReference.h"
#include "StateTreeState.h"
#include "StructUtils/PropertyBag.h"

namespace
{
	/** 에셋의 에디터 데이터를 얻는다. 실패 시 스크립트 에러를 올리고 null 을 돌려준다. */
	UStateTreeEditorData* GetEditorData(UStateTree* StateTree)
	{
		if (!StateTree)
		{
			UKismetSystemLibrary::RaiseScriptError(TEXT("StateTree 가 null 이다."));
			return nullptr;
		}

		UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
		if (!EditorData)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("'%s' 에 에디터 데이터가 없다."), *StateTree->GetPathName()));
		}
		return EditorData;
	}

	/** 루트 파라미터 백은 private UPROPERTY 이고 공개 API 는 const 게터뿐이라 리플렉션으로 가변 접근한다. */
	FInstancedPropertyBag* GetMutableRootParameterBag(UStateTreeEditorData& EditorData)
	{
		const FStructProperty* BagProperty = FindFProperty<FStructProperty>(UStateTreeEditorData::StaticClass(), TEXT("RootParameterPropertyBag"));
		if (!BagProperty)
		{
			UKismetSystemLibrary::RaiseScriptError(TEXT("RootParameterPropertyBag 프로퍼티를 찾지 못했다. 엔진 리네이밍 의심."));
			return nullptr;
		}
		return BagProperty->ContainerPtrToValuePtr<FInstancedPropertyBag>(&EditorData);
	}

	TSharedPtr<FJsonObject> ParseJsonObject(const FString& Json)
	{
		TSharedPtr<FJsonObject> JsonObject;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
		if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("JSON 파싱 실패: %s"), *Json));
			return nullptr;
		}
		return JsonObject;
	}

	/**
	 * 백의 지정 파라미터에 JSON 값을 기입하고 그 서술자를 돌려준다. 실패 시 스크립트 에러 후 null.
	 * UOL 처럼 ImportTextItem 을 가진 구조체는 JSON 문자열 리터럴로 들어와도 FJsonObjectConverter 가 ImportText 로 처리한다.
	 */
	const FPropertyBagPropertyDesc* SetBagValueFromJson(FInstancedPropertyBag& Bag, const FString& Name, const TSharedPtr<FJsonValue>& JsonValue)
	{
		const FPropertyBagPropertyDesc* Desc = Bag.FindPropertyDescByName(FName(*Name));
		if (!Desc || !Desc->CachedProperty)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("파라미터 '%s' 가 백에 없다."), *Name));
			return nullptr;
		}

		FProperty* Property = const_cast<FProperty*>(Desc->CachedProperty);
		void* ValueAddress = Property->ContainerPtrToValuePtr<void>(Bag.GetMutableValue().GetMemory());
		FText FailReason;
		if (!FJsonObjectConverter::JsonValueToUProperty(JsonValue, Property, ValueAddress, 0, 0, false, &FailReason))
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("파라미터 '%s' 값 기입 실패: %s"), *Name, *FailReason.ToString()));
			return nullptr;
		}
		return Desc;
	}

	/** MetaJson({"키":"값"}) 을 서술자에 얹는다. 빈 문자열은 아무것도 하지 않는다. 실패 시 스크립트 에러 후 false. */
	bool ApplyMetaJson(FPropertyBagPropertyDesc& Desc, const FString& MetaJson)
	{
		if (MetaJson.IsEmpty())
		{
			return true;
		}

		const TSharedPtr<FJsonObject> MetaObject = ParseJsonObject(MetaJson);
		if (!MetaObject)
		{
			return false;
		}

		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : MetaObject->Values)
		{
			FString MetaValue;
			if (!Pair.Value->TryGetString(MetaValue))
			{
				UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("메타 '%s' 값은 문자열이어야 한다."), *Pair.Key));
				return false;
			}
			Desc.SetMetaData(FName(*Pair.Key), MetaValue);
		}
		return true;
	}

	FString SerializeJson(const TSharedRef<FJsonObject>& JsonObject)
	{
		FString Result;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Result);
		FJsonSerializer::Serialize(JsonObject, Writer);
		return Result;
	}

	/** 오브젝트의 FStateTreeReference UPROPERTY 를 찾는다. 실패 시 스크립트 에러를 올리고 null 을 돌려준다. */
	FStateTreeReference* GetMutableReference(UObject* Object, const FName PropertyName)
	{
		if (!Object)
		{
			UKismetSystemLibrary::RaiseScriptError(TEXT("Object 가 null 이다."));
			return nullptr;
		}

		const FStructProperty* ReferenceProperty = FindFProperty<FStructProperty>(Object->GetClass(), PropertyName);
		if (!ReferenceProperty || ReferenceProperty->Struct != FStateTreeReference::StaticStruct())
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("'%s' 에서 FStateTreeReference 프로퍼티 '%s' 를 찾지 못했다."), *Object->GetPathName(), *PropertyName.ToString()));
			return nullptr;
		}
		return ReferenceProperty->ContainerPtrToValuePtr<FStateTreeReference>(Object);
	}

	/** 컴파일러 로그의 메시지 목록이 protected 라 파생으로 읽기 접근만 연다. */
	struct FWxStateTreeCompilerLogReader : public FStateTreeCompilerLog
	{
		using FStateTreeCompilerLog::Messages;
	};

	/** GUID 문자열과 경로 문자열로 바인딩 경로를 만든다. 실패 시 스크립트 에러와 함께 false. */
	bool MakeBindingPath(const FString& StructId, const FString& Path, FPropertyBindingPath& OutPath)
	{
		FGuid Guid;
		if (!FGuid::Parse(StructId, Guid))
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("GUID 파싱 실패: %s"), *StructId));
			return false;
		}

		OutPath.SetStructID(Guid);
		if (!OutPath.FromString(Path))
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("프로퍼티 경로 파싱 실패: %s"), *Path));
			return false;
		}
		return true;
	}
}

UStateTree* UWxStateTreeToolset::CreateStateTree(const FString& PackagePath, const FString& AssetName)
{
	if (PackagePath.IsEmpty() || AssetName.IsEmpty())
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("PackagePath 와 AssetName 은 비울 수 없다."));
		return nullptr;
	}

	UStateTreeFactory* Factory = NewObject<UStateTreeFactory>();
	Factory->SetSchemaClass(UStateTreeComponentSchema::StaticClass());

	UObject* NewAsset = FAssetToolsModule::GetModule().Get().CreateAsset(AssetName, PackagePath, UStateTree::StaticClass(), Factory);
	if (!NewAsset)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("에셋 생성 실패: %s/%s"), *PackagePath, *AssetName));
		return nullptr;
	}
	return Cast<UStateTree>(NewAsset);
}

FString UWxStateTreeToolset::GetRootParameters(UStateTree* StateTree)
{
	UStateTreeEditorData* EditorData = GetEditorData(StateTree);
	if (!EditorData)
	{
		return FString();
	}

	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("rootParametersId"), EditorData->GetRootParametersGuid().ToString(EGuidFormats::DigitsWithHyphens));

	TArray<TSharedPtr<FJsonValue>> Parameters;
	if (const UPropertyBag* BagStruct = EditorData->GetRootParametersPropertyBag().GetPropertyBagStruct())
	{
		for (const FPropertyBagPropertyDesc& Desc : BagStruct->GetPropertyDescs())
		{
			TSharedRef<FJsonObject> Parameter = MakeShared<FJsonObject>();
			Parameter->SetStringField(TEXT("name"), Desc.Name.ToString());
			Parameter->SetStringField(TEXT("id"), Desc.ID.ToString(EGuidFormats::DigitsWithHyphens));
			Parameter->SetStringField(TEXT("type"), StaticEnum<EPropertyBagPropertyType>()->GetNameStringByValue(static_cast<int64>(Desc.ValueType)));
			Parameter->SetStringField(TEXT("container"), StaticEnum<EPropertyBagContainerType>()->GetNameStringByValue(static_cast<int64>(Desc.ContainerTypes.GetFirstContainerType())));
			Parameter->SetStringField(TEXT("valueTypeObject"), Desc.ValueTypeObject ? Desc.ValueTypeObject->GetPathName() : FString());
			Parameters.Add(MakeShared<FJsonValueObject>(Parameter));
		}
	}
	Root->SetArrayField(TEXT("parameters"), Parameters);

	return SerializeJson(Root);
}

FString UWxStateTreeToolset::AddRootParameter(UStateTree* StateTree, FName Name, const FString& Type, const FString& ValueTypePath, bool bArray, const FString& MetaJson)
{
	UStateTreeEditorData* EditorData = GetEditorData(StateTree);
	if (!EditorData)
	{
		return FString();
	}

	FInstancedPropertyBag* Bag = GetMutableRootParameterBag(*EditorData);
	if (!Bag)
	{
		return FString();
	}

	const int64 TypeValue = StaticEnum<EPropertyBagPropertyType>()->GetValueByNameString(Type);
	if (TypeValue == INDEX_NONE)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("알 수 없는 파라미터 타입: %s"), *Type));
		return FString();
	}
	const EPropertyBagPropertyType ValueType = static_cast<EPropertyBagPropertyType>(TypeValue);

	// 값 타입 오브젝트가 필요한 타입은 경로 로드에 실패하면 여기서 끊는다 — 백에 타입 불명 파라미터가 생기는 것을 막는다.
	const UObject* ValueTypeObject = nullptr;
	if (ValueType == EPropertyBagPropertyType::Struct)
	{
		ValueTypeObject = LoadObject<UScriptStruct>(nullptr, *ValueTypePath);
	}
	else if (ValueType == EPropertyBagPropertyType::Object || ValueType == EPropertyBagPropertyType::SoftObject
		|| ValueType == EPropertyBagPropertyType::Class || ValueType == EPropertyBagPropertyType::SoftClass)
	{
		ValueTypeObject = LoadObject<UClass>(nullptr, *ValueTypePath);
	}
	else if (!ValueTypePath.IsEmpty())
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("타입 %s 은 값 타입 오브젝트를 받지 않는다: %s"), *Type, *ValueTypePath));
		return FString();
	}
	if (!ValueTypePath.IsEmpty() && !ValueTypeObject)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("값 타입 로드 실패: %s"), *ValueTypePath));
		return FString();
	}

	// 메타는 서술자 단위로 실려 백 생성 시 FProperty 에 전파된다 — AddProperty 계열엔 메타 인자가 없어 서술자 직접 구성으로 통일한다.
	FPropertyBagPropertyDesc Desc = bArray
		? FPropertyBagPropertyDesc(Name, EPropertyBagContainerType::Array, ValueType, ValueTypeObject)
		: FPropertyBagPropertyDesc(Name, ValueType, ValueTypeObject);
	if (!ApplyMetaJson(Desc, MetaJson))
	{
		return FString();
	}

	EditorData->Modify();
	const EPropertyBagAlterationResult Result = Bag->AddProperties({Desc});
	if (Result != EPropertyBagAlterationResult::Success)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("파라미터 '%s' 추가 실패(결과 코드 %d)."), *Name.ToString(), static_cast<int32>(Result)));
		return FString();
	}

	UStateTreeEditingSubsystem::MarkAsModified(StateTree);

	const FPropertyBagPropertyDesc* NewDesc = Bag->FindPropertyDescByName(Name);
	return NewDesc ? NewDesc->ID.ToString(EGuidFormats::DigitsWithHyphens) : FString();
}

bool UWxStateTreeToolset::SetRootParameterMeta(UStateTree* StateTree, FName Name, const FString& MetaJson)
{
	UStateTreeEditorData* EditorData = GetEditorData(StateTree);
	if (!EditorData)
	{
		return false;
	}

	FInstancedPropertyBag* Bag = GetMutableRootParameterBag(*EditorData);
	if (!Bag)
	{
		return false;
	}

	const FPropertyBagPropertyDesc* ExistingDesc = Bag->FindPropertyDescByName(Name);
	if (!ExistingDesc)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("파라미터 '%s' 가 백에 없다."), *Name.ToString()));
		return false;
	}

	FPropertyBagPropertyDesc Desc = *ExistingDesc;
	Desc.MetaData.Reset();
	if (!ApplyMetaJson(Desc, MetaJson))
	{
		return false;
	}

	EditorData->Modify();

	// 덮어쓰기 경로가 기존 ID 를 보존하므로 바인딩이 유지되고, 백 교체 시 값도 ID 매칭으로 따라온다.
	const EPropertyBagAlterationResult Result = Bag->AddProperties({Desc}, /*bOverwrite*/ true);
	if (Result != EPropertyBagAlterationResult::Success)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("파라미터 '%s' 메타 기입 실패(결과 코드 %d)."), *Name.ToString(), static_cast<int32>(Result)));
		return false;
	}

	UStateTreeEditingSubsystem::MarkAsModified(StateTree);
	return true;
}

bool UWxStateTreeToolset::RemoveRootParameter(UStateTree* StateTree, FName Name)
{
	UStateTreeEditorData* EditorData = GetEditorData(StateTree);
	if (!EditorData)
	{
		return false;
	}

	FInstancedPropertyBag* Bag = GetMutableRootParameterBag(*EditorData);
	if (!Bag)
	{
		return false;
	}

	if (!Bag->FindPropertyDescByName(Name))
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("파라미터 '%s' 가 백에 없다."), *Name.ToString()));
		return false;
	}

	EditorData->Modify();
	const EPropertyBagAlterationResult Result = Bag->RemovePropertyByName(Name);
	if (Result != EPropertyBagAlterationResult::Success)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("파라미터 '%s' 제거 실패(결과 코드 %d)."), *Name.ToString(), static_cast<int32>(Result)));
		return false;
	}

	UStateTreeEditingSubsystem::MarkAsModified(StateTree);
	return true;
}

bool UWxStateTreeToolset::SetRootParameterValues(UStateTree* StateTree, const FString& ValuesJson)
{
	UStateTreeEditorData* EditorData = GetEditorData(StateTree);
	if (!EditorData)
	{
		return false;
	}

	FInstancedPropertyBag* Bag = GetMutableRootParameterBag(*EditorData);
	const TSharedPtr<FJsonObject> Values = ParseJsonObject(ValuesJson);
	if (!Bag || !Values)
	{
		return false;
	}

	EditorData->Modify();
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Values->Values)
	{
		if (!SetBagValueFromJson(*Bag, Pair.Key, Pair.Value))
		{
			return false;
		}
	}

	UStateTreeEditingSubsystem::MarkAsModified(StateTree);
	return true;
}

bool UWxStateTreeToolset::AddBinding(UStateTree* StateTree, const FString& SourceStructId, const FString& SourcePath, const FString& TargetStructId, const FString& TargetPath)
{
	UStateTreeEditorData* EditorData = GetEditorData(StateTree);
	if (!EditorData)
	{
		return false;
	}

	FPropertyBindingPath Source;
	FPropertyBindingPath Target;
	if (!MakeBindingPath(SourceStructId, SourcePath, Source) || !MakeBindingPath(TargetStructId, TargetPath, Target))
	{
		return false;
	}

	EditorData->Modify();
	EditorData->AddPropertyBinding(Source, Target);
	UStateTreeEditingSubsystem::MarkAsModified(StateTree);
	return true;
}

bool UWxStateTreeToolset::RemoveBinding(UStateTree* StateTree, const FString& TargetStructId, const FString& TargetPath)
{
	UStateTreeEditorData* EditorData = GetEditorData(StateTree);
	if (!EditorData)
	{
		return false;
	}

	FPropertyBindingPath Target;
	if (!MakeBindingPath(TargetStructId, TargetPath, Target))
	{
		return false;
	}

	EditorData->Modify();
	EditorData->RemovePropertyBinding(Target);
	UStateTreeEditingSubsystem::MarkAsModified(StateTree);
	return true;
}

FString UWxStateTreeToolset::GetBindings(UStateTree* StateTree)
{
	UStateTreeEditorData* EditorData = GetEditorData(StateTree);
	if (!EditorData)
	{
		return FString();
	}

	TArray<TSharedPtr<FJsonValue>> Entries;
	// 바인딩 컬렉션 순회가 TFunctionRef 를 요구해 람다가 불가피하다.
	EditorData->GetPropertyEditorBindings()->ForEachBinding([&Entries](const FPropertyBindingBinding& Binding)
	{
		TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("sourceId"), Binding.GetSourcePath().GetStructID().ToString(EGuidFormats::DigitsWithHyphens));
		Entry->SetStringField(TEXT("sourcePath"), Binding.GetSourcePath().ToString());
		Entry->SetStringField(TEXT("targetId"), Binding.GetTargetPath().GetStructID().ToString(EGuidFormats::DigitsWithHyphens));
		Entry->SetStringField(TEXT("targetPath"), Binding.GetTargetPath().ToString());
		Entries.Add(MakeShared<FJsonValueObject>(Entry));
	});

	FString Result;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Result);
	FJsonSerializer::Serialize(Entries, Writer);
	return Result;
}

bool UWxStateTreeToolset::LinkStateToAsset(UStateTreeState* State, UStateTree* LinkedAsset)
{
	if (!State || !LinkedAsset)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("State 와 LinkedAsset 은 null 일 수 없다."));
		return false;
	}

	State->Modify();
	State->Type = EStateTreeStateType::LinkedAsset;
	State->SetLinkedStateAsset(LinkedAsset);
	return true;
}

bool UWxStateTreeToolset::SetStateParameterValues(UStateTreeState* State, const FString& ValuesJson)
{
	if (!State)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("State 가 null 이다."));
		return false;
	}

	const TSharedPtr<FJsonObject> Values = ParseJsonObject(ValuesJson);
	if (!Values)
	{
		return false;
	}

	State->Modify();
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Values->Values)
	{
		const FPropertyBagPropertyDesc* Desc = SetBagValueFromJson(State->Parameters.Parameters, Pair.Key, Pair.Value);
		if (!Desc)
		{
			return false;
		}
		State->Parameters.PropertyOverrides.AddUnique(Desc->ID);
	}

	if (UStateTree* StateTree = State->GetTypedOuter<UStateTree>())
	{
		UStateTreeEditingSubsystem::MarkAsModified(StateTree);
	}
	return true;
}

bool UWxStateTreeToolset::ClearStateParameterOverride(UStateTreeState* State, FName Name)
{
	if (!State)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("State 가 null 이다."));
		return false;
	}

	const FPropertyBagPropertyDesc* Desc = State->Parameters.Parameters.FindPropertyDescByName(Name);
	if (!Desc)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("파라미터 '%s' 가 상태에 없다."), *Name.ToString()));
		return false;
	}

	State->Modify();
	State->Parameters.PropertyOverrides.Remove(Desc->ID);
	// 오버라이드가 풀린 값은 링크 재동기화가 링크 에셋 기본값으로 되돌린다.
	State->UpdateParametersFromLinkedSubtree();

	if (UStateTree* StateTree = State->GetTypedOuter<UStateTree>())
	{
		UStateTreeEditingSubsystem::MarkAsModified(StateTree);
	}
	return true;
}

bool UWxStateTreeToolset::SetReferenceStateTree(UObject* Object, FName PropertyName, UStateTree* StateTree)
{
	FStateTreeReference* Reference = GetMutableReference(Object, PropertyName);
	if (!Reference)
	{
		return false;
	}

	Object->Modify();
	// SetStateTree 가 파라미터 레이아웃을 에셋 기준으로 동기화한다.
	Reference->SetStateTree(StateTree);
	return true;
}

bool UWxStateTreeToolset::SetReferenceParameterValues(UObject* Object, FName PropertyName, const FString& ValuesJson)
{
	FStateTreeReference* Reference = GetMutableReference(Object, PropertyName);
	if (!Reference)
	{
		return false;
	}

	const TSharedPtr<FJsonObject> Values = ParseJsonObject(ValuesJson);
	if (!Values)
	{
		return false;
	}

	Object->Modify();
	Reference->SyncParameters();
	FInstancedPropertyBag& Bag = Reference->GetMutableParameters();
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Values->Values)
	{
		const FPropertyBagPropertyDesc* Desc = SetBagValueFromJson(Bag, Pair.Key, Pair.Value);
		if (!Desc)
		{
			return false;
		}
		// 오버라이드 마킹이 없으면 이후 레이아웃 동기화가 값을 에셋 기본값으로 되돌린다.
		Reference->SetPropertyOverridden(Desc->ID, true);
	}
	return true;
}

FString UWxStateTreeToolset::CompileStateTree(UStateTree* StateTree)
{
	if (!StateTree)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("StateTree 가 null 이다."));
		return FString();
	}

	FWxStateTreeCompilerLogReader Log;
	const bool bSuccess = UStateTreeEditingSubsystem::CompileStateTree(StateTree, Log);

	TStringBuilder<1024> Result;
	Result << (bSuccess ? TEXT("succeeded") : TEXT("failed"));
	for (const FStateTreeCompilerLogMessage& Message : Log.Messages)
	{
		Result << TEXT("\n");
		switch (Message.Severity)
		{
		case EMessageSeverity::Error:
			Result << TEXT("[Error] ");
			break;
		case EMessageSeverity::Warning:
			Result << TEXT("[Warning] ");
			break;
		default:
			Result << TEXT("[Info] ");
			break;
		}
		if (Message.State)
		{
			Result << Message.State->Name << TEXT(": ");
		}
		if (!Message.Item.Name.IsNone())
		{
			Result << Message.Item.Name << TEXT(": ");
		}
		Result << Message.Message;
	}
	return FString(Result);
}

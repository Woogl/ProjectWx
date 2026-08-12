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

	FString SerializeJson(const TSharedRef<FJsonObject>& JsonObject)
	{
		FString Result;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Result);
		FJsonSerializer::Serialize(JsonObject, Writer);
		return Result;
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

FString UWxStateTreeToolset::AddRootParameter(UStateTree* StateTree, FName Name, const FString& Type, const FString& StructPath, bool bArray)
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

	const UScriptStruct* ValueStruct = nullptr;
	if (ValueType == EPropertyBagPropertyType::Struct)
	{
		ValueStruct = LoadObject<UScriptStruct>(nullptr, *StructPath);
		if (!ValueStruct)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("값 구조체 로드 실패: %s"), *StructPath));
			return FString();
		}
	}

	EditorData->Modify();
	const EPropertyBagAlterationResult Result = bArray
		? Bag->AddContainerProperty(Name, EPropertyBagContainerType::Array, ValueType, ValueStruct)
		: Bag->AddProperty(Name, ValueType, ValueStruct);
	if (Result != EPropertyBagAlterationResult::Success)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("파라미터 '%s' 추가 실패(결과 코드 %d)."), *Name.ToString(), static_cast<int32>(Result)));
		return FString();
	}

	UStateTreeEditingSubsystem::MarkAsModified(StateTree);

	const FPropertyBagPropertyDesc* NewDesc = Bag->FindPropertyDescByName(Name);
	return NewDesc ? NewDesc->ID.ToString(EGuidFormats::DigitsWithHyphens) : FString();
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

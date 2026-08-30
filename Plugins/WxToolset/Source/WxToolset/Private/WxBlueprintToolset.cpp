// Copyright Woogle. All Rights Reserved.

#include "WxBlueprintToolset.h"

#include "Dom/JsonObject.h"
#include "Engine/Blueprint.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	/** 이 블루프린트가 직접 선언한 변수의 서술자를 찾는다. */
	const FBPVariableDescription* FindVariable(const UBlueprint* Blueprint, const FName VarName)
	{
		if (!Blueprint)
		{
			UKismetSystemLibrary::RaiseScriptError(TEXT("Blueprint 가 null 이다."));
			return nullptr;
		}

		const int32 Index = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VarName);
		if (Index == INDEX_NONE)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("'%s' 가 선언한 변수 중에 '%s' 가 없다. 물려받은 변수는 선언한 쪽에서 고친다."), *Blueprint->GetPathName(), *VarName.ToString()));
			return nullptr;
		}
		return &Blueprint->NewVariables[Index];
	}
}

bool UWxBlueprintToolset::SetVariableMeta(UBlueprint* Blueprint, FName VarName, const FString& MetaJson)
{
	if (!FindVariable(Blueprint, VarName))
	{
		return false;
	}

	TSharedPtr<FJsonObject> MetaObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(MetaJson);
	if (!FJsonSerializer::Deserialize(Reader, MetaObject) || !MetaObject.IsValid())
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("JSON 파싱 실패: %s"), *MetaJson));
		return false;
	}

	// 기입은 키마다 통지·컴파일 마킹을 일으키므로, 중간에 끊겨 절반만 들어가지 않게 전부 검증한 뒤 시작한다.
	TArray<TPair<FName, FString>> Entries;
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : MetaObject->Values)
	{
		FString MetaValue;
		if (!Pair.Value->TryGetString(MetaValue))
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("메타 '%s' 값은 문자열이어야 한다."), *Pair.Key));
			return false;
		}
		Entries.Emplace(FName(*Pair.Key), MetaValue);
	}

	for (const TPair<FName, FString>& Entry : Entries)
	{
		FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, VarName, nullptr, Entry.Key, Entry.Value);
	}
	return true;
}

FString UWxBlueprintToolset::GetVariableMeta(UBlueprint* Blueprint, FName VarName)
{
	const FBPVariableDescription* Variable = FindVariable(Blueprint, VarName);
	if (!Variable)
	{
		return FString();
	}

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	for (const FBPVariableMetaDataEntry& Entry : Variable->MetaDataArray)
	{
		Root->SetStringField(Entry.DataKey.ToString(), Entry.DataValue);
	}

	FString Result;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Result);
	FJsonSerializer::Serialize(Root, Writer);
	return Result;
}

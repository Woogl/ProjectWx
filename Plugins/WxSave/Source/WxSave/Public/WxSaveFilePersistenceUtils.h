// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SubclassOf.h"
#include "WxSaveFilePersistenceUtils.generated.h"

class UWxPersistenceSaveGame;

/**
 * WxSave 의 BP 진입점 (샘플 USaveFilePersistenceUtils 골격 이식). UWxPersistenceGameSubsystem 의 공개 API 를 정적 래퍼로 노출한다.
 */
UCLASS()
class WXSAVE_API UWxSaveFilePersistenceUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** SpecificClass 의 새 SaveGame 을 만들어 활성 슬롯으로 등록하고 반환한다. SlotName 이 비면 기본 슬롯 이름을 쓴다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static UWxPersistenceSaveGame* StartNewSaveFile(const UObject* WorldContextObject, const FString& SlotName, int32 UserIndex, TSubclassOf<UWxPersistenceSaveGame> SpecificClass);

	/** 슬롯 파일을 메모리로 로드하고 저장된 맵으로 트래블한다(부재 시 빈 슬롯 리셋 후에도 트래블 — 사망 리스폰 경로). 반환은 활성 SaveGame. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static UWxPersistenceSaveGame* LoadFromFile(const UObject* WorldContextObject, const FString& SlotName, int32 UserIndex);

	/** 활성 SaveGame 의 슬롯을 다시 로드해 저장 안 된 변경을 버린다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static UWxPersistenceSaveGame* ReloadFromFile(const UObject* WorldContextObject, bool bStartTravel = true);

	/** 라이브 상태를 활성 SaveGame 에 플러시하고 그 슬롯에 디스크 기록한다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static void SaveToFile(const UObject* WorldContextObject);

	/** 활성 SaveGame 의 트래블 데이터에 저장된 맵으로 트래블한다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static void TravelFromSaveFile(const UObject* WorldContextObject);

	/** 개발 기본 슬롯 이름("Test"). UI BP 가 하드코딩 문자열 대신 사용한다. */
	UFUNCTION(BlueprintPure, Category = "Wx|Save")
	static FString GetDefaultSlotName();
};

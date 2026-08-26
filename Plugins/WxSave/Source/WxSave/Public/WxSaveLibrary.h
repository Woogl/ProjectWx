// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SubclassOf.h"
#include "WxSaveLibrary.generated.h"

class UWxSaveGame;

/**
 * WxSave 의 BP 진입점 (샘플 USaveFilePersistenceUtils 골격 이식). UWxSaveGameSubsystem 의 공개 API 를 정적 래퍼로 노출한다.
 */
UCLASS()
class WXSAVE_API UWxSaveLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 활성 SaveGame 이 없으면 빈 문자열. */
	UFUNCTION(BlueprintPure, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static FString GetCurrentSaveSlotName(const UObject* WorldContextObject);

	/** SpecificClass 의 새 SaveGame 을 만들어 활성 슬롯으로 등록하고 반환한다. SlotName 은 그대로 슬롯 정체성이 되므로 유효한 이름을 넘겨야 한다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static UWxSaveGame* StartNewSaveFile(const UObject* WorldContextObject, const FString& SlotName, int32 UserIndex, TSubclassOf<UWxSaveGame> SpecificClass);

	/**
	 * 슬롯 파일을 메모리로 로드하고 저장된 맵으로 트래블한다(부재 시 빈 슬롯 리셋 후에도 트래블 — 사망 리스폰 경로). 반환은 활성 SaveGame.
	 * SlotName 이 비면 활성 슬롯을 다시 읽는다(저장 안 된 변경을 버리고 마지막 세이브로 재개 — 사망 화면 경로). SaveToFile 의 빈 슬롯 규칙과 대칭이다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static UWxSaveGame* LoadFromFile(const UObject* WorldContextObject, const FString& SlotName, int32 UserIndex);

	/** 라이브 상태를 활성 SaveGame 에 플러시하고 디스크 기록한다. SlotName 이 비면 활성 슬롯에, 지정되면 활성 슬롯을 그 이름으로 재지정한 뒤 기록한다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static void SaveToFile(const UObject* WorldContextObject, const FString& SlotName, int32 UserIndex);

	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static void TravelFromSaveFile(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static bool DoesSaveFileExist(const UObject* WorldContextObject, const FString& SlotName, int32 UserIndex);

	/** 디스크의 슬롯 파일만 지우고 인메모리 활성 SaveGame 은 건드리지 않는다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static bool DeleteSaveFile(const UObject* WorldContextObject, const FString& SlotName, int32 UserIndex);
};

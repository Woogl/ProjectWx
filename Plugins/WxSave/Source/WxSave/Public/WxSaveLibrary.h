// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxSaveLibrary.generated.h"

class APlayerController;

/**
 * Blueprint 진입점. UWxSaveSubsystem 으로 위임하는 thin wrapper.
 *
 * 서버 권위 호출만 의미가 있다. 클라에서 호출되면 서브시스템이 무시한다.
 * 슬롯명은 UWxSaveDeveloperSettings::PlayerSlotName 에서 조회한다.
 */
UCLASS()
class WXSAVE_API UWxSaveLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** PC 빙의 Pawn Transform + 월드의 모든 IWxSavable 액터 상태를 슬롯에 비동기 기록. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (DefaultToSelf = "PlayerController"))
	static void SaveGame(APlayerController* PlayerController);

	/**
	 * 슬롯에 저장된 위치로 PC 를 재스폰. 기존 Pawn 폐기 후 슬롯 Transform 에 새 Pawn 스폰.
	 * 슬롯이 없으면 동작하지 않고 false 반환 (신규 세션은 GameMode 기본 흐름이 처리).
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (DefaultToSelf = "PlayerController"))
	static bool RestartFromSave(APlayerController* PlayerController);

	/** 슬롯에 유효한 위치가 저장되어 있는지 확인. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static bool HasSavedGame(const UObject* WorldContextObject);

	/** 슬롯 세이브 파일을 삭제. 슬롯이 없으면 false. 메모리 상태도 빈 슬롯으로 리셋. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static bool DeleteSavedGame(const UObject* WorldContextObject);

	/**
	 * "New Game" 메뉴에서 호출. 적용된 슬롯 상태를 리셋해 PlayerStart 흐름으로 신규 세션을 시작할 수 있게 한다.
	 * 슬롯 파일은 보존되며 다음 SaveGame 호출이 덮어쓴다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static void BeginNewGame(const UObject* WorldContextObject);
};

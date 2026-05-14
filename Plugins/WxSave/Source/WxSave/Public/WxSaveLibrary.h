// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxSaveLibrary.generated.h"

class APlayerController;
class UWxPlayerSave;

/**
 * 플레이어 위치의 저장/복원을 Blueprint 에서 명시적으로 호출하기 위한 진입점.
 * UWxPlayerSave 슬롯 IO 를 캡슐화한다. 서버 권위. 클라이언트에서 호출되면 무시된다.
 *
 * 슬롯명은 UWxSaveDeveloperSettings::PlayerSlotName 에서 조회한다.
 */
UCLASS()
class WXSAVE_API UWxSaveLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** PC 가 빙의한 Pawn 의 현재 Transform 을 슬롯에 비동기 기록한다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save")
	static void SavePlayerLocation(APlayerController* PlayerController);

	/**
	 * PC 의 기존 Pawn 을 폐기하고 새 Pawn 을 스폰한다.
	 * 슬롯에 저장된 위치가 있으면 그 Transform 에서, 없으면 기본 PlayerStart 에서 스폰한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save")
	static void RestartPlayer(APlayerController* PlayerController);

	/** 슬롯에 유효한 위치가 저장되어 있는지 확인한다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save")
	static bool HasSavedPlayerLocation();

	/** 슬롯 세이브 파일을 삭제한다. 슬롯이 없으면 무시. 성공 여부 반환. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save")
	static bool DeletePlayerSave();

private:
	static FString GetSlotName();
	static UWxPlayerSave* LoadOrCreateSlot(const FString& SlotName);
};

// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxGameSaveLibrary.generated.h"

/**
 * Blueprint 에서 게임 세이브 데이터를 갱신/조회하기 위한 진입점.
 * 실제 구현은 UWxGameSaveSubsystem 으로 위임한다.
 */
UCLASS()
class UWxGameSaveLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 마지막 체크포인트 Transform 을 SlotData 에 기록하고 슬롯을 비동기 저장한다.
	 * 호스트(서버) 머신에서만 효과를 가진다. 클라이언트에서 호출되면 무시된다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static void SetLastCheckpoint(const UObject* WorldContextObject, const FTransform& Transform);

	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static bool HasValidCheckpoint(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static FTransform GetLastCheckpoint(const UObject* WorldContextObject);

	/**
	 * 로컬 플레이어 컨트롤러의 기존 Pawn 을 폐기하고 새 Pawn 을 스폰한다.
	 * 마지막 체크포인트가 기록되어 있으면 그 Transform 에서, 아니면 기본 PlayerStart 에서 스폰한다.
	 * 호스트(서버) 머신에서만 효과를 가진다. 클라이언트에서 호출되면 무시된다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static void RestartLocalPlayerAtLastCheckpoint(const UObject* WorldContextObject);
};

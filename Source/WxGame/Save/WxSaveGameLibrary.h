// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxSaveGameLibrary.generated.h"

/**
 * Blueprint 에서 게임 세이브 데이터를 갱신/조회하기 위한 진입점.
 * UGameplayStatics 의 SaveGame API 를 직접 호출하는 표준 패턴.
 */
UCLASS()
class UWxSaveGameLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 마지막 체크포인트 Transform 을 슬롯에 기록한다.
	 * 기존 슬롯 데이터를 동기 load 한 뒤 갱신해 AsyncSaveGameToSlot 으로 비동기 저장한다.
	 * 호스트(서버) 머신에서만 효과를 가진다. 클라이언트에서 호출되면 무시된다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static void SaveLastCheckpoint(const UObject* WorldContextObject, const FTransform& Transform);

	/** 슬롯의 마지막 체크포인트 Transform 을 동기 load 해 반환한다. 미기록 시 FTransform::Identity. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static FTransform GetLastCheckpoint(const UObject* WorldContextObject);

	/** 슬롯에 체크포인트가 기록되어 있는지 동기 load 해 확인한다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static bool HasValidCheckpoint(const UObject* WorldContextObject);

	/**
	 * 로컬 플레이어 컨트롤러의 기존 Pawn 을 폐기하고 새 Pawn 을 스폰한다.
	 * 마지막 체크포인트가 기록되어 있으면 그 Transform 에서, 아니면 기본 PlayerStart 에서 스폰한다.
	 * 부활 직후 UWxSpawnerSubsystem::RespawnAll 을 호출해 처치된 모든 Spawner 를 부활시킨다.
	 * 호스트(서버) 머신에서만 효과를 가진다. 클라이언트에서 호출되면 무시된다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static void RestartPlayerAtLastCheckpoint(const UObject* WorldContextObject, int32 PlayerIndex);
};

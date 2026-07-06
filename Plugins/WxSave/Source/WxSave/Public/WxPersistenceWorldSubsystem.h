// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Subsystems/WorldSubsystem.h"
#include "WxPersistenceWorldSubsystem.generated.h"

class ULevel;
class UWxPersistenceSaveGame;

/**
 * 월드 단위 플러시/복원 오케스트레이션을 담당하는 World 서브시스템 (샘플 UPersistenceWorldSubsystem 골격 이식).
 * 메모리 SaveGame 의 소유는 UWxPersistenceGameSubsystem 이고, 이 서브시스템은 월드 수명 이벤트에 맞춰 그 SaveGame 을 읽고 쓴다.
 *
 * 자동 처리:
 *  - 영구 레벨 초기화 (OnWorldInitializedActors) / 스트리밍-인 (LevelAddedToWorld): IWxSavable 액터에 레코드 자동 복원.
 *  - 스트리밍-아웃 (LevelRemovedFromWorld): 셀의 IWxSavable 상태를 메모리에 자동 캡처해 셀 왕복으로 인한 손실 방지.
 *  - 맵 이탈 (OnWorldBeginTearDown): 현재 월드 IWxSavable 전체를 메모리에 플러시해 같은 세션 맵 왕복 상태 유지(디스크 기록 없음).
 *  - IsTravelingFromSaveFile 동안 위 자동 캡처를 전부 스킵해, 막 로드한 세이브가 라이브 상태로 오염되는 것을 방지.
 *  - OnWorldBeginPlay 에서 트래블 완료를 게임 서브시스템에 보고해 가드를 해제한다.
 */
UCLASS()
class WXSAVE_API UWxPersistenceWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** RequestSaveFlush 완료 통지. 현재 플러시가 전부 동기라 즉시 발화되지만, 비동기 작업이 생길 때 지연 완료로 되돌릴 seam 으로 시그니처를 유지한다(샘플 동일 형태). */
	DECLARE_MULTICAST_DELEGATE(FOnSaveFlushComplete);

	/**
	 * 디스크 기록 전, 라이브 상태를 SaveGame 에 플러시한다: 맵 트래블 데이터(teardown 중엔 스킵 — 맵 전환을 일으킨 게임 코드가 다음 시작 지점의 소유자) + IWxSavable 액터 전체.
	 * OnComplete 는 플러시 완료 후 발화한다(현재 동기라 반환 전 즉시).
	 */
	void RequestSaveFlush(FOnSaveFlushComplete::FDelegate OnComplete);

	//~ Begin UWorldSubsystem
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	//~ End UWorldSubsystem

private:
	/** 현재 맵·폰 트랜스폼·컨트롤 로테이션을 캡처해 게임 서브시스템의 TravelData 로 푸시한다. */
	void FlushMapTravelData();

	/** 현재 월드의 IWxSavable 액터 전체를 SaveGame 레코드로 캡처한다. */
	void FlushSavableActors();

	/** 액터+컴포넌트의 UPROPERTY(SaveGame) 를 레코드로 직렬화하고 버전 헤더를 갱신한다. */
	void CaptureActor(UWxPersistenceSaveGame& SaveGame, AActor* Actor);

	/** @return 슬롯에서 일치 레코드를 찾아 복원했으면 true (신규 세션 등 레코드 없음/미설정 키는 false). */
	bool RestoreActor(const UWxPersistenceSaveGame& SaveGame, AActor* Actor);

	/** OnWorldInitializedActors 핸들러: 영구 레벨 + 초기 WP 셀 액터에 레코드 자동 복원. */
	void HandleWorldInitializedActors(const UWorld::FActorsInitializedParams& Params);

	/** LevelAddedToWorld 핸들러: 스트리밍-인 셀(World Partition 포함) 액터에 레코드 자동 복원. */
	void HandleLevelAddedToWorld(ULevel* Level, UWorld* World);

	/** LevelRemovedFromWorld 핸들러: 스트리밍-아웃 직전 셀 액터 상태를 메모리에 자동 캡처. 로드 트래블 중엔 스킵. */
	void HandleLevelRemovedFromWorld(ULevel* Level, UWorld* World);

	/** OnWorldBeginTearDown 핸들러: 맵 이탈 시 IWxSavable 전체를 메모리에 플러시(디스크 기록 없음). 로드 트래블 중엔 스킵. */
	void HandleWorldBeginTearDown(UWorld* World);

	FDelegateHandle WorldInitializedActorsHandle;

	FDelegateHandle LevelAddedHandle;

	FDelegateHandle LevelRemovedHandle;

	FDelegateHandle WorldBeginTearDownHandle;
};

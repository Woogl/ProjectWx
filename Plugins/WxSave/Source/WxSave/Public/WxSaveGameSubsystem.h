// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WxSaveGameSubsystem.generated.h"

class ULevel;
class UWxSaveGame;

/**
 * WxSave 의 단일 진입점.
 *
 * 공개 API:
 *  - SaveSlot: 현재 월드의 IWxSavable 액터 + 첫 플레이어 Pawn Transform 을 슬롯 파일에 비동기 저장.
 *  - LoadSlot: 슬롯 파일을 메모리로 로드한 뒤 현재 맵을 ServerTravel 로 리로드한다(authority 전제). 액터 복원은
 *    리로드된 새 월드의 스트리밍 핸들러가, 플레이어 부활 위치는 새 GameMode 의 ChoosePlayerStart 가 담당한다.
 *    파일이 없으면 메모리를 빈 슬롯으로 리셋한 뒤에도 동일하게 리로드한다(체크포인트 미존재 시 기본 PlayerStart 부활).
 *  - GetPlayerRespawnTransform: GameMode 가 ChoosePlayerStart 에서 부활 진입점을 조회 (ServerTravel 후 흐름).
 *
 * GameInstanceSubsystem 이라 ServerTravel 을 가로질러 메모리가 유지된다.
 *
 * World Partition 셀 스트리밍은 내부 핸들러가 자동 처리:
 *  - 스트리밍-인 (LevelAddedToWorld) / 영구 레벨 초기화 (OnWorldInitializedActors): IWxSavable 액터에 메모리 기록을 자동 복원.
 *  - 스트리밍-아웃 (LevelRemovedFromWorld): 셀의 IWxSavable 상태를 메모리에 자동 캡처해 셀 왕복으로 인한 손실 방지.
 *  - 메모리가 비어있는 신규 세션은 복원이 noop.
 *  - 자기 GameInstance 의 게임 월드만 처리 (PIE 다중 인스턴스 격리).
 */
UCLASS()
class WXSAVE_API UWxSaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** 현재 월드의 IWxSavable 액터 상태 + 첫 플레이어 Pawn Transform 을 캡처하고 SlotName 파일에 비동기 기록한다. */
	void SaveSlot(const FString& SlotName);

	/**
	 * SlotName 파일을 메모리로 로드한 뒤 현재 맵을 ServerTravel 로 리로드한다(authority 전제). 액터 복원은 리로드된
	 * 월드의 스트리밍 핸들러가 자동 수행한다. 파일 부재 시 메모리를 빈 슬롯으로 리셋한 뒤에도 동일하게 리로드한다.
	 * @return 슬롯 파일이 존재했는지 여부(리로드는 어느 경우든 수행되므로 사후 통보용).
	 */
	bool LoadSlot(const FString& SlotName);

	/** GameMode 가 ChoosePlayerStart 에서 호출. 슬롯에 한 번도 SaveSlot 된 적 없으면 false. */
	bool GetPlayerRespawnTransform(FTransform& OutTransform) const;

	/** 현재 메모리 슬롯 상태(레코드 키 목록 + 부활 Transform)를 LogWxSave 로 덤프한다. 콘솔 명령 Wx.Save.Dump 의 구현. */
	void LogSaveState() const;

	//~ Begin UGameInstanceSubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End UGameInstanceSubsystem

private:
	void EnsureSaveObject();

	void CaptureActor(AActor* Actor);

	/** @return 슬롯에서 일치 레코드를 찾아 복원했으면 true (신규 세션 등 레코드 없음/미설정 키는 false). */
	bool RestoreActor(AActor* Actor);

	/** 본 GameInstance 의 게임 월드인지 확인 (PIE 다중 인스턴스 격리용). */
	bool IsOwnedGameWorld(const UWorld* World) const;

	/** OnWorldInitializedActors 핸들러: 영구 레벨 + 초기 WP 셀 액터의 메모리 ActorRecords 자동 복원. */
	void HandleWorldInitializedActors(const UWorld::FActorsInitializedParams& Params);

	/** LevelAddedToWorld 핸들러: 스트리밍-인 셀(World Partition 포함) 액터에 메모리 ActorRecords 자동 복원. */
	void HandleLevelAddedToWorld(ULevel* Level, UWorld* World);

	/** LevelRemovedFromWorld 핸들러: 스트리밍-아웃 직전 셀 액터 상태를 메모리에 자동 캡처. */
	void HandleLevelRemovedFromWorld(ULevel* Level, UWorld* World);

	UPROPERTY()
	TObjectPtr<UWxSaveGame> CurrentSave;

	FDelegateHandle WorldInitializedActorsHandle;

	FDelegateHandle LevelAddedHandle;

	FDelegateHandle LevelRemovedHandle;
};

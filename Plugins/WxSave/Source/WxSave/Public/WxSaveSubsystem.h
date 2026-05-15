// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WxSaveSubsystem.generated.h"

class APlayerController;
class USaveGame;
class UWxPlayerSave;

/**
 * WxSave 의 슬롯 IO + 메모리 상태 단일 관리.
 *
 * GameInstanceSubsystem 으로 월드 리로드를 가로질러 단일 인스턴스가 유지된다 (souls-like 패턴):
 *  - Initialize: 슬롯이 있으면 동기 로드, 없으면 빈 슬롯 생성. 메모리에만 보관하고 자동 적용은 하지 않는다.
 *  - RestartFromSave: 슬롯 적용 플래그 set + 슬롯 맵으로 ServerTravel. 맵 리로드 후 OnWorldInitializedActors 가
 *    슬롯 상태를 fresh world 에 자동 push. PC pawn 의 spawn 위치는 AWxGameMode 가 IsSaveApplied/HasSavedLocation 을
 *    query 해 saved transform 에서 직접 spawn 시킴 (모든 PC 가 같은 좌표로 모이고 SpawnCollisionHandling 이 분산 처리).
 *  - 한 번 적용된 세션의 스트리밍 인-스트림 액터에도 자동 push (셀 재진입 시 슬롯 상태 일관성).
 *
 * 직접 호출 API:
 *  - SaveGame(PC): PC 빙의 Pawn Transform + 현재 맵 + 월드 savable 액터 상태 → 메모리 갱신 → 슬롯 비동기 기록.
 *  - RestartFromSave(PC): 슬롯에 저장된 맵으로 ServerTravel. 슬롯 없으면 noop. 멀티 세션에서도 모든 PC 가 따라온다.
 *  - BeginNewGame(): "New Game" 진입점에서 호출. 메모리 슬롯을 빈 상태로 리셋해 자동 push/spawn 을 비활성화.
 *  - DeleteSavedGame(): 슬롯 삭제 + 메모리 초기화.
 */
UCLASS()
class WXSAVE_API UWxSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** WorldContextObject 의 GameInstance 에 등록된 본 서브시스템 조회. nullptr 가능. */
	static UWxSaveSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	/** PC 의 빙의 Pawn Transform 과 현재 월드의 모든 savable 액터 상태를 슬롯에 비동기 기록한다. 서버 권위. 클라에서는 무시. */
	void SaveGame(APlayerController* PC);

	/**
	 * 슬롯에 저장된 맵으로 OpenLevel. 재로드 후 OnWorldInitializedActors 가 슬롯 상태를 자동 push 하고
	 * AWxGameMode 가 saved transform 으로 첫 Pawn 을 spawn 시킨다.
	 * 슬롯에 저장 위치가 없으면 noop. 슬롯 없는 신규 세션은 GameMode 기본 흐름(PlayerStart)이 처리.
	 *
	 * 반환값: 맵 reload 를 트리거했으면 true.
	 */
	bool RestartFromSave(APlayerController* PC);

	bool HasSavedGame() const;

	/** 슬롯 파일 삭제 + 메모리 상태 리셋. 슬롯 부재 시 false. */
	bool DeleteSavedGame();

	/**
	 * "New Game" 진입점에서 호출. 메모리 슬롯을 빈 상태로 교체하고 적용 플래그를 끈다.
	 * 슬롯 파일은 보존되며 다음 SaveGame 호출이 덮어쓴다.
	 * 호출하지 않으면 같은 GameInstance 라이프타임 동안 bSaveApplied 가 sticky 로 유지되어 신규 세션에도 자동 적용된다.
	 */
	void BeginNewGame();

	/** RestartFromSave 로 슬롯이 세션에 적용된 상태인지. AWxGameMode 가 spawn 결정 시 query. */
	bool IsSaveApplied() const;

	/** 슬롯에 유효한 플레이어 위치가 저장되어 있는지. */
	bool HasSavedLocation() const;

	/** 슬롯에 저장된 플레이어 Transform. HasSavedLocation 이 true 일 때만 의미 있음. */
	FTransform GetSavedPlayerTransform() const;

	/** 메모리 슬롯 객체. 외부 직접 mutate 금지 (디버그/조회 전용). */
	const UWxPlayerSave* GetCurrentSave() const;

private:
	void LoadOrCreateInMemorySave();

	void HandleWorldInitializedActors(const UWorld::FActorsInitializedParams& Params);

	void HandleLevelAddedToWorld(ULevel* Level, UWorld* World);

	void ApplyStateToWorld(UWorld* World);

	void ApplyStateToLevel(ULevel* Level);

	void ApplyStateToActor(AActor* Actor);

	void CaptureWorldState(UWorld* World);

	void CaptureActorState(AActor* Actor);

	UPROPERTY()
	TObjectPtr<UWxPlayerSave> CurrentSave;

	/** RestartFromSave 로 슬롯이 월드에 적용된 세션인지. true 일 때만 자동 push 와 GameMode 의 saved-transform spawn 이 활성화된다. */
	bool bSaveApplied = false;

	FDelegateHandle WorldInitializedActorsHandle;

	FDelegateHandle LevelAddedHandle;
};

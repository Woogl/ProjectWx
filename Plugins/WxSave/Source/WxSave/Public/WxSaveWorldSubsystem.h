// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Subsystems/WorldSubsystem.h"
#include "WxSaveWorldSubsystem.generated.h"

class IWxSavable;
class ULevel;
class UWxSaveGame;
class UWxSaveGameSubsystem;

/**
 * 월드 단위 플러시/복원 오케스트레이션을 담당하는 World 서브시스템 (샘플 UPersistenceWorldSubsystem 골격 이식).
 * 메모리 SaveGame 의 소유는 UWxSaveGameSubsystem 이고, 이 서브시스템은 월드 수명 이벤트에 맞춰 그 SaveGame 을 읽고 쓴다.
 *
 * 자동 처리:
 *  - 영구 레벨 초기화 (OnWorldInitializedActors) / 스트리밍-인 (LevelAddedToWorld): IWxSavable 액터에 레코드 자동 복원.
 *  - 스트리밍-아웃 (LevelRemovedFromWorld): 셀의 IWxSavable 상태를 메모리에 자동 캡처해 셀 왕복으로 인한 손실 방지.
 *  - 맵 이탈 (OnWorldBeginTearDown): 현재 월드 IWxSavable 전체를 메모리에 플러시해 같은 세션 맵 왕복 상태 유지(디스크 기록 없음).
 *  - IsTravelingFromSaveFile 동안 위 자동 캡처를 전부 스킵해, 막 로드한 세이브가 라이브 상태로 오염되는 것을 방지.
 *  - OnWorldBeginPlay 에서 트래블 완료를 게임 서브시스템에 보고해 가드를 해제한다.
 */
UCLASS()
class WXSAVE_API UWxSaveWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** RequestSaveFlush 완료 통지. 현재 플러시가 전부 동기라 즉시 발화되지만, 비동기 작업이 생길 때 지연 완료로 되돌릴 seam 으로 시그니처를 유지한다(샘플 동일 형태). */
	DECLARE_MULTICAST_DELEGATE(FOnSaveFlushComplete);

	/**
	 * 디스크 기록 전, 라이브 상태를 SaveGame 에 플러시한다: 맵 트래블 데이터 + 플레이어 스냅샷(트랜스폼·스탯) + IWxSavable 액터 전체.
	 * 앞의 셋은 teardown 중엔 스킵한다 — 맵 전환을 일으킨 게임 코드가 다음 시작 지점의 소유자이고, 그 시점 폰은 이미 사라졌거나 사망 상태일 수 있다.
	 * ResumeTransform 이 주어지면 재개 지점을 그 값으로 확정한다(체크포인트 오토세이브 — 상호작용 위치와 무관하게 체크포인트 자리로 고정). null 이면 폰 위치를 캡처한다.
	 * OnComplete 는 플러시 완료 후 발화한다(현재 동기라 반환 전 즉시).
	 */
	void RequestSaveFlush(FOnSaveFlushComplete::FDelegate OnComplete, const FTransform* ResumeTransform = nullptr);

	/** 플레이어 액터의 ASC 어트리뷰트 base 값을 OutStats 에 캡처한다(복제되는 것만 — 비복제 메타 제외). GAS 만 알고 구체 AttributeSet 타입엔 무관하다. */
	static void CapturePlayerStats(AActor* PlayerActor, TMap<FName, float>& OutStats);

	/** 캡처된 어트리뷰트 base 값을 플레이어 액터의 ASC 에 적용한다. 2패스로 적용해 PreAttributeChange 클램프·PostAttributeChange 비율 재조정과의 순서 의존을 흡수한다. */
	static void ApplyPlayerStats(AActor* PlayerActor, const TMap<FName, float>& InStats);

	//~ Begin UWorldSubsystem
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	//~ End UWorldSubsystem

private:
	/** 메모리 SaveGame 을 소유한 게임 서브시스템. 없으면 Warning 을 남기고 null 을 답한다. */
	UWxSaveGameSubsystem* GetGameSubsystem() const;

	/** 게임 서브시스템이 들고 있는 활성 SaveGame. 없으면 Warning 을 남기고 null 을 답한다. */
	UWxSaveGame* GetActiveSaveGame() const;

	/** 현재 맵을 캡처해 게임 서브시스템의 TravelData 로 푸시한다. */
	void FlushMapTravelData();

	void FlushSavableActors();

	/**
	 * SaveGame 최상위 PlayerTransform(재개 지점)을 채운다.
	 * ResumeTransform 이 주어지면 폰을 보지 않고 그 값을 그대로 쓴다. null 이면 첫 플레이어 폰의 트랜스폼을 캡처하고, 폰 부재 시 이전 캡처를 보존한다.
	 */
	void FlushPlayerTransform(const FTransform* ResumeTransform);

	/** 첫 플레이어 폰의 어트리뷰트를 SaveGame 최상위 PlayerStats 로 캡처한다(명시적 저장 경로 공통 — 체크포인트·메뉴 모두). */
	void FlushPlayerStats();

	/**
	 * 이 오브젝트를 저장할 필요가 있는가 — UPROPERTY(SaveGame) 중 아키타입 기본값과 다른 것이 하나라도 있으면 true.
	 * 태그 직렬화가 기본값과 같은 프로퍼티를 어차피 쓰지 않으므로, 같은 기준(아키타입 대비 Identical)을 직렬화 전에 미리 묻는 것이다.
	 */
	static bool ShouldSave(const UObject* Object);

	/**
	 * 액터+컴포넌트의 UPROPERTY(SaveGame) 를 레코드로 직렬화하고 버전 헤더를 갱신한다. 기본값과 다른 것을 가진 대상만 담으므로 컴포넌트 엔트리도 그런 컴포넌트에만 생긴다.
	 * 액터도 컴포넌트도 전부 기본값이면 기록하지 않고 기존 레코드까지 지운다 — 복원해봐야 레벨이 세워 둔 값을 다시 쓰는 것이라 결과가 같다.
	 * @return 레코드를 기록했으면 true (구현체 없음/미설정 키/전부 기본값은 false).
	 */
	bool CaptureActor(UWxSaveGame& SaveGame, AActor* Actor);

	/**
	 * @return 슬롯에서 일치 레코드를 찾아 복원했으면 true (신규 세션 등 레코드 없음/미설정 키는 false).
	 * @param bOutIsSavable 액터가 IWxSavable 이었는지. 호출부가 집계하려고 다시 캐스팅하지 않게 답한다.
	 */
	bool RestoreActor(const UWxSaveGame& SaveGame, AActor* Actor, bool* bOutIsSavable = nullptr);

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

// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "GameFeaturePluginOperationResult.h"

#include "WxExperienceManagerComponent.generated.h"

class UGameFeatureAction;
class UWxExperienceDefinition;

DECLARE_MULTICAST_DELEGATE_OneParam(FWxOnExperienceLoaded, const UWxExperienceDefinition*);

/** Experience 로드 파이프라인의 진행 상태. */
enum class EWxExperienceLoadState : uint8
{
	Unloaded,
	Loading,
	LoadingGameFeatures,
	ExecutingActions,
	Loaded,
	Deactivating,
};

/**
 * Experience 로드·적용의 주체 (Lyra ExperienceManagerComponent 이식).
 *
 * GameMode(서버 전용)가 고른 Experience 참조를 복제해, 서버는 직접 호출·클라는 OnRep 으로 각자 같은 로드 파이프라인을 주행한다:
 * 에셋 번들 비동기 로드 → GameFeature 플러그인 활성 → 자기 월드 한정 컨텍스트로 액션 실행 → 로드 완료 브로드캐스트.
 * 로드 완료 이전의 폰 스폰·시작 지급을 막는 것은 GameMode 의 책임이다(CallOrRegister_OnExperienceLoaded 로 대기).
 */
UCLASS()
class UWxExperienceManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UWxExperienceManagerComponent(const FObjectInitializer& ObjectInitializer);

	//~ Begin UActorComponent
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UActorComponent

	/**
	 * 서버 전용. ID 를 에셋으로 해석(정의 에셋 자체는 소형이라 동기 로드)하고 로드 파이프라인을 시작한다.
	 * 이미 설정돼 있으면 무시한다 — 엔진 Reset() 이 InitGameState 를 재호출하는 경로에 대한 멱등 처리.
	 */
	void SetCurrentExperience(FPrimaryAssetId ExperienceId);

	/** 로드가 이미 끝났으면 즉시 호출하고, 아니면 완료 시점에 호출하도록 등록한다. */
	void CallOrRegister_OnExperienceLoaded(FWxOnExperienceLoaded::FDelegate&& Delegate);

	bool IsExperienceLoaded() const;

	/** 로드 완료를 전제로 현재 Experience 를 반환한다. 완료 전 호출은 프로그래밍 오류다. */
	const UWxExperienceDefinition* GetCurrentExperienceChecked() const;

private:
	UFUNCTION()
	void OnRep_CurrentExperience();

	/** Loading 진입: Experience·ActionSet 의 넷모드별 에셋 번들을 비동기 로드한다. */
	void StartExperienceLoad();

	/** 번들 로드 완료 콜백: GameFeature 플러그인 URL 을 수집해 활성화를 요청한다. 없으면 즉시 마무리로 넘어간다. */
	void HandleExperienceAssetsLoaded();

	/** GameFeature 플러그인 1개 활성 완료 콜백: 전부 끝나면 마무리로 넘어간다. */
	void HandleGameFeaturePluginLoaded(const UE::GameFeatures::FResult& Result);

	/** ExecutingActions: 자기 월드 한정 컨텍스트로 전체 액션을 활성화하고 Loaded 를 브로드캐스트한다. */
	void FinishExperienceLoad();

	/** 플러그인 이름 목록을 URL 로 해석해 GameFeaturePluginURLs 에 누적한다. 미발견은 에러 로그 후 건너뛴다. */
	void CollectGameFeaturePluginURLs(const TArray<FString>& FeaturePluginList);

	/** Experience 본체와 ActionSet 의 액션을 실행 순서대로 평탄화한다. */
	void CollectActions(TArray<UGameFeatureAction*>& OutActions) const;

	/** 이 판의 게임플레이 구성. 서버가 InitGameState 에서 설정하고 클라는 복제로 받는다. */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentExperience)
	TObjectPtr<const UWxExperienceDefinition> CurrentExperience;

	EWxExperienceLoadState LoadState = EWxExperienceLoadState::Unloaded;

	/** 활성화를 요청한 GameFeature 플러그인 URL. EndPlay 비활성의 대상이다. */
	TArray<FString> GameFeaturePluginURLs;

	int32 NumGameFeaturePluginsLoading = 0;

	/** 로드 완료 1회성 통지. 브로드캐스트 후 비운다. */
	FWxOnExperienceLoaded OnExperienceLoaded;
};

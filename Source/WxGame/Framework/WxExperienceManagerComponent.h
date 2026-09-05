// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "GameFeaturePluginOperationResult.h"

#include "WxExperienceManagerComponent.generated.h"

class UGameFeatureAction;
class UWxExperienceDefinition;

DECLARE_MULTICAST_DELEGATE_OneParam(FWxOnExperienceLoaded, const UWxExperienceDefinition*);

enum class EWxExperienceLoadState : uint8
{
	Unloaded,
	Loading,
	LoadingGameFeatures,
	ExecutingActions,
	Loaded,
	Failed,
	Deactivating,
};

/**
 * Experience 로드·적용의 주체 (Lyra ExperienceManagerComponent 이식).
 *
 * GameMode(서버 전용)가 고른 Experience 참조를 복제해, 서버는 직접 호출·클라는 OnRep 으로 각자 같은 로드 파이프라인을 주행한다.
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
	 * 서버 전용. 정의 에셋 자체는 소형이라 동기 로드한다.
	 * 이미 설정돼 있으면 무시한다 — 엔진 Reset() 이 InitGameState 를 재호출하는 경로에 대한 멱등 처리.
	 */
	void SetCurrentExperience(FPrimaryAssetId ExperienceId);

	void CallOrRegister_OnExperienceLoaded(FWxOnExperienceLoaded::FDelegate&& Delegate);

	bool IsExperienceLoaded() const;

	bool HasLoadFailed() const;

	const UWxExperienceDefinition* GetCurrentExperienceChecked() const;

	/** 확정됐으나 아직 로드 중일 수 있는 Experience 를 반환한다(미확정이면 널). */
	const UWxExperienceDefinition* GetCurrentExperience() const;

private:
	UFUNCTION()
	void OnRep_CurrentExperience();

	/** Loading 진입: Experience·ActionSet 의 넷모드별 에셋 번들을 비동기 로드한다. */
	void StartExperienceLoad();

	/** 번들 로드 완료 콜백: GameFeature 플러그인 URL 을 수집해 활성화를 요청한다. 없으면 즉시 마무리로 넘어간다. */
	void HandleExperienceAssetsLoaded();

	/** GameFeature 플러그인 1개 활성 완료 콜백: 전부 끝나면 성공 시에만 마무리로 넘어간다. */
	void HandleGameFeaturePluginLoaded(const UE::GameFeatures::FResult& Result, FString PluginURL);

	/** ExecutingActions: 자기 월드 한정 컨텍스트로 전체 액션을 활성화하고 Loaded 를 브로드캐스트한다. */
	void FinishExperienceLoad();

	/** 이 Experience 가 요청한 GameFeature 활성 참조를 해제한다. 로드 도중 종료할 때만 아직 콜백이 오지 않은 요청도 비활성화를 시도한다. */
	void ReleaseGameFeaturePluginRequests(bool bDeactivateAllRequestedPlugins);

	/** 플러그인 이름 목록을 URL 로 해석해 GameFeaturePluginURLs 에 누적한다. 하나라도 미발견이면 false 를 반환한다. */
	bool CollectGameFeaturePluginURLs(const TArray<FString>& FeaturePluginList);

	/** Experience 본체와 ActionSet 의 액션을 실행 순서대로 평탄화한다. */
	void CollectActions(TArray<UGameFeatureAction*>& OutActions) const;

	/** 서버가 InitGameState 에서 설정한다. */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentExperience)
	TObjectPtr<const UWxExperienceDefinition> CurrentExperience;

	EWxExperienceLoadState LoadState = EWxExperienceLoadState::Unloaded;

	/** EndPlay 비활성의 대상이다. */
	TArray<FString> GameFeaturePluginURLs;

	/** 활성 콜백이 성공한 플러그인만 기록한다. 실패 Experience 의 정리에서 비활성화 대상을 구분한다. */
	TArray<FString> ActivatedGameFeaturePluginURLs;

	int32 NumGameFeaturePluginsLoading = 0;

	/** 병렬 GameFeature 활성 중 하나라도 실패하면, 모든 콜백을 회수한 뒤 Experience 완료를 발행하지 않는다. */
	bool bGameFeaturePluginLoadFailed = false;

	/** 브로드캐스트 후 비운다. */
	FWxOnExperienceLoaded OnExperienceLoaded;
};

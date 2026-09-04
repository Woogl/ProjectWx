// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WxBossComponent.generated.h"

class AWxEnemyCharacter;
class AWxCharacterBase;
class USceneComponent;
class UWxBossComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FWxOnBossReady, UWxBossComponent* /*BossComponent*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FWxOnBossEndPlay, UWxBossComponent* /*BossComponent*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FWxOnBossEngagementChanged, bool /*bEngaged*/);

/**
 * 부착된 적을 보스로 표시하고, 보스전 활성 상태와 생명주기를 HUD 관찰자에게 제공한다.
 *
 * 보스는 종류가 아니라 역할이라 상속이 아닌 부착으로 가른다 — 같은 적 BP를 필드몹으로도 보스로도 쓸 수 있다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXGAME_API UWxBossComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	AWxEnemyCharacter* GetBossCharacter() const;
	/** AI가 전투 대상을 확보해 교전 중인지 반환한다. */
	bool IsEngaged() const;

	FWxOnBossEndPlay OnBossEndPlay;
	FWxOnBossEngagementChanged OnEngagementChanged;

	/**
	 * 보스가 쓸 수 있게 될 때마다 발행된다. 배치(서버)·복제 도착(클라) 어느 경로든 BeginPlay 로 수렴한다.
	 * 관찰자가 보스보다 먼저 존재할 수 있어(HUD 뷰모델) 인스턴스가 아니라 클래스 차원에 둔다 — 구독자는 월드로 자기 것인지 가린다.
	 */
	static FWxOnBossReady OnAnyBossReady;

private:
	UFUNCTION()
	void HandleAITargetChanged(USceneComponent* NewTarget);

	UFUNCTION()
	void HandleBossDeath(AWxCharacterBase* DeadCharacter);

	void SetEngaged(bool bInEngaged);

	bool bEngaged = false;
};

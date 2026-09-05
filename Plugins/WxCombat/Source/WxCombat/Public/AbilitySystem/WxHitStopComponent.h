// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "WxHitStopComponent.generated.h"

class UWxAbilitySystemComponent;
struct FActiveGameplayEffect;
struct FGameplayEffectSpec;

/**
 * 히트스톱 반응. 소유자 ASC에 Effect.HitStop이 붙어 있는 동안 아바타의 애니메이션과 이동을 함께 세운다.
 *
 * 몽타주 재생 속도가 아니라 메시의 GlobalAnimRateScale을 쓴다 — 몽타주보다 아래층이라 슬롯 그룹·재생 주체·몽타주 교체를 보지 않고, 되돌릴 값이 상수 1이라 무엇을 얼렸는지 기억할 필요가 없다.
 * 이동도 속도나 이동 모드가 아니라 이동 컴포넌트의 틱을 세운다 — 같은 이유로 되돌릴 값이 상수이고, 넉업 속도가 보존돼 풀리면 하던 이동이 이어진다.
 * 두 값의 주인은 히트스톱뿐이므로 다른 연출이 같은 값을 쓰기 시작하면 서로 덮는다.
 *
 * 권위자와 소유 클라는 자기가 받은 GE 인스턴스로, 시뮬 프록시는 복제된 태그로 판정한다.
 * 이동은 이 머신이 직접 굴리는 캐릭터에만 세운다 — 남의 캐릭터는 위치가 복제나 ServerMove로 들어와 틱을 꺼도 서지 않고, 스무딩만 멎는다.
 * 예측이 기각되면 GE가 제거되며 그대로 롤백된다.
 */
UCLASS()
class WXCOMBAT_API UWxHitStopComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxHitStopComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 지금 이 머신에서 시간이 서 있다. */
	bool IsFrozen() const;

private:
	bool IsSimulatedProxy() const;

	void HandleActiveGameplayEffectAdded(UAbilitySystemComponent* Target, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);
	void HandleGameplayEffectRemoved(const FActiveGameplayEffect& RemovedEffect);
	void HandleHitStopTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** 토글이 아니라 대입이라 이벤트를 놓쳐도 다음 호출에서 참값으로 돌아온다. */
	void RefreshFrozenState();

	UPROPERTY()
	TObjectPtr<UWxAbilitySystemComponent> AbilitySystemComponent;

	/** 이 컴포넌트가 센 인스턴스. 마지막 하나가 빠질 때 풀리므로 얼어 있는 동안의 재적중은 마지막 적중 기준이 된다. */
	TSet<FActiveGameplayEffectHandle> FrozenHandles;
};

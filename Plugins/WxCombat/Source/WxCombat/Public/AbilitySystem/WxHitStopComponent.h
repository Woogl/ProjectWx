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
 * 히트스톱 반응. 소유자 ASC에 Effect.HitStop이 붙어 있는 동안 아바타 메시의 애니메이션 시간을 세운다.
 *
 * 몽타주 재생 속도가 아니라 메시의 GlobalAnimRateScale을 쓴다 — 몽타주보다 아래층이라 슬롯 그룹·재생 주체·몽타주 교체를 보지 않고,
 * 되돌릴 값이 상수 1이라 무엇을 얼렸는지 기억할 필요가 없다. 이 값의 주인은 히트스톱뿐이므로 다른 연출이 같은 값을 쓰기 시작하면 서로 덮는다.
 *
 * 권위자와 소유 클라는 자기가 받은 GE 인스턴스로 판정한다. 공격자 쪽(자기가 인스티게이터)은 스펙의 어빌리티 인스턴스가 아직 몽타주를 쥐고
 * 있을 때만 세므로 복제본이 걸러지고, 예측 인스턴스만 남아 본인 화면이 정확히 지속시간에 풀린다. 예측이 기각되면 GE가 제거되며 그대로 롤백된다.
 * 시뮬 프록시는 로컬 인스턴스가 없어 복제된 태그가 유일한 신호다.
 *
 * 태그가 걷혔는데 아직 세워져 있으면 어느 머신에서든 되돌린다 — 권위가 끝났다고 하는 정지는 새어 나간 상태다.
 */
UCLASS()
class WXCOMBAT_API UWxHitStopComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxHitStopComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleActiveGameplayEffectAdded(UAbilitySystemComponent* Target, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);
	void HandleGameplayEffectRemoved(const FActiveGameplayEffect& RemovedEffect);
	void HandleHitStopTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** 지금 상태로 애니메이션 시간을 다시 정한다. 토글이 아니라 대입이라 이벤트를 놓쳐도 다음 호출에서 참값으로 돌아온다. */
	void RefreshAnimRateScale();

	UPROPERTY()
	TObjectPtr<UWxAbilitySystemComponent> AbilitySystemComponent;

	/** 이 컴포넌트가 센 인스턴스. 마지막 하나가 빠질 때 풀리므로 얼어 있는 동안의 재적중은 마지막 적중 기준이 된다. */
	TSet<FActiveGameplayEffectHandle> FrozenHandles;
};

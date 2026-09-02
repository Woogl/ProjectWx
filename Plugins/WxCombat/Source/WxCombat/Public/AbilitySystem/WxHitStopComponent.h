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
 * 히트스톱 반응. 소유자 ASC에 Effect.HitStop을 부여하는 GE가 들어오면 재생 중인 몽타주를 얼리고, 얼린 인스턴스가 모두 빠지면 되돌린다.
 *
 * 공격자 쪽(자기가 인스티게이터)은 스펙의 어빌리티 인스턴스가 아직 몽타주를 쥐고 있을 때만 얼려, 복제본을 거르고 같은 적중 처리에서 먼저 발동한 반응(패리 등)에 양보한다.
 * 피격자 쪽은 권위자와 로컬 조종자만 얼리고 나머지는 몽타주 복제로 본다 — 서버가 얼린 재생 속도는 로컬 조종 액터에 복제되지 않아 소유 클라는 복제본 도착에 맞춰 스스로 얼린다.
 * 복원은 태그 개수가 아니라 얼린 인스턴스 집합으로 판정한다 — 소유 클라에는 예측 인스턴스와 복제본이 공존해 태그가 RTT만큼 늦게 걷힌다.
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

	UPROPERTY()
	TObjectPtr<UWxAbilitySystemComponent> AbilitySystemComponent;

	/** 이 컴포넌트가 얼린 인스턴스. 마지막 하나가 빠질 때 복원하므로 얼어 있는 동안의 재적중은 마지막 적중 기준으로 풀린다. */
	TSet<FActiveGameplayEffectHandle> FrozenHandles;
};

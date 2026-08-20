// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/Ability/WxMontageSelector.h"
#include "WxAbility_HitReact.generated.h"

/**
 * 데미지 파이프라인이 보내는 Event.HitReact.* 로 트리거되어 그 태그에 매칭되는 몽타주를 재생한다.
 *
 * 대미지 파이프라인은 공격이 요청한 태그를 가공 없이 넘기고, 그 태그로 무엇을 할지는 여기서 정한다 — 그로기 중이면 넉 계열을 Normal로 낮춘다.
 *
 * 평타로는 경직이 나지 않는다 — 대미지 행이 Event.HitReact.* 태그를 싣지 않으면 이벤트 자체가 발송되지 않기 때문이다.
 * 가드 중 피격 반응은 WxAbility_Guard가 직접 처리하므로 Effect.Guard 중에는 이 어빌리티가 뜨지 않는다.
 * 피격 중에도 새 액션을 허용하는 캐릭터는 어빌리티 BP에서 차단·캔슬 컨테이너를 비운다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_HitReact : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_HitReact();

	/** 처형 짝 피격이 공격자 몽타주와 프레임 싱크돼야 하므로 ASPD를 반영하지 않는다. */
	virtual float GetMontagePlayRate() const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 피격 종류(Event.HitReact.*)로 세트가 갈린다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx", meta = (ShowOnlyInnerProperties))
	FWxMontageSelector MontageSelector;

private:
	void FaceInstigator(AActor* AvatarActor, const AActor* Instigator);
};

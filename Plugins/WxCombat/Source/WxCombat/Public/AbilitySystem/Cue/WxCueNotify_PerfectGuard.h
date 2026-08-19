// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "WxCueNotify_PerfectGuard.generated.h"

class UNiagaraSystem;
class USoundBase;

/** 퍼펙트 가드로 판정된 피격에서 UWxAbilitySystemComponent 가 발행한다. */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API UWxCueNotify_PerfectGuard : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UWxCueNotify_PerfectGuard();

	virtual void HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TObjectPtr<UNiagaraSystem> HitNiagaraSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TObjectPtr<USoundBase> HitSound;
};

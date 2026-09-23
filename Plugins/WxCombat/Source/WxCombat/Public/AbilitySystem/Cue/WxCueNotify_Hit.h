// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "WxCueNotify_Hit.generated.h"

class UNiagaraSystem;
class USoundBase;

/**
 * 대미지 GE의 피격 반응 컴포넌트가 서버에서 발행한다 — 공격자 클라도 서버 판정 뒤에 받는다.
 * 성립하지 않는 히트는 발행 전에 걸러지므로 여기서 따로 거르지 않는다.
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API UWxCueNotify_Hit : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UWxCueNotify_Hit();

	virtual void HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TObjectPtr<UNiagaraSystem> HitNiagaraSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TObjectPtr<USoundBase> HitSound;
	
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TSubclassOf<UCameraShakeBase> CameraShake;
};

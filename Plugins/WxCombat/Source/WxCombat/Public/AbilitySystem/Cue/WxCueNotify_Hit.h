// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "WxCueNotify_Hit.generated.h"

class UNiagaraSystem;
class USoundBase;

/**
 * 대미지 GE가 이 큐를 들고 다니므로 예측 적용한 공격자 클라에서도 엔진이 발행한다 — 임팩트가 서버 왕복을 기다리지 않는다.
 * 대신 발행 시점이 ExecCalc 판정보다 앞이라, 판정으로 걸러졌어야 할 히트는 여기서 대상 상태를 보고 스스로 접는다.
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

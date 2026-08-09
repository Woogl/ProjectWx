// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "WxCueNotify_PerfectGuard.generated.h"

class UNiagaraSystem;
class USoundBase;

/**
 * 퍼펙트 가드 성공 시 재생되는 GameplayCue.
 *
 * ExecCalc_Damage 의 퍼펙트 가드 분기에서 임팩트 위치를 Parameters.Location 으로 받아 스파크 Niagara 와 사운드를 재생한다.
 * 데미지 플로터/임팩트 큐와 분리되어 있으므로 BP 서브클래스에서 자유롭게 연출을 구성한다.
 */
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

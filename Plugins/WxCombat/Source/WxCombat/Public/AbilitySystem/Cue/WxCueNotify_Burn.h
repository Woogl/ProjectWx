// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "WxCueNotify_Burn.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/**
 * 화상 지속 GameplayCue.
 *
 * WxEffect_Burn이 활성 상태인 동안 시각 효과(Niagara)를 표시한다.
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API AWxCueNotify_Burn : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AWxCueNotify_Burn();

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Cue")
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

private:
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> SpawnedNiagaraComponent;
};

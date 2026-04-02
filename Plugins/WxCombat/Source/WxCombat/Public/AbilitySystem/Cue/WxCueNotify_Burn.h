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
 * 이펙트가 만료되거나 제거되면 자동으로 정리된다.
 */
UCLASS(Blueprintable)
class WXCOMBAT_API AWxCueNotify_Burn : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AWxCueNotify_Burn();

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	/** 화상 지속 중 캐릭터에 부착할 Niagara 이펙트 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Cue")
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

	/** 이펙트 부착 소켓 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Cue")
	FName AttachSocketName = NAME_None;

private:
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> SpawnedNiagaraComponent;
};

// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "WxCueNotify_BuffATK.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/**
 * ATK 버프 지속 GameplayCue.
 *
 * WxEffect_BuffATK가 활성 상태인 동안 시각 효과(Niagara)를 표시한다.
 * 이펙트가 만료되거나 제거되면 자동으로 정리된다.
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API AWxCueNotify_BuffATK : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AWxCueNotify_BuffATK();

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	/** 버프 지속 중 캐릭터에 부착할 Niagara 이펙트 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Cue")
	TObjectPtr<UNiagaraSystem> BuffNiagaraSystem;

	/** 이펙트 부착 소켓 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Cue")
	FName AttachSocketName = NAME_None;

private:
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> SpawnedNiagaraComponent;
};

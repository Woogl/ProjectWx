// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "WxCueNotify_Exceed.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/**
 * WxEffect_Exceed가 활성 상태인 동안 대상의 무기(AWxWeaponBase) 메시에 Niagara를 부착한다.
 * Niagara 컴포넌트의 Outer가 무기 액터이므로 무기와 함께 라이프사이클이 묶이며, 무기가 파괴되거나 Cue가 제거되면 자동으로 정리된다.
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API AWxCueNotify_Exceed : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AWxCueNotify_Exceed();

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Cue")
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

	/** 비워두면 무기 메시 원점에 부착 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Cue")
	FName WeaponAttachSocket = NAME_None;

private:
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> SpawnedNiagaraComponent;
};

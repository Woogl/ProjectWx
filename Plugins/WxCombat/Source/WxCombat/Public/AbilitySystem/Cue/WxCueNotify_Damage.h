// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "WxCueNotify_Damage.generated.h"

class AWxDamageFloaterActor;
class UNiagaraSystem;

/**
 * 데미지 플로터 GameplayCue 베이스 클래스.
 * HandleGameplayCue에서 DamageFloater 액터를 스폰한다.
 * BP 서브클래스에서 FloaterActorClass를 설정한다.
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API UWxCueNotify_Damage : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UWxCueNotify_Damage();

	virtual void HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Damage Floater")
	TSubclassOf<AWxDamageFloaterActor> FloaterActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Floater")
	TObjectPtr<UNiagaraSystem> HitNiagaraSystem;
};

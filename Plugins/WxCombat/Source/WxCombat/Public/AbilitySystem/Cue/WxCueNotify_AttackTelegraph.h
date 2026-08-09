// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "WxCueNotify_AttackTelegraph.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/**
 * 공격 텔레그래프(선딜 표시) GameplayCue.
 *
 * 순정 "GameplayCue (Looping)" 노티파이가 몽타주 구간에서 AddGameplayCue로 발행하면 활성 동안 대상 캐릭터 메시에 Niagara를 부착한다.
 * 재생 길이는 NS 자체 저작값(루프/고정)을 따른다.
 * 색상은 BP 서브클래스가 GameplayCueTag(색상 태그)와 NiagaraSystem(색상 NS)을 각자 지정해 나눈다.
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API AWxCueNotify_AttackTelegraph : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AWxCueNotify_AttackTelegraph();

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Cue")
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

	/** 메시의 부착 소켓 이름. 비워두면 메시 원점(발밑)에 부착 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Cue")
	FName SocketName = NAME_None;

private:
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> SpawnedNiagaraComponent;
};

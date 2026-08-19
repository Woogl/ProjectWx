// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Cue/WxCueNotify_Hit.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "WxGameplayTags.h"

UWxCueNotify_Hit::UWxCueNotify_Hit()
{
	GameplayCueTag = WxGameplayTags::GameplayCue_Hit;
}

void UWxCueNotify_Hit::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	Super::HandleGameplayCue(MyTarget, EventType, Parameters);

	if (EventType != EGameplayCueEvent::Executed || !MyTarget)
	{
		return;
	}

	// 예측 클라는 ExecCalc을 건너뛰고 낙관 발행하므로, 이 검사가 없으면 회피에 성공해도 공격자 화면에만 스파크가 튄다.
	// 대상이 남이면 무적 태그는 GE 복제로 도착하므로, 대상이 방금 무적에 들어간 순간에는 놓칠 수 있다.
	const UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MyTarget);
	if (TargetASC && TargetASC->HasMatchingGameplayTag(WxGameplayTags::Effect_Invincible))
	{
		return;
	}

	UWorld* World = MyTarget->GetWorld();
	if (!World)
	{
		return;
	}

	if (HitNiagaraSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, HitNiagaraSystem, Parameters.Location);
	}

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, HitSound, Parameters.Location);
	}
}

// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Groggy.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/Effect/WxEffect_DrainGP.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "BrainComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "WxGameplayTags.h"

UWxAbility_Groggy::UWxAbility_Groggy()
{
	// 소유 클라도 활성화돼야 그로기 자세가 그 화면에 뜬다 — 엔진은 로컬 조종 액터에 복제 몽타주를 적용하지 않는다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	// 그 클라 인스턴스는 연출만 맡는다. 실행·종료 요청을 서버가 무시하게 해 판정을 서버에 묶는다.
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Groggy);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Groggy);
	
	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Death);

	// 그로기 동안 새 액션을 막는다.
	ActivationGroup = EWxAbilityActivationGroup::Reaction;

	// 전부를 지목해도 반응은 끊기지 않아, 처형 짝 피격처럼 겹쳐 있어야 할 것이 살아남는다.
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = WxGameplayTags::Event_Groggy;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UWxAbility_Groggy::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!GroggyMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(MontagePollingTimerHandle, this, &UWxAbility_Groggy::HandleMontagePollTick, 0.1f, true);
	}

	// 클라가 복제된 GP로 다시 판정하면 종료 시점이 어긋나, 그 창의 히트에서 HitReact의 넉 강등이 갈리고 LaunchCharacter가 한쪽에서만 실행된다.
	if (ActorInfo->IsNetAuthority())
	{
		GPDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(UWxCombatAttributeSet::GetGPAttribute())
			.AddUObject(this, &UWxAbility_Groggy::HandleGPChanged);

		// 재생 rate를 1.0으로 고정하므로 몽타주 길이가 곧 그로기 길이다.
		const float GroggyDuration = GroggyMontage->GetPlayLength();

		FGameplayEffectSpecHandle DrainSpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_DrainGP::StaticClass(), GetAbilityLevel());
		if (DrainSpecHandle.IsValid())
		{
			DrainSpecHandle.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Duration, GroggyDuration);
			DrainGPEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, DrainSpecHandle);
		}
	}

	if (APawn* AvatarPawn = Cast<APawn>(ActorInfo->AvatarActor.Get()))
	{
		if (AAIController* AIController = Cast<AAIController>(AvatarPawn->GetController()))
		{
			if (UBrainComponent* Brain = AIController->GetBrainComponent())
			{
				Brain->PauseLogic(TEXT("Groggy"));
			}
		}
	}

	HandleMontagePollTick();
}

void UWxAbility_Groggy::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ActorInfo)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(MontagePollingTimerHandle);
		}

		if (APawn* AvatarPawn = Cast<APawn>(ActorInfo->AvatarActor.Get()))
		{
			if (AAIController* AIController = Cast<AAIController>(AvatarPawn->GetController()))
			{
				if (UBrainComponent* Brain = AIController->GetBrainComponent())
				{
					Brain->ResumeLogic(TEXT("Groggy"));
				}
			}
		}

		if (ActorInfo->AbilitySystemComponent.IsValid())
		{
			UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

			// ActivateAbility가 GroggyMontage 미설정으로 곧장 EndAbility를 부르는 경로가 있어 널일 수 있다.
			if (GroggyMontage)
			{
				ASC->StopMontageIfCurrent(*GroggyMontage);
			}

			if (DrainGPEffectHandle.IsValid())
			{
				ASC->RemoveActiveGameplayEffect(DrainGPEffectHandle);
				DrainGPEffectHandle.Invalidate();
			}

			if (GPDelegateHandle.IsValid())
			{
				ASC->GetGameplayAttributeValueChangeDelegate(UWxCombatAttributeSet::GetGPAttribute())
					.Remove(GPDelegateHandle);
				GPDelegateHandle.Reset();
			}
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Groggy::HandleGPChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue <= 0.f)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UWxAbility_Groggy::HandleMontagePollTick()
{
	if (!CurrentActorInfo)
	{
		return;
	}

	UAbilitySystemComponent* ASC = CurrentActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return;
	}

	if (ASC->GetCurrentMontage() != nullptr)
	{
		return;
	}
	
	ASC->PlayMontage(this, CurrentActivationInfo, GroggyMontage, 1.f);
}

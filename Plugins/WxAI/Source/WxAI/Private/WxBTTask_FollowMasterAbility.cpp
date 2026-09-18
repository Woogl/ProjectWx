// Copyright Woogle. All Rights Reserved.

#include "WxBTTask_FollowMasterAbility.h"
#include "WxBlackboardKeys.h"
#include "AIController.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UWxBTTask_FollowMasterAbility::UWxBTTask_FollowMasterAbility()
{
	NodeName = TEXT("Follow Master Ability");
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UWxBTTask_FollowMasterAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	UAbilitySystemComponent* ASC = AIController ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AIController->GetPawn()) : nullptr;
	UAbilitySystemComponent* Master = BB ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(WxBlackboardKeys::GetMaster(BB)) : nullptr;
	if (!ASC || !Master || FollowedAbilities.IsEmpty())
	{
		return EBTNodeResult::Failed;
	}

	CachedASC = ASC;
	CachedOwnerComp = &OwnerComp;
	MasterASC = Master;

	// Master 스킬 도중에 소환됐거나 앞 단계가 끝나기 전에 Master가 스킬을 썼으면, 진행 중인 것을 따라잡는다.
	const UGameplayAbility* InProgressAbility = nullptr;
	{
		FScopedAbilityListLock MasterScopeLock(*Master);
		for (const FGameplayAbilitySpec& Spec : Master->GetActivatableAbilities())
		{
			if (Spec.IsActive() && Spec.Ability && Spec.Ability->GetAssetTags().HasAnyExact(FollowedAbilities))
			{
				InProgressAbility = Spec.Ability;
				break;
			}
		}
	}
	if (InProgressAbility)
	{
		return Follow(InProgressAbility->GetAssetTags());
	}

	Master->AbilityActivatedCallbacks.AddUObject(this, &UWxBTTask_FollowMasterAbility::HandleMasterAbilityActivated);
	return EBTNodeResult::InProgress;
}

FString UWxBTTask_FollowMasterAbility::GetStaticDescription() const
{
	return FString::Printf(TEXT("Master가 발동하면 따라 함: %s"), *FollowedAbilities.ToStringSimple());
}

EBTNodeResult::Type UWxBTTask_FollowMasterAbility::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	const FGameplayAbilitySpecHandle Handle = ActivatedHandle;

	// 취소가 동기로 보내는 종료 통지가 FinishLatentTask로 새지 않게 구독부터 끊는다.
	CleanUp();

	// 반응 어빌리티는 대개 취소를 거부하는 Override라 종료를 기다리면 트리가 Aborting에 갇힌다. 요청만 하고 마감한다.
	if (ASC && Handle.IsValid())
	{
		ASC->CancelAbilityHandle(Handle);
	}
	return EBTNodeResult::Aborted;
}

void UWxBTTask_FollowMasterAbility::HandleMasterAbilityActivated(UGameplayAbility* Ability)
{
	if (!Ability || !Ability->GetAssetTags().HasAnyExact(FollowedAbilities))
	{
		return;
	}

	UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get();
	const EBTNodeResult::Type Result = Follow(Ability->GetAssetTags());
	if (Result != EBTNodeResult::InProgress && OwnerComp)
	{
		FinishLatentTask(*OwnerComp, Result);
	}
}

EBTNodeResult::Type UWxBTTask_FollowMasterAbility::Follow(const FGameplayTagContainer& MasterAbilityTags)
{
	if (UAbilitySystemComponent* Master = MasterASC.Get())
	{
		Master->AbilityActivatedCallbacks.RemoveAll(this);
	}
	MasterASC.Reset();

	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		CleanUp();
		return EBTNodeResult::Failed;
	}

	const FGameplayTagContainer FollowedTags = MasterAbilityTags.FilterExact(FollowedAbilities);
	{
		// 순회 중 발동이 어빌리티 목록을 바꿀 수 있다(GE의 GrantedAbilities 등). 락은 루프에만 걸어 뒤따르는 재조회가 반영된 목록을 보게 한다.
		FScopedAbilityListLock ActiveScopeLock(*ASC);
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			const FGameplayAbilitySpecHandle Handle = Spec.Handle;
			if (Spec.Ability && Spec.Ability->GetAssetTags().HasAny(FollowedTags) && ASC->TryActivateAbility(Handle))
			{
				ActivatedHandle = Handle;
				break;
			}
		}
	}

	if (!ActivatedHandle.IsValid())
	{
		CleanUp();
		return EBTNodeResult::Failed;
	}

	// 발동 안에서 이미 끝났으면 종료 통지를 놓쳤으므로 기다리지 않는다. 발동은 됐으니 치른 것으로 본다.
	const FGameplayAbilitySpec* ActiveSpec = ASC->FindAbilitySpecFromHandle(ActivatedHandle);
	if (!ActiveSpec || !ActiveSpec->IsActive())
	{
		CleanUp();
		return EBTNodeResult::Succeeded;
	}

	ASC->OnAbilityEnded.AddUObject(this, &UWxBTTask_FollowMasterAbility::HandleAbilityEnded);
	return EBTNodeResult::InProgress;
}

void UWxBTTask_FollowMasterAbility::HandleAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	if (AbilityEndedData.AbilitySpecHandle != ActivatedHandle)
	{
		return;
	}

	UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get();
	CleanUp();
	if (OwnerComp)
	{
		FinishLatentTask(*OwnerComp, AbilityEndedData.bWasCancelled ? EBTNodeResult::Failed : EBTNodeResult::Succeeded);
	}
}

void UWxBTTask_FollowMasterAbility::CleanUp()
{
	if (UAbilitySystemComponent* Master = MasterASC.Get())
	{
		Master->AbilityActivatedCallbacks.RemoveAll(this);
	}
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->OnAbilityEnded.RemoveAll(this);
	}
	MasterASC.Reset();
	CachedASC.Reset();
	CachedOwnerComp.Reset();
	ActivatedHandle = FGameplayAbilitySpecHandle();
}

// Copyright Woogle. All Rights Reserved.

#include "AI/WxBTTask_ActivateAbility.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

UWxBTTask_ActivateAbility::UWxBTTask_ActivateAbility()
{
	NodeName = TEXT("Activate Ability");
}

EBTNodeResult::Type UWxBTTask_ActivateAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	APawn* Pawn = AIController->GetPawn();
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!ASC || !AbilityTag.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	if (!ASC->TryActivateAbilitiesByTag(AbilityTag.GetSingleTagContainer()))
	{
		return EBTNodeResult::Failed;
	}

	CachedASC = ASC;
	CachedOwnerComp = &OwnerComp;

	AbilityEndedDelegateHandle = ASC->OnAbilityEnded.AddUObject(
		this, &UWxBTTask_ActivateAbility::HandleAbilityEnded);

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UWxBTTask_ActivateAbility::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	CleanUp();
	return EBTNodeResult::Aborted;
}

void UWxBTTask_ActivateAbility::HandleAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	if (!AbilityEndedData.AbilityThatEnded ||
		!AbilityEndedData.AbilityThatEnded->AbilityTags.HasTag(AbilityTag))
	{
		return;
	}

	CleanUp();

	UBehaviorTreeComponent* BTComp = CachedOwnerComp.Get();
	if (!BTComp)
	{
		return;
	}

	const EBTNodeResult::Type Result = AbilityEndedData.bWasCancelled
		? EBTNodeResult::Failed
		: EBTNodeResult::Succeeded;
	FinishLatentTask(*BTComp, Result);
}

void UWxBTTask_ActivateAbility::CleanUp()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->OnAbilityEnded.Remove(AbilityEndedDelegateHandle);
	}
	AbilityEndedDelegateHandle.Reset();
}

FString UWxBTTask_ActivateAbility::GetStaticDescription() const
{
	return FString::Printf(TEXT("Activate Ability: %s"), *AbilityTag.ToString());
}

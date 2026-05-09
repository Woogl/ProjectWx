// Copyright Woogle. All Rights Reserved.

#include "WxBTTask_ActivateAbility.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UWxBTTask_ActivateAbility::UWxBTTask_ActivateAbility()
{
	NodeName = TEXT("Activate Ability");
	bCreateNodeInstance = true;
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

	FGameplayAbilitySpec* Spec = nullptr;
	for (FGameplayAbilitySpec& IterSpec : ASC->GetActivatableAbilities())
	{
		if (IterSpec.Ability && IterSpec.Ability->GetAssetTags().HasTag(AbilityTag))
		{
			Spec = &IterSpec;
			break;
		}
	}

	if (!Spec || !ASC->TryActivateAbility(Spec->Handle))
	{
		return EBTNodeResult::Failed;
	}

	// TryActivateAbility 내부에서 어빌리티가 동기적으로 종료될 수 있다 (CommitAbility 실패 등).
	// 이 경우 OnAbilityEnded가 이미 브로드캐스트된 후이므로, 델리게이트를 등록해도 콜백이 발생하지 않아 BT가 InProgress 상태로 영구 정지한다.
	if (!Spec->IsActive())
	{
		return EBTNodeResult::Failed;
	}

	CachedASC = ASC;
	CachedOwnerComp = &OwnerComp;
	ActivatedHandle = Spec->Handle;

	AbilityEndedDelegateHandle = ASC->OnAbilityEnded.AddUObject(
		this, &UWxBTTask_ActivateAbility::HandleAbilityEnded);

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UWxBTTask_ActivateAbility::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 델리게이트를 먼저 해제하여, CancelAbilities가 트리거하는 OnAbilityEnded 콜백이 FinishLatentTask를 호출하지 않도록 한다.
	CleanUp();

	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(AbilityTag);
		ASC->CancelAbilities(&Tags);
	}

	return EBTNodeResult::Aborted;
}

void UWxBTTask_ActivateAbility::HandleAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	if (AbilityEndedData.AbilitySpecHandle != ActivatedHandle)
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
	ActivatedHandle = FGameplayAbilitySpecHandle();
}

FString UWxBTTask_ActivateAbility::GetStaticDescription() const
{
	return FString::Printf(TEXT("Activate Ability: %s"), *AbilityTag.ToString());
}

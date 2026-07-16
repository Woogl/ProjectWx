// Copyright Woogle. All Rights Reserved.

#include "WxBTTask_ActivateAbility.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UWxBTTask_ActivateAbility::UWxBTTask_ActivateAbility()
{
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

	// 동일 태그 어빌리티가 여러 개일 수 있으므로, 발동에 성공하는 첫 후보를 채택한다.
	FGameplayAbilitySpecHandle ActivatedSpecHandle;
	for (const FGameplayAbilitySpec& IterSpec : ASC->GetActivatableAbilities())
	{
		if (IterSpec.Ability && IterSpec.Ability->GetAssetTags().HasTag(AbilityTag))
		{
			// 핸들은 값 타입이라 TryActivateAbility 가 배열을 재할당해도 안전하다. 호출 전에 캡처한다.
			const FGameplayAbilitySpecHandle CandidateHandle = IterSpec.Handle;
			if (ASC->TryActivateAbility(CandidateHandle))
			{
				// 성공 시 즉시 break — 활성화로 배열이 재할당됐어도 이후 IterSpec 을 건드리지 않는다.
				ActivatedSpecHandle = CandidateHandle;
				break;
			}
			// 실패(CanActivate 실패)는 ActivatableAbilities 를 바꾸지 않으므로 다음 후보로 계속 진행해도 안전하다.
		}
	}

	if (!ActivatedSpecHandle.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	// TryActivateAbility 는 활성화 도중 어빌리티 부여/제거로 ActivatableAbilities 배열을 재할당할 수 있어,
	// 활성화 이전에 잡아둔 Spec 포인터는 무효가 될 수 있다. 반드시 핸들로 다시 조회한다.
	// 또한 TryActivateAbility 내부에서 어빌리티가 동기적으로 종료될 수 있다 (CommitAbility 실패 등).
	// 이 경우 OnAbilityEnded가 이미 브로드캐스트된 후이므로, 델리게이트를 등록해도 콜백이 발생하지 않아 BT가 InProgress 상태로 영구 정지한다.
	const FGameplayAbilitySpec* ActiveSpec = ASC->FindAbilitySpecFromHandle(ActivatedSpecHandle);
	if (!ActiveSpec || !ActiveSpec->IsActive())
	{
		return EBTNodeResult::Failed;
	}

	CachedASC = ASC;
	CachedOwnerComp = &OwnerComp;
	ActivatedHandle = ActivatedSpecHandle;

	AbilityEndedDelegateHandle = ASC->OnAbilityEnded.AddUObject(
		this, &UWxBTTask_ActivateAbility::HandleAbilityEnded);

	return EBTNodeResult::InProgress;
}

FString UWxBTTask_ActivateAbility::GetStaticDescription() const
{
	return FString::Printf(TEXT("AbilityTag: %s"), *AbilityTag.ToString());
}

EBTNodeResult::Type UWxBTTask_ActivateAbility::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// CleanUp 이 ActivatedHandle 을 리셋하므로 취소할 핸들을 먼저 캡처한다.
	const FGameplayAbilitySpecHandle HandleToCancel = ActivatedHandle;

	// 델리게이트를 먼저 해제하여, CancelAbilityHandle이 트리거하는 OnAbilityEnded 콜백이 FinishLatentTask를 호출하지 않도록 한다.
	CleanUp();

	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->CancelAbilityHandle(HandleToCancel);
	}

	return Super::AbortTask(OwnerComp, NodeMemory);
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

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

	FGameplayAbilitySpecHandle ActivatedSpecHandle;
	{
		// 순회 중 활성화도 실패 통지도 어빌리티 목록을 바꿀 수 있다(GE의 GrantedAbilities, 실패 콜백의 Give/Clear 등).
		// 락이 없으면 Add/RemoveAtSwap 이 즉시 반영돼 순회 중인 참조가 무효화된다. 엔진 입력 경로도 같은 이유로 이 락을 건다.
		// 락은 루프에만 걸어, 뒤따르는 재조회가 부여/제거까지 반영된 목록을 보게 한다.
		FScopedAbilityListLock ActiveScopeLock(*ASC);

		// 동일 태그 어빌리티가 여러 개일 수 있으므로, 발동에 성공하는 첫 후보를 채택한다.
		for (const FGameplayAbilitySpec& IterSpec : ASC->GetActivatableAbilities())
		{
			if (IterSpec.Ability && IterSpec.Ability->GetAssetTags().HasTag(AbilityTag))
			{
				if (ASC->TryActivateAbility(IterSpec.Handle))
				{
					ActivatedSpecHandle = IterSpec.Handle;
					break;
				}
			}
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

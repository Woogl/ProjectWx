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

	CachedASC = ASC;
	CachedOwnerComp = &OwnerComp;

	// TryActivateAbility 안에서 어빌리티가 동기 종료될 수 있으므로(즉발 어빌리티, CommitAbility 실패 등), 그 종료 통지를 받으려면 발동 전에 바인드해야 한다.
	AbilityEndedDelegateHandle = ASC->OnAbilityEnded.AddUObject(
		this, &UWxBTTask_ActivateAbility::HandleAbilityEnded);

	ActivationResult = EBTNodeResult::InProgress;
	bIsActivating = true;
	{
		// 순회 중 활성화도 실패 통지도 어빌리티 목록을 바꿀 수 있다(GE의 GrantedAbilities, 실패 콜백의 Give/Clear 등).
		// 락은 루프에만 걸어, 뒤따르는 재조회가 부여/제거까지 반영된 목록을 보게 한다.
		FScopedAbilityListLock ActiveScopeLock(*ASC);

		// 동일 태그 어빌리티가 여러 개일 수 있으므로, 발동에 성공하는 첫 후보를 채택한다.
		for (const FGameplayAbilitySpec& IterSpec : ASC->GetActivatableAbilities())
		{
			if (IterSpec.Ability && IterSpec.Ability->GetAssetTags().HasTag(AbilityTag))
			{
				// 종료 콜백이 발동 도중 도착하므로 판별용 핸들을 미리 세운다.
				ActivatedHandle = IterSpec.Handle;
				if (ASC->TryActivateAbility(IterSpec.Handle))
				{
					break;
				}
				ActivatedHandle = FGameplayAbilitySpecHandle();
			}
		}
	}
	bIsActivating = false;

	// 발동 구간 안에서 끝났으면 그 결과가 결론이다(정리는 콜백이 이미 마쳤다).
	if (ActivationResult != EBTNodeResult::InProgress)
	{
		return ActivationResult;
	}

	if (!ActivatedHandle.IsValid())
	{
		CleanUp();
		return EBTNodeResult::Failed;
	}

	// TryActivateAbility 는 활성화 도중 어빌리티 부여/제거로 ActivatableAbilities 배열을 재할당할 수 있어, 활성화 이전에 잡아둔 Spec 포인터는 무효가 될 수 있다.
	// 종료 통지 없이 비활성이면(스펙 제거 등) 콜백이 오지 않아 BT 가 InProgress 로 영구 정지하므로 실패로 마감한다.
	const FGameplayAbilitySpec* ActiveSpec = ASC->FindAbilitySpecFromHandle(ActivatedHandle);
	if (!ActiveSpec || !ActiveSpec->IsActive())
	{
		CleanUp();
		return EBTNodeResult::Failed;
	}

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

	const EBTNodeResult::Type Result = AbilityEndedData.bWasCancelled
		? EBTNodeResult::Failed
		: EBTNodeResult::Succeeded;

	// 발동 구간 안이면 아직 InProgress 를 반환하기 전이라 FinishLatentTask 를 부를 수 없다. ExecuteTask 가 대신 반환한다.
	if (bIsActivating)
	{
		ActivationResult = Result;
		return;
	}

	UBehaviorTreeComponent* BTComp = CachedOwnerComp.Get();
	if (!BTComp)
	{
		return;
	}

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

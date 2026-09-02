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
	bIsRequestingAbort = false;

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

	if (!ActivatedHandle.IsValid())
	{
		CleanUp();
		return EBTNodeResult::Failed;
	}

	// TryActivateAbility 는 활성화 도중 어빌리티 부여/제거로 ActivatableAbilities 배열을 재할당할 수 있어, 활성화 이전에 잡아둔 Spec 포인터는 무효가 될 수 있다.
	const FGameplayAbilitySpec* ActiveSpec = ASC->FindAbilitySpecFromHandle(ActivatedHandle);

	// 발동 구간에 도착한 종료 통지가 방금 시작한 실행의 것이라는 보장은 없다.
	// 엔진은 재발동(bRetriggerInstancedAbility)에서 같은 핸들로 기존 실행을 먼저 끝낸 뒤 재활성화하므로, 통지 대신 "지금 도는 실행이 있는가" 를 결론으로 삼는다.
	if (ActiveSpec && ActiveSpec->IsActive())
	{
		return EBTNodeResult::InProgress;
	}

	// 비활성이면 발동 구간 안에서 끝난 것이므로 그때 받은 통지가 결론이다.
	// 통지 없이 비활성이면(스펙 제거 등) 콜백이 오지 않아 BT 가 InProgress 로 영구 정지하므로 실패로 마감한다.
	const EBTNodeResult::Type Result = ActivationResult != EBTNodeResult::InProgress
		? ActivationResult
		: EBTNodeResult::Failed;

	CleanUp();
	return Result;
}

FString UWxBTTask_ActivateAbility::GetStaticDescription() const
{
	return FString::Printf(TEXT("AbilityTag: %s"), *AbilityTag.ToString());
}

EBTNodeResult::Type UWxBTTask_ActivateAbility::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC || !ActivatedHandle.IsValid())
	{
		CleanUp();
		return EBTNodeResult::Aborted;
	}

	// 취소 요청은 실제 종료를 보장하지 않는다. 종료 통지를 받을 때까지 구독을 유지한다.
	bIsRequestingAbort = true;
	ASC->CancelAbilityHandle(ActivatedHandle);
	bIsRequestingAbort = false;

	const FGameplayAbilitySpec* ActiveSpec = ASC->FindAbilitySpecFromHandle(ActivatedHandle);
	if (!ActiveSpec || !ActiveSpec->IsActive())
	{
		// CancelAbilityHandle이 동기적으로 끝낸 경우에는 AbortTask 안의 콜백을 마감하지 않는다.
		CleanUp();
		return EBTNodeResult::Aborted;
	}

	return EBTNodeResult::InProgress;
}

void UWxBTTask_ActivateAbility::HandleAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	if (AbilityEndedData.AbilitySpecHandle != ActivatedHandle)
	{
		return;
	}

	const EBTNodeResult::Type Result = AbilityEndedData.bWasCancelled
		? EBTNodeResult::Failed
		: EBTNodeResult::Succeeded;

	// 발동 구간의 통지는 재발동으로 끝난 이전 실행의 것일 수 있어 그 자체로는 결론이 되지 못한다.
	// 결과만 남기고 구독 해제와 판단은 ExecuteTask 에 맡긴다.
	if (bIsActivating)
	{
		ActivationResult = Result;
		return;
	}

	// CancelAbilityHandle은 동기적으로 OnAbilityEnded를 브로드캐스트할 수 있다.
	// 이때 엔진은 아직 이 태스크를 Aborting으로 기록하지 않았으므로 AbortTask가 직접 Aborted를 반환한다.
	if (bIsRequestingAbort)
	{
		return;
	}

	UBehaviorTreeComponent* BTComp = CachedOwnerComp.Get();
	CleanUp();
	if (!BTComp)
	{
		return;
	}

	if (BTComp->GetTaskStatus(this) == EBTTaskStatus::Aborting)
	{
		FinishLatentAbort(*BTComp);
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
	bIsRequestingAbort = false;
}

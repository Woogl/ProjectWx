// Copyright Woogle. All Rights Reserved.

#include "AI/WxStateTreeTask_ActivateAbility.h"

#include "AIController.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"

FWxStateTreeTask_ActivateAbility::FWxStateTreeTask_ActivateAbility()
{
	// 완료는 어빌리티 종료 통지로 받는다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_ActivateAbility::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	Instance.AbilitySystem = nullptr;
	Instance.AbilityHandle = FGameplayAbilitySpecHandle();
	Instance.AbilityEndedHandle.Reset();

	const AAIController* AIController = Cast<AAIController>(Context.GetOwner());
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AIController ? AIController->GetPawn() : nullptr);
	if (!ASC || !Instance.AbilityTag.IsValid())
	{
		return EStateTreeRunStatus::Failed;
	}

	// TryActivateAbility 안에서 어빌리티가 동기 종료될 수 있다(즉발 어빌리티, CommitAbility 실패 등).
	// 그때는 이 상태가 아직 활성이 아니라 약한 실행 컨텍스트로 완료를 보낼 수 없으므로, 발동 구간의 종료 통지는 지역 변수로 받는다.
	FGameplayAbilitySpecHandle CandidateHandle;
	TOptional<bool> bCandidateEndCancelled;
	const FDelegateHandle ActivationEndedHandle = ASC->OnAbilityEnded.AddLambda(
		[&CandidateHandle, &bCandidateEndCancelled](const FAbilityEndedData& EndedData)
		{
			if (EndedData.AbilitySpecHandle == CandidateHandle)
			{
				bCandidateEndCancelled = EndedData.bWasCancelled;
			}
		});

	FGameplayAbilitySpecHandle ActivatedHandle;
	{
		// 순회 중 활성화도 실패 통지도 어빌리티 목록을 바꿀 수 있다(GE의 GrantedAbilities, 실패 콜백의 Give/Clear 등).
		// 락은 루프에만 걸어, 뒤따르는 재조회가 부여/제거까지 반영된 목록을 보게 한다.
		FScopedAbilityListLock ActiveScopeLock(*ASC);

		// 동일 태그 어빌리티가 여러 개일 수 있으므로, 발동에 성공하는 첫 후보를 채택한다.
		for (const FGameplayAbilitySpec& IterSpec : ASC->GetActivatableAbilities())
		{
			if (IterSpec.Ability && IterSpec.Ability->GetAssetTags().HasTag(Instance.AbilityTag))
			{
				// 채택하지 않은 후보가 남긴 통지는 다음 후보의 결론이 될 수 없다.
				CandidateHandle = IterSpec.Handle;
				bCandidateEndCancelled.Reset();
				if (ASC->TryActivateAbility(IterSpec.Handle))
				{
					ActivatedHandle = IterSpec.Handle;
					break;
				}
			}
		}
	}

	// 람다가 잡은 지역 변수가 사라지기 전에 걷는다.
	ASC->OnAbilityEnded.Remove(ActivationEndedHandle);

	if (!ActivatedHandle.IsValid())
	{
		return EStateTreeRunStatus::Failed;
	}

	// 발동 구간에 도착한 종료 통지가 방금 시작한 실행의 것이라는 보장은 없다.
	// 엔진은 재발동(bRetriggerInstancedAbility)에서 같은 핸들로 기존 실행을 먼저 끝낸 뒤 재활성화하므로, 통지 대신 "지금 도는 실행이 있는가" 를 결론으로 삼는다.
	const FGameplayAbilitySpec* ActiveSpec = ASC->FindAbilitySpecFromHandle(ActivatedHandle);
	if (!ActiveSpec || !ActiveSpec->IsActive())
	{
		// 비활성이면 발동 구간 안에서 끝난 것이므로 그때 받은 통지가 결론이다. 통지 없이 비활성이면(스펙 제거 등) 실패로 마감한다.
		return bCandidateEndCancelled.IsSet() && !bCandidateEndCancelled.GetValue()
			? EStateTreeRunStatus::Succeeded
			: EStateTreeRunStatus::Failed;
	}

	Instance.AbilitySystem = ASC;
	Instance.AbilityHandle = ActivatedHandle;
	Instance.AbilityEndedHandle = ASC->OnAbilityEnded.AddLambda(
		[ActivatedHandle, WeakContext = Context.MakeWeakExecutionContext()](const FAbilityEndedData& EndedData)
		{
			if (EndedData.AbilitySpecHandle == ActivatedHandle)
			{
				WeakContext.FinishTask(EndedData.bWasCancelled ? EStateTreeFinishTaskType::Failed : EStateTreeFinishTaskType::Succeeded);
			}
		});

	return EStateTreeRunStatus::Running;
}

void FWxStateTreeTask_ActivateAbility::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	UAbilitySystemComponent* ASC = Instance.AbilitySystem.Get();
	if (!ASC)
	{
		return;
	}

	// 취소가 동기적으로 내는 종료 통지가 떠나는 상태를 완료시키지 않도록 구독부터 걷는다.
	ASC->OnAbilityEnded.Remove(Instance.AbilityEndedHandle);

	const FGameplayAbilitySpec* ActiveSpec = ASC->FindAbilitySpecFromHandle(Instance.AbilityHandle);
	if (ActiveSpec && ActiveSpec->IsActive())
	{
		ASC->CancelAbilityHandle(Instance.AbilityHandle);
	}
}

#if WITH_EDITOR
FText FWxStateTreeTask_ActivateAbility::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Activate Ability ({0})"), FText::FromName(InstanceData->AbilityTag.GetTagName()));
}
#endif

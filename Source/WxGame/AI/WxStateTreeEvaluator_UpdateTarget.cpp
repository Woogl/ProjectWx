// Copyright Woogle. All Rights Reserved.

#include "AI/WxStateTreeEvaluator_UpdateTarget.h"

#include "AI/WxAIController.h"
#include "WxGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "StateTreeExecutionContext.h"

void FWxStateTreeEvaluator_UpdateTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	Instance.TimeUntilUpdate -= DeltaTime;
	if (Instance.TimeUntilUpdate > 0.f)
	{
		return;
	}
	Instance.TimeUntilUpdate = Interval;

	AWxAIController* AIController = Cast<AWxAIController>(Context.GetOwner());
	if (!AIController)
	{
		return;
	}

	AActor* CurrentTarget = AIController->GetTargetActor();
	const UAbilitySystemComponent* CurrentTargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(CurrentTarget);
	if (IsValid(CurrentTarget) && !(CurrentTargetASC && (CurrentTargetASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death) || CurrentTargetASC->HasMatchingGameplayTag(WxGameplayTags::Effect_IgnoreAggro))))
	{
		return;
	}

	UAIPerceptionComponent* Perception = AIController->GetPerceptionComponent();
	if (!Perception)
	{
		AIController->SetTargetActor(nullptr);
		return;
	}

	// 타겟에서 내려오는 대상은 감지 기록까지 지운다 — Hearing·Damage 자극은 MaxAge 안에 남아 있어, 자격을 되찾는 순간 그대로 어그로가 된다.
	Perception->ForgetActor(CurrentTarget);

	AActor* NewTarget = FindPerceivedTarget(*Perception, AIController->GetPawn());

	// 대상 없이 머무는 동안에는 같은 값을 다시 쓰지 않는다 — 쓸 때마다 락온 컴포넌트까지 내려간다.
	if (NewTarget != CurrentTarget)
	{
		AIController->SetTargetActor(NewTarget);
	}
}

AActor* FWxStateTreeEvaluator_UpdateTarget::FindPerceivedTarget(const UAIPerceptionComponent& Perception, const AActor* SelfActor) const
{
	TArray<AActor*> PerceivedActors;
	Perception.GetCurrentlyPerceivedActors(nullptr, PerceivedActors);

	for (AActor* PerceivedActor : PerceivedActors)
	{
		// 엔진 청각은 소리를 낸 본인의 리스너를 제외하지 않아, 자기 발소리가 그대로 자기 자극으로 돌아온다.
		if (PerceivedActor == SelfActor || !IsValid(PerceivedActor))
		{
			continue;
		}

		const UAbilitySystemComponent* PerceivedASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PerceivedActor);
		if (!(PerceivedASC && (PerceivedASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death) || PerceivedASC->HasMatchingGameplayTag(WxGameplayTags::Effect_IgnoreAggro))))
		{
			return PerceivedActor;
		}
	}

	return nullptr;
}

// Copyright Woogle. All Rights Reserved.

#include "WxBTService_UpdateTargetActor.h"

#include "WxBlackboardKeys.h"
#include "WxGameplayTags.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"

UWxBTService_UpdateTargetActor::UWxBTService_UpdateTargetActor()
{
	NodeName = TEXT("Update Target Actor");

	bNotifyTick = true;

	Interval = 0.1f;
	RandomDeviation = 0.0f;
}

void UWxBTService_UpdateTargetActor::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!AIController || !Blackboard)
	{
		return;
	}

	// 파괴된 타겟은 블랙보드 약참조가 이미 비워 두므로, 사망만 여기서 가른다.
	AActor* CurrentTarget = WxBlackboardKeys::GetTargetActor(Blackboard);
	if (CurrentTarget && !IsActorDead(CurrentTarget))
	{
		return;
	}

	const UAIPerceptionComponent* Perception = AIController->GetPerceptionComponent();
	if (!Perception)
	{
		return;
	}

	WxBlackboardKeys::SetTargetActor(Blackboard, FindPerceivedTarget(*Perception, AIController->GetPawn()));
}

AActor* UWxBTService_UpdateTargetActor::FindPerceivedTarget(const UAIPerceptionComponent& Perception, const AActor* SelfActor) const
{
	TArray<AActor*> PerceivedActors;
	Perception.GetCurrentlyPerceivedActors(nullptr, PerceivedActors);

	for (AActor* PerceivedActor : PerceivedActors)
	{
		// 엔진 청각은 소리를 낸 본인의 리스너를 제외하지 않아, 자기 발소리가 그대로 자기 자극으로 돌아온다.
		if (PerceivedActor != SelfActor && !IsActorDead(PerceivedActor))
		{
			return PerceivedActor;
		}
	}

	return nullptr;
}

bool UWxBTService_UpdateTargetActor::IsActorDead(AActor* Actor) const
{
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	return ASC && ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death);
}

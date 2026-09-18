// Copyright Woogle. All Rights Reserved.

#include "WxBTDecorator_ObserveAbility.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UWxBTDecorator_ObserveAbility::UWxBTDecorator_ObserveAbility()
{
	NodeName = TEXT("Observe Ability");
	bCreateNodeInstance = true;
	INIT_DECORATOR_NODE_NOTIFY_FLAGS();
	ActorToObserve.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, ActorToObserve), AActor::StaticClass());
}

void UWxBTDecorator_ObserveAbility::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (Asset.BlackboardAsset)
	{
		ActorToObserve.ResolveSelectedKey(*Asset.BlackboardAsset);
	}
}

FString UWxBTDecorator_ObserveAbility::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s의 %s 실행 중"), *Super::GetStaticDescription(), *ActorToObserve.SelectedKeyName.ToString(), *AbilityTags.ToStringSimple());
}

bool UWxBTDecorator_ObserveAbility::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	// 노드 인스턴스에는 transient인 키 ID가 복사된다는 보장이 없어 키 이름으로 읽는다.
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	const UAbilitySystemComponent* ASC = BB ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Cast<AActor>(BB->GetValueAsObject(ActorToObserve.SelectedKeyName))) : nullptr;
	if (!ASC)
	{
		return false;
	}

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (!Spec.Ability || !Spec.Ability->GetAssetTags().HasAny(AbilityTags))
		{
			continue;
		}

		// 발동 통지는 ActiveCount 증가 전이지만 PerActor 인스턴스는 이미 활성 상태다.
		const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
		const bool bIsActive = Spec.Ability->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerActor
			? Instance && Instance->IsActive()
			: Spec.IsActive();
		if (bIsActive)
		{
			return true;
		}
	}
	return false;
}

void UWxBTDecorator_ObserveAbility::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	CachedOwnerComp = &OwnerComp;
	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		BB->RegisterObserver(BB->GetKeyID(ActorToObserve.SelectedKeyName), this,
			FOnBlackboardChangeNotification::CreateUObject(this, &UWxBTDecorator_ObserveAbility::HandleActorToObserveChanged));
	}
	BindObservedASC();
}

void UWxBTDecorator_ObserveAbility::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UnbindObservedASC();
	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		BB->UnregisterObserversFrom(this);
	}
	CachedOwnerComp.Reset();

	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}

EBlackboardNotificationResult UWxBTDecorator_ObserveAbility::HandleActorToObserveChanged(const UBlackboardComponent& Blackboard, FBlackboard::FKey KeyID)
{
	UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get();
	if (!OwnerComp)
	{
		return EBlackboardNotificationResult::RemoveObserver;
	}

	BindObservedASC();
	ConditionalFlowAbort(*OwnerComp, EBTDecoratorAbortRequest::ConditionResultChanged);
	return EBlackboardNotificationResult::ContinueObserving;
}

void UWxBTDecorator_ObserveAbility::HandleAbilityActivated(UGameplayAbility* Ability)
{
	UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get();
	if (!OwnerComp || !Ability || !Ability->GetAssetTags().HasAny(AbilityTags))
	{
		return;
	}

	ConditionalFlowAbort(*OwnerComp, EBTDecoratorAbortRequest::ConditionResultChanged);
}

void UWxBTDecorator_ObserveAbility::HandleAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get();
	const UGameplayAbility* Ability = AbilityEndedData.AbilityThatEnded;
	if (OwnerComp && Ability && Ability->GetAssetTags().HasAny(AbilityTags))
	{
		ConditionalFlowAbort(*OwnerComp, EBTDecoratorAbortRequest::ConditionResultChanged);
	}
}

void UWxBTDecorator_ObserveAbility::BindObservedASC()
{
	UnbindObservedASC();

	UBehaviorTreeComponent* OwnerComp = CachedOwnerComp.Get();
	const UBlackboardComponent* BB = OwnerComp ? OwnerComp->GetBlackboardComponent() : nullptr;
	UAbilitySystemComponent* ASC = BB ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Cast<AActor>(BB->GetValueAsObject(ActorToObserve.SelectedKeyName))) : nullptr;
	if (!ASC)
	{
		return;
	}

	ObservedASC = ASC;
	ASC->AbilityActivatedCallbacks.AddUObject(this, &UWxBTDecorator_ObserveAbility::HandleAbilityActivated);
	ASC->OnAbilityEnded.AddUObject(this, &UWxBTDecorator_ObserveAbility::HandleAbilityEnded);
}

void UWxBTDecorator_ObserveAbility::UnbindObservedASC()
{
	if (UAbilitySystemComponent* ASC = ObservedASC.Get())
	{
		ASC->AbilityActivatedCallbacks.RemoveAll(this);
		ASC->OnAbilityEnded.RemoveAll(this);
	}
	ObservedASC.Reset();
}

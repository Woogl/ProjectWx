// Copyright Woogle. All Rights Reserved.

#include "WxBTService_ObserveMasterAbility.h"
#include "WxBlackboardKeys.h"
#include "WxAIModule.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "WxBTDecorator_MasterAbility.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

UWxBTService_ObserveMasterAbility::UWxBTService_ObserveMasterAbility()
{
	NodeName = TEXT("Observe Master Ability");
	bCreateNodeInstance = true;
	INIT_SERVICE_NODE_NOTIFY_FLAGS();
	bNotifyTick = false;
}

void UWxBTService_ObserveMasterAbility::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);
	if (!OwnerComp.GetAIOwner() || !OwnerComp.GetAIOwner()->HasAuthority())
	{
		return;
	}
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB || BB->GetKeyType(BB->GetKeyID(WxBlackboardKeys::Master)) != UBlackboardKeyType_Object::StaticClass())
	{
		UE_LOG(LogWxAI, Error, TEXT("Observe Master Ability: Blackboard=%s Asset=%s Master=%s"),
			*GetNameSafe(BB), *GetNameSafe(BB ? BB->GetBlackboardAsset() : nullptr),
			*GetNameSafe(BB ? BB->GetKeyType(BB->GetKeyID(WxBlackboardKeys::Master)).Get() : nullptr));
		return;
	}
	CachedBlackboard = BB;
	BB->RegisterObserver(BB->GetKeyID(WxBlackboardKeys::Master), this,
		FOnBlackboardChangeNotification::CreateUObject(this, &UWxBTService_ObserveMasterAbility::HandleMasterChanged));
	BindMaster();
}

void UWxBTService_ObserveMasterAbility::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UnbindMaster();
	if (UBlackboardComponent* BB = CachedBlackboard.Get())
	{
		BB->UnregisterObserversFrom(this);
	}
	PendingAbility = FGameplayTag();
	CachedBlackboard.Reset();
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}

EBlackboardNotificationResult UWxBTService_ObserveMasterAbility::HandleMasterChanged(const UBlackboardComponent& Blackboard, FBlackboard::FKey KeyID)
{
	BindMaster();
	return EBlackboardNotificationResult::ContinueObserving;
}

void UWxBTService_ObserveMasterAbility::UnbindMaster()
{
	if (UAbilitySystemComponent* ASC = MasterASC.Get())
	{
		ASC->AbilityActivatedCallbacks.Remove(ActivationHandle);
	}
	ActivationHandle.Reset();
	MasterASC.Reset();
}

void UWxBTService_ObserveMasterAbility::BindMaster()
{
	UnbindMaster();
	UBlackboardComponent* BB = CachedBlackboard.Get();
	if (!BB)
	{
		return;
	}
	PendingAbility = FGameplayTag();
	AActor* Master = Cast<AActor>(BB->GetValueAsObject(WxBlackboardKeys::Master));
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Master);
	if (!ASC)
	{
		return;
	}
	MasterASC = ASC;
	ActivationHandle = ASC->AbilityActivatedCallbacks.AddUObject(this, &UWxBTService_ObserveMasterAbility::HandleAbilityActivated);
	// 스킬 중간에 소환된 경우를 한 번 따라잡는다. 트리 재탐색은 같은 실행을 다시 요청하지 않는다.
	if (SynchronizedASC != ASC)
	{
		SynchronizedASC = ASC;
		FScopedAbilityListLock Lock(*ASC);
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (Spec.IsActive())
			{
				HandleAbilityActivated(Spec.Ability);
			}
		}
	}
}

void UWxBTService_ObserveMasterAbility::HandleAbilityActivated(UGameplayAbility* Ability)
{
	UBlackboardComponent* BB = CachedBlackboard.Get();
	UBehaviorTreeComponent* BT = BB ? Cast<UBehaviorTreeComponent>(BB->GetBrainComponent()) : nullptr;
	const UBTCompositeNode* Parent = GetParentNode();
	if (!BT || !Parent || !Ability)
	{
		return;
	}
	const FGameplayTag AbilityRoot = FGameplayTag::RequestGameplayTag(TEXT("Ability"));
	for (const FGameplayTag& Tag : Ability->GetAssetTags())
	{
		if (!Tag.MatchesTag(AbilityRoot))
		{
			continue;
		}
		PendingAbility = Tag;
		UE_LOG(LogWxAI, Verbose, TEXT("%s detected Master ability %s"), *GetNameSafe(BB->GetOwner()), *Tag.ToString());
		for (const FBTCompositeChild& Child : Parent->Children)
		{
			for (UBTDecorator* Decorator : Child.Decorators)
			{
				const UWxBTDecorator_MasterAbility* Reaction = Cast<UWxBTDecorator_MasterAbility>(Decorator);
				if (Reaction && Reaction->CalculateRawConditionValue(*BT, nullptr))
				{
					// 부모부터 다시 선택해야 같은 분기의 재시작과 낮은 우선순위 스킬로의 전환도 가능하다.
					BT->RequestExecution(Parent, BT->FindInstanceContainingNode(Parent), this, -1, EBTNodeResult::Aborted);
					return;
				}
			}
		}
		break;
	}
}

void UWxBTService_ObserveMasterAbility::DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);
	if (const UBlackboardComponent* BB = CachedBlackboard.Get())
	{
		Values.Add(FString::Printf(TEXT("Master: %s | 감지한 스킬: %s"), *GetNameSafe(MasterASC.Get()), *PendingAbility.ToString()));
	}
}

UWxBTService_ObserveMasterAbility* UWxBTService_ObserveMasterAbility::Find(const UBehaviorTreeComponent& OwnerComp, const UBTNode* Context)
{
	const UBTNode* Root = Context ? Context : OwnerComp.GetActiveNode();
	if (!Root)
	{
		return nullptr;
	}
	while (Root->GetParentNode())
	{
		Root = Root->GetParentNode();
	}
	const UBTCompositeNode* Composite = Cast<UBTCompositeNode>(Root);
	if (!Composite)
	{
		return nullptr;
	}
	// 이 서비스는 최상위 Composite에 배치한다. 상태는 에셋 템플릿이 아닌 AI별 인스턴스에서 읽는다.
	for (UBTService* Service : Composite->Services)
	{
		if (Service && Service->IsA<UWxBTService_ObserveMasterAbility>())
		{
			const int32 InstanceIndex = OwnerComp.FindInstanceContainingNode(Service);
			if (InstanceIndex != INDEX_NONE)
			{
				return Cast<UWxBTService_ObserveMasterAbility>(Service->GetNodeInstance(OwnerComp, OwnerComp.GetNodeMemory(Service, InstanceIndex)));
			}
		}
	}
	return nullptr;
}

FGameplayTag UWxBTService_ObserveMasterAbility::GetPendingAbility() const
{
	return PendingAbility;
}

void UWxBTService_ObserveMasterAbility::ConsumeAbility()
{
	PendingAbility = FGameplayTag();
}

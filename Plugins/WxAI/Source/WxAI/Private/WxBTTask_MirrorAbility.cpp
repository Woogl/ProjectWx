// Copyright Woogle. All Rights Reserved.

#include "WxBTTask_MirrorAbility.h"

#include "WxAIModule.h"
#include "WxBlackboardKeys.h"
#include "AIController.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

UWxBTTask_MirrorAbility::UWxBTTask_MirrorAbility()
{
	NodeName = TEXT("Mirror Ability");

	// 실행 상태(따라잡은 태그·발동 핸들·구독)를 노드가 직접 들고 있으므로 폰마다 인스턴스가 필요하다.
	bCreateNodeInstance = true;

	// TickTask 오버라이드를 감지해 알림 플래그(bNotifyTick 등)를 자동 설정한다(엔진 관용).
	INIT_TASK_NODE_NOTIFY_FLAGS();

	MirrorTarget.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UWxBTTask_MirrorAbility, MirrorTarget), AActor::StaticClass());
	MirrorTarget.SelectedKeyName = WxBlackboardKeys::Master;
}

void UWxBTTask_MirrorAbility::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		MirrorTarget.ResolveSelectedKey(*BlackboardAsset);
	}
	else
	{
		MirrorTarget.InvalidateResolvedKey();
	}
}

EBTNodeResult::Type UWxBTTask_MirrorAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	bIsRequestingAbort = false;

	const AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!ASC)
	{
		return EBTNodeResult::Failed;
	}

	MirroredTag = FindMirroredTag(OwnerComp);
	if (!MirroredTag.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	CachedASC = ASC;
	CachedOwnerComp = &OwnerComp;

	// TryActivateAbility 안에서 어빌리티가 동기 종료될 수 있으므로(CommitAbility 실패 등), 그 종료 통지를 받으려면 발동 전에 바인드해야 한다.
	AbilityEndedDelegateHandle = ASC->OnAbilityEnded.AddUObject(
		this, &UWxBTTask_MirrorAbility::HandleAbilityEnded);

	ActivationResult = EBTNodeResult::InProgress;
	bIsActivating = true;
	{
		// 순회 중 활성화도 실패 통지도 어빌리티 목록을 바꿀 수 있다(GE의 GrantedAbilities, 실패 콜백의 Give/Clear 등).
		FScopedAbilityListLock ActiveScopeLock(*ASC);

		// 같은 식별 태그의 어빌리티가 여럿일 수 있으므로, 발동에 성공하는 첫 후보를 채택한다.
		for (const FGameplayAbilitySpec& IterSpec : ASC->GetActivatableAbilities())
		{
			if (IterSpec.Ability && IterSpec.Ability->GetAssetTags().HasTag(MirroredTag))
			{
				// 종료 콜백이 발동 도중 도착하므로 판별용 핸들을 미리 세운다.
				ActivatedHandle = IterSpec.Handle;
				if (ASC->TryActivateAbility(IterSpec.Handle))
				{
					break;
				}
				// 채택하지 않은 후보가 남긴 통지는 다음 후보의 결론이 될 수 없다.
				ActivatedHandle = FGameplayAbilitySpecHandle();
				ActivationResult = EBTNodeResult::InProgress;
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

FString UWxBTTask_MirrorAbility::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s 가 쓰는 %s 를 따라한다"),
		*MirrorTarget.SelectedKeyName.ToString(),
		MirroredAbilities.IsEmpty() ? TEXT("없음") : *MirroredAbilities.ToStringSimple());
}

void UWxBTTask_MirrorAbility::DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);

	Values.Add(FString::Printf(TEXT("따라하는 중: %s"), MirroredTag.IsValid() ? *MirroredTag.ToString() : TEXT("없음")));
}

EBTNodeResult::Type UWxBTTask_MirrorAbility::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	// 엔진의 취소는 CanBeCanceled 를 거부하는 인스턴스에서 로그 한 줄 없이 아무 일도 하지 않는다.
	// 종료 통지가 온다는 보장이 없으므로 기다리지 않는다 — 여기서 InProgress 로 앉으면 트리 전체가 Aborting 에 갇힌다.
	for (const UGameplayAbility* Instance : ActiveSpec->GetAbilityInstances())
	{
		if (Instance && !Instance->CanBeCanceled())
		{
			UE_LOG(LogWxAI, Warning, TEXT("어빌리티 '%s' 가 취소를 거부해 Abort 를 즉시 마감합니다. 취소되지 않는 어빌리티는 따라할 목록에 넣지 마세요. (AbilityTag: %s)"),
				*Instance->GetName(), *MirroredTag.ToString());

			CleanUp();
			return EBTNodeResult::Aborted;
		}
	}

	return EBTNodeResult::InProgress;
}

void UWxBTTask_MirrorAbility::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	const UAbilitySystemComponent* TargetASC = FindMirrorTargetAbilitySystem(OwnerComp);
	if (TargetASC && TargetASC->HasMatchingGameplayTag(MirroredTag))
	{
		return;
	}

	// 대상이 놓았다(또는 사라졌다). abort 가 아니라 정상 마감이므로, 뒤따르는 종료 통지가 이 태스크를 끝내게 둔다.
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->CancelAbilityHandle(ActivatedHandle);
	}
}

FGameplayTag UWxBTTask_MirrorAbility::FindMirroredTag(const UBehaviorTreeComponent& OwnerComp) const
{
	const UAbilitySystemComponent* TargetASC = FindMirrorTargetAbilitySystem(OwnerComp);
	if (!TargetASC)
	{
		return FGameplayTag();
	}

	// 활성 어빌리티가 발행한 식별 태그만 남긴다. 부모 태그로 저작해도 걸리도록 Filter 가 계층을 펼쳐 준다.
	FGameplayTagContainer TargetTags;
	TargetASC->GetOwnedGameplayTags(TargetTags);
	return TargetTags.Filter(MirroredAbilities).First();
}

UAbilitySystemComponent* UWxBTTask_MirrorAbility::FindMirrorTargetAbilitySystem(const UBehaviorTreeComponent& OwnerComp) const
{
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return nullptr;
	}

	AActor* TargetActor = Cast<AActor>(Blackboard->GetValue<UBlackboardKeyType_Object>(MirrorTarget.GetSelectedKeyID()));
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
}

void UWxBTTask_MirrorAbility::HandleAbilityEnded(const FAbilityEndedData& AbilityEndedData)
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
	// AbortTask 안에서 FinishLatentAbort 로 되돌아가지 않게 막는다 — 마감은 AbortTask 반환값 하나에 맡긴다.
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

void UWxBTTask_MirrorAbility::CleanUp()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->OnAbilityEnded.Remove(AbilityEndedDelegateHandle);
	}
	AbilityEndedDelegateHandle.Reset();
	ActivatedHandle = FGameplayAbilitySpecHandle();
	MirroredTag = FGameplayTag();
	bIsRequestingAbort = false;
}

// Copyright Woogle. All Rights Reserved.

#include "Quest/WxStateTreeTask_WaitMoveToTarget.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"
#include "UniversalObjectLocators/ActorLocatorFragment.h"
#include "WxQuestModule.h"

FWxStateTreeTask_WaitMoveToTarget::FWxStateTreeTask_WaitMoveToTarget()
{
	// 도달 대기 중 같은 상태가 재선택되어도 진행을 끊을 이유가 없다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_WaitMoveToTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	if (Instance.Targets.IsEmpty())
	{
		UE_LOG(LogWxQuest, Warning, TEXT("Wait Move To Target: 도달을 판정할 대상이 지정되지 않음."));
	}
	for (const FUniversalObjectLocator& Locator : Instance.Targets)
	{
		if (Locator.IsEmpty())
		{
			UE_LOG(LogWxQuest, Warning, TEXT("Wait Move To Target: 빈 로케이터 항목이 있음(지정 %d개)."), Instance.Targets.Num());
			break;
		}
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_WaitMoveToTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner)
	{
		return EStateTreeRunStatus::Running;
	}

	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Owner, 0);
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Pawn)
	{
		return EStateTreeRunStatus::Running;
	}

	// 미해석(빈 로케이터·스트리밍 아웃)인 대상은 그 자리만 판정에서 빠진다.
	for (const FUniversalObjectLocator& Locator : Instance.Targets)
	{
		const AActor* Target = Cast<AActor>(Locator.SyncFind(Owner));
		if (Target && FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation()) <= Instance.AcceptRadius)
		{
			return EStateTreeRunStatus::Succeeded;
		}
	}

	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FWxStateTreeTask_WaitMoveToTarget::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Wait Move To Target ({0})"), GetTargetsText(InstanceData->Targets));
}

FString FWxStateTreeTask_WaitMoveToTarget::GetTargetDisplayName(const FUniversalObjectLocator& Locator) const
{
	if (Locator.IsEmpty())
	{
		return TEXT("unset");
	}

	if (const AActor* Actor = Cast<AActor>(Locator.SyncFind()))
	{
		return Actor->GetActorLabel();
	}

	// 미해석(언로드 등)이면 액터 프래그먼트의 소프트 경로 끝 이름이라도 보여준다.
	const FUniversalObjectLocatorFragment* Fragment = Locator.GetLastFragment();
	const FActorLocatorFragment* Payload = nullptr;
	if (Fragment && Fragment->TryGetPayloadAs(FActorLocatorFragment::FragmentType, Payload) && Payload)
	{
		const FString SubPath = Payload->Path.GetSubPathString();
		int32 DotIndex = INDEX_NONE;
		return SubPath.FindLastChar(TEXT('.'), DotIndex) ? SubPath.Mid(DotIndex + 1) : SubPath;
	}

	return TEXT("unresolved");
}

FText FWxStateTreeTask_WaitMoveToTarget::GetTargetsText(const TArray<FUniversalObjectLocator>& Targets) const
{
	if (Targets.IsEmpty())
	{
		return INVTEXT("none");
	}

	constexpr int32 MaxNames = 3;
	TArray<FString> Names;
	for (int32 Index = 0; Index < Targets.Num() && Index < MaxNames; ++Index)
	{
		Names.Add(GetTargetDisplayName(Targets[Index]));
	}

	FString Joined = FString::Join(Names, TEXT(", "));
	if (Targets.Num() > MaxNames)
	{
		Joined += FString::Printf(TEXT(" +%d"), Targets.Num() - MaxNames);
	}
	return FText::FromString(Joined);
}
#endif

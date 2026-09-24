// Copyright Woogle. All Rights Reserved.

#include "Device/WxDeviceStateTreeComponent.h"

#include "Device/WxDevice.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "StateTree.h"
#include "StateTreeExecutionContext.h"
#include "WxWorldModule.h"

namespace
{
	const FName RootInitialStateName(TEXT("Root"));
}

UWxDeviceStateTreeComponent::UWxDeviceStateTreeComponent()
{
	SetIsReplicatedByDefault(true);
	InitialState = RootInitialStateName;
}

void UWxDeviceStateTreeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWxDeviceStateTreeComponent, StateSnapshot);
}

bool UWxDeviceStateTreeComponent::IsRestoring(const FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition)
{
	// 트리 시작(재시작 포함)은 엔진이 소스 상태를 비워 둔다.
	if (!Transition.SourceStateID.IsValid())
	{
		return true;
	}

	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	const UWxDeviceStateTreeComponent* Component = Owner ? Owner->FindComponentByClass<UWxDeviceStateTreeComponent>() : nullptr;
	return Component && Component->bRestoringState;
}

void UWxDeviceStateTreeComponent::StartLogic()
{
	Super::StartLogic();
	SynchronizeAfterStart();
}

void UWxDeviceStateTreeComponent::RestartLogic()
{
	Super::RestartLogic();
	SynchronizeAfterStart();
}

void UWxDeviceStateTreeComponent::SynchronizeAfterStart()
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		// 틱 없이 잠드는 첫 상태('작동 대기' 뿐인 상태)도 발행되게 한다. InitialState 로의 전이는 그 요청이 깨운 틱이 발행한다.
		if (InitialState == RootInitialStateName || !EnterState(FGameplayTag::RequestGameplayTag(InitialState, false), true))
		{
			PublishState();
		}
	}
	else if (StateSnapshot.EntrySerial != 0)
	{
		// BeginPlay 보다 먼저 도착한 스냅샷.
		ApplyInteractor();
		EnterState(StateSnapshot.StateTag, true);
	}
}

void UWxDeviceStateTreeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 복원으로 요청한 전이는 이 틱의 전이 처리에서 적용됐다.
	bRestoringState = false;

	if (GetOwnerRole() == ROLE_Authority)
	{
		PublishState();
	}
}

void UWxDeviceStateTreeComponent::PublishState()
{
	const FGameplayTag ActiveTag = FindActiveStateTag();
	if (!ActiveTag.IsValid() || ActiveTag == StateSnapshot.StateTag)
	{
		return;
	}

	StateSnapshot.StateTag = ActiveTag;
	if (++StateSnapshot.EntrySerial == 0)
	{
		++StateSnapshot.EntrySerial;
	}
	const AWxDevice* Device = Cast<AWxDevice>(GetOwner());
	ACharacter* Interactor = Device ? Device->GetInteractingCharacter() : nullptr;
	StateSnapshot.Interactor = IsValid(Interactor) ? Interactor : nullptr;
	StateSnapshot.SelectedOptionValue = Device ? Device->GetSelectedOptionValue() : INDEX_NONE;
	GetOwner()->ForceNetUpdate();
	UE_LOG(LogWxWorld, Verbose, TEXT("Device publish: %s"), *DescribeSynchronization());
}

void UWxDeviceStateTreeComponent::OnRep_StateSnapshot(const FWxDeviceStateSnapshot& Previous)
{
	// 당사자 참조가 뒤늦게 해소되면 같은 번호로 한 번 더 통지된다.
	ApplyInteractor();
	UE_LOG(LogWxWorld, Verbose, TEXT("Device receive: %s"), *DescribeSynchronization());

	// 트리가 아직 시작 전이면 StartLogic 이 이 스냅샷을 적용한다.
	if (Previous.EntrySerial == StateSnapshot.EntrySerial || GetStateTreeRunStatus() == EStateTreeRunStatus::Unset)
	{
		return;
	}

	const bool bLive = Previous.EntrySerial != 0 && StateSnapshot.EntrySerial == Previous.EntrySerial + 1;
	if (bLive && FindActiveStateTag() == StateSnapshot.StateTag)
	{
		// 클라가 제 타이머로 먼저 도착했다.
		return;
	}

	EnterState(StateSnapshot.StateTag, !bLive);
}

void UWxDeviceStateTreeComponent::ApplyInteractor()
{
	if (AWxDevice* Device = Cast<AWxDevice>(GetOwner()))
	{
		// 새 상태가 이전 당사자를 사용하지 않게 한다.
		Device->InteractingCharacter = IsValid(StateSnapshot.Interactor) ? StateSnapshot.Interactor.Get() : nullptr;
		Device->SelectedOptionValue = StateSnapshot.SelectedOptionValue;
	}
}

bool UWxDeviceStateTreeComponent::EnterState(FGameplayTag Tag, bool bRestore)
{
	const UStateTree* Asset = StateTreeRef.GetStateTree();
	const FStateTreeStateHandle State = Asset ? Asset->GetStateHandleFromGameplayTag(Tag, UStateTree::EStateGameplayTagQueryMethod::MatchesExact) : FStateTreeStateHandle::Invalid;
	if (!State.IsValid())
	{
		UE_LOG(LogWxWorld, Error, TEXT("Device: 태그 '%s' 상태가 루트 에셋에 없다 — %s"), *Tag.ToString(), *DescribeSynchronization());
		return false;
	}

	// 끝난 트리는 전이 요청을 받지 않는다. 순정 IsRunning 은 스스로 끝난 트리(Tree Succeeded 전이)에도 참이라 실행 상태를 직접 본다.
	if (GetStateTreeRunStatus() != EStateTreeRunStatus::Running)
	{
		Super::RestartLogic();
	}

	FStateTreeExecutionContext Context(*GetOwner(), *Asset, InstanceData);
	if (GetStateTreeRunStatus() != EStateTreeRunStatus::Running || !SetContextRequirements(Context))
	{
		UE_LOG(LogWxWorld, Error, TEXT("Device: 트리를 시작하지 못해 '%s' 로 들어갈 수 없다 — %s"), *Tag.ToString(), *DescribeSynchronization());
		return false;
	}

	// 요청은 다음 틱의 전이 처리에서 적용되므로 복원 표시는 그 틱이 끝날 때 내린다.
	bRestoringState = bRestoringState || bRestore;
	Context.RequestTransition(State, EStateTreeTransitionPriority::Critical);
	UE_LOG(LogWxWorld, Verbose, TEXT("Device request %s target=%s: %s"), bRestore ? TEXT("restore") : TEXT("live"), *Tag.ToString(), *DescribeSynchronization());

	return true;
}

FGameplayTag UWxDeviceStateTreeComponent::FindActiveStateTag() const
{
	const UStateTree* Asset = StateTreeRef.GetStateTree();
	const FStateTreeExecutionState* Execution = InstanceData.GetExecutionState();
	for (int32 FrameIndex = Execution->ActiveFrames.Num() - 1; FrameIndex >= 0; --FrameIndex)
	{
		const FStateTreeExecutionFrame& Frame = Execution->ActiveFrames[FrameIndex];
		// 받는 쪽 EnterState 는 루트 에셋에서만 태그를 찾으므로 링크된 에셋의 태그는 건너뛴다.
		if (!Frame.StateTree || Frame.StateTree != Asset)
		{
			continue;
		}
		for (int32 Index = Frame.ActiveStates.Num() - 1; Index >= 0; --Index)
		{
			const FCompactStateTreeState* State = Frame.StateTree->GetStateFromHandle(Frame.ActiveStates[Index]);
			if (State && State->Tag.IsValid())
			{
				return State->Tag;
			}
		}
	}

	return FGameplayTag();
}

FString UWxDeviceStateTreeComponent::DescribeSynchronization() const
{
	return FString::Printf(TEXT("%s role=%s local=%s authority=%s/%u run=%s interactor=%s value=%d"),
		*GetNameSafe(GetOwner()), GetOwnerRole() == ROLE_Authority ? TEXT("Authority") : TEXT("Client"),
		*FindActiveStateTag().ToString(), *StateSnapshot.StateTag.ToString(), StateSnapshot.EntrySerial,
		*UEnum::GetValueAsString(GetStateTreeRunStatus()), *GetNameSafe(StateSnapshot.Interactor), StateSnapshot.SelectedOptionValue);
}

#if WITH_GAMEPLAY_DEBUGGER
FString UWxDeviceStateTreeComponent::GetDebugInfoString() const
{
	return Super::GetDebugInfoString() + TEXT("\n") + DescribeSynchronization();
}
#endif

#if WITH_EDITOR
TArray<FPropertyTextFName> UWxDeviceStateTreeComponent::GetInitialStateOptions() const
{
	TArray<FPropertyTextFName> Options;
	Options.Add({ .ValueString = RootInitialStateName, .DisplayName = FText::FromName(RootInitialStateName) });
	if (const UStateTree* Asset = StateTreeRef.GetStateTree())
	{
		for (const FCompactStateTreeState& State : Asset->GetStates())
		{
			if (State.Tag.IsValid())
			{
				Options.Add({ .ValueString = State.Tag.GetTagName(),
					.DisplayName = FText::Format(INVTEXT("{0} ({1})"), FText::FromName(State.Name), FText::FromName(State.Tag.GetTagName())) });
			}
		}
	}
	return Options;
}
#endif

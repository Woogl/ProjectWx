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

FGameplayTag UWxDeviceStateTreeComponent::GetStateTag() const
{
	return FGameplayTag::RequestGameplayTag(StateSnapshot.StateTagName, false);
}

bool UWxDeviceStateTreeComponent::IsRunning() const
{
	// 순정 bIsRunning은 자동 완료 때 갱신되지 않는다. 종료된 트리로 상호작용을 보내지 않는다.
	return Super::IsRunning() && GetStateTreeRunStatus() == EStateTreeRunStatus::Running;
}

bool UWxDeviceStateTreeComponent::IsRestoringState() const
{
	return bRestoringState;
}

void UWxDeviceStateTreeComponent::BeginPlay()
{
	if (GetOwnerRole() == ROLE_Authority && InitialState != RootInitialStateName)
	{
		InitialTarget = FGameplayTag::RequestGameplayTag(InitialState, false);
		if (!HasState(InitialTarget))
		{
			UE_LOG(LogWxWorld, Warning, TEXT("Device(%s): InitialState '%s' not found; using Root."), *GetNameSafe(GetOwner()), *InitialState.ToString());
			InitialTarget = FGameplayTag();
		}
	}

	Super::BeginPlay();
}

void UWxDeviceStateTreeComponent::StartLogic()
{
	{
		TGuardValue<bool> RestoreGuard(bRestoringState, true);
		Super::StartLogic();
	}

	SynchronizeAfterStart();
}

void UWxDeviceStateTreeComponent::RestartLogic()
{
	{
		TGuardValue<bool> RestoreGuard(bRestoringState, true);
		Super::RestartLogic();
	}

	SynchronizeAfterStart();
}

void UWxDeviceStateTreeComponent::SynchronizeAfterStart()
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (InitialTarget.IsValid())
		{
			EnterState(InitialTarget, true);
		}
		else
		{
			// 틱 없이 잠드는 첫 상태('작동 대기' 뿐인 상태)도 발행되게 한다.
			PublishState();
		}
	}
	else if (StateSnapshot.EntrySerial != 0)
	{
		// BeginPlay 보다 먼저 도착한 스냅샷.
		ApplyInteractor();
		EnterState(GetStateTag(), true);
	}
}

void UWxDeviceStateTreeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 복원으로 요청한 전이는 이 틱의 전이 처리에서 적용됐다.
	bRestoringState = false;

	if (GetOwnerRole() == ROLE_Authority)
	{
		if (InitialTarget.IsValid())
		{
			if (FindActiveStateTag() != InitialTarget)
			{
				UE_LOG(LogWxWorld, Warning, TEXT("Device(%s): InitialState '%s' 로 들어가지 못했다 — 그 상태의 진입 조건을 확인."), *GetNameSafe(GetOwner()), *InitialTarget.ToString());
			}
			InitialTarget = FGameplayTag();
		}
		PublishState();
	}

	if (!IsRunning())
	{
		DisableTick();
	}
}

void UWxDeviceStateTreeComponent::PublishState()
{
	const FGameplayTag ActiveTag = FindActiveStateTag();
	if (!ActiveTag.IsValid() || ActiveTag.GetTagName() == StateSnapshot.StateTagName)
	{
		return;
	}

	StateSnapshot.StateTagName = ActiveTag.GetTagName();
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

	const FGameplayTag AuthorityTag = GetStateTag();
	const bool bLive = Previous.EntrySerial != 0 && StateSnapshot.EntrySerial == Previous.EntrySerial + 1;
	if (bLive && FindActiveStateTag() == AuthorityTag)
	{
		// 클라가 제 타이머로 먼저 도착했다.
		return;
	}

	EnterState(AuthorityTag, !bLive);
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

void UWxDeviceStateTreeComponent::EnterState(FGameplayTag Tag, bool bRestore)
{
	const UStateTree* Asset = StateTreeRef.GetStateTree();
	const FStateTreeStateHandle State = Asset ? Asset->GetStateHandleFromGameplayTag(Tag, UStateTree::EStateGameplayTagQueryMethod::MatchesExact) : FStateTreeStateHandle::Invalid;
	if (!State.IsValid())
	{
		UE_LOG(LogWxWorld, Error, TEXT("Device: 태그 '%s' 상태가 루트 에셋에 없다 — %s"), *Tag.ToString(), *DescribeSynchronization());
		return;
	}

	if (!IsRunning())
	{
		// 끝난 트리는 전이 요청을 받지 않는다.
		TGuardValue<bool> RestoreGuard(bRestoringState, true);
		Super::RestartLogic();
	}

	FStateTreeExecutionContext Context(*GetOwner(), *Asset, InstanceData);
	if (!IsRunning() || !SetContextRequirements(Context))
	{
		UE_LOG(LogWxWorld, Error, TEXT("Device: 트리를 시작하지 못해 '%s' 로 들어갈 수 없다 — %s"), *Tag.ToString(), *DescribeSynchronization());
		return;
	}

	// 요청은 다음 틱의 전이 처리에서 적용되므로 복원 표시는 그 틱이 끝날 때 내린다.
	bRestoringState = bRestoringState || bRestore;
	Context.RequestTransition(State, EStateTreeTransitionPriority::Critical);
	UE_LOG(LogWxWorld, Verbose, TEXT("Device request %s target=%s: %s"), bRestore ? TEXT("restore") : TEXT("live"), *Tag.ToString(), *DescribeSynchronization());
}

FGameplayTag UWxDeviceStateTreeComponent::FindActiveStateTag() const
{
	const FStateTreeExecutionState* Execution = InstanceData.GetExecutionState();
	for (int32 FrameIndex = Execution->ActiveFrames.Num() - 1; FrameIndex >= 0; --FrameIndex)
	{
		const FStateTreeExecutionFrame& Frame = Execution->ActiveFrames[FrameIndex];
		if (!Frame.StateTree)
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

bool UWxDeviceStateTreeComponent::HasState(FGameplayTag Tag) const
{
	const UStateTree* Asset = StateTreeRef.GetStateTree();
	return Asset && Tag.IsValid() && Asset->GetStateHandleFromGameplayTag(Tag, UStateTree::EStateGameplayTagQueryMethod::MatchesExact).IsValid();
}

FString UWxDeviceStateTreeComponent::DescribeSynchronization() const
{
	return FString::Printf(TEXT("%s role=%s local=%s authority=%s/%u run=%s interactor=%s value=%d"),
		*GetNameSafe(GetOwner()), GetOwnerRole() == ROLE_Authority ? TEXT("Authority") : TEXT("Client"),
		*FindActiveStateTag().ToString(), *StateSnapshot.StateTagName.ToString(), StateSnapshot.EntrySerial,
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

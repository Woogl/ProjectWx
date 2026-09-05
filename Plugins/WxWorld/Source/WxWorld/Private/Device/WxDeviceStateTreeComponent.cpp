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
	constexpr uint8 MaxSyncAttempts = 3;
}

void FWxDeviceExecutionExtension::ScheduleNextTick(const FContextParameters& Context, const FNextTickArguments& Args)
{
	if (Component)
	{
		Component->ConditionalEnableTick();
	}
}

void FWxDeviceExecutionExtension::OnBeginApplyTransition(const FContextParameters& Context, const FStateTreeTransitionResult& Transition)
{
	if (Component)
	{
		Component->HandleBeginApplyTransition(Context, Transition);
	}
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

void UWxDeviceStateTreeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	
	Super::EndPlay(EndPlayReason);
}

void UWxDeviceStateTreeComponent::StartLogic()
{
	if (InstanceData.GetExecutionState()->CurrentPhase != EStateTreeUpdatePhase::Unset)
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Device(%s): StartLogic ignored during StateTree update."), *GetNameSafe(GetOwner()));
		return;
	}
	SyncAttempts = 0;
	SyncFailure.Reset();
	bRequestPending = false;
	ApplyInteractor();
	TGuardValue<bool> RestoreGuard(bRestoringState, true);
	
	Super::StartLogic();
	
	InstallExecutionObserver();
	ObserveActiveState();
	Synchronize();
}

void UWxDeviceStateTreeComponent::RestartLogic()
{
	if (InstanceData.GetExecutionState()->CurrentPhase != EStateTreeUpdatePhase::Unset)
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Device(%s): RestartLogic ignored during StateTree update."), *GetNameSafe(GetOwner()));
		return;
	}
	SyncAttempts = 0;
	SyncFailure.Reset();
	bRequestPending = false;
	ApplyInteractor();
	TGuardValue<bool> RestoreGuard(bRestoringState, true);
	
	Super::RestartLogic();
	
	InstallExecutionObserver();
	ObserveActiveState();
	Synchronize();
}

void UWxDeviceStateTreeComponent::InstallExecutionObserver()
{
	// StartTree가 순정 확장을 새로 만들므로 Start/Restart 직후마다 교체한다.
	FWxDeviceExecutionExtension Extension;
	Extension.Component = this;
	InstanceData.GetMutableExecutionState()->ExecutionExtension = TInstancedStruct<FStateTreeExecutionExtension>::Make<FWxDeviceExecutionExtension>(MoveTemp(Extension));
	ObservedFrameID = UE::StateTree::FActiveFrameID();
	ObservedStateID = UE::StateTree::FActiveStateID();
	LastEnteredTag = FGameplayTag();
	bPendingReselect = false;
}

void UWxDeviceStateTreeComponent::StopLogic(const FString& Reason)
{
	ObserveActiveState();
	
	Super::StopLogic(Reason);
	
	if (GetOwnerRole() == ROLE_Authority && !bEndingPlay && !InitialTarget.IsValid())
	{
		PublishAuthorityState();
	}
}

void UWxDeviceStateTreeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	const bool bRestore = InitialTarget.IsValid() || (GetOwnerRole() != ROLE_Authority && (!bHasAppliedSnapshot || (bRequestPending && !bRequestIsLive)));
	TGuardValue<bool> RestoreGuard(bRestoringState, bRestore);
	
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	ObserveActiveState();
	Synchronize();
	if (!IsRunning())
	{
		DisableTick();
	}
}

void UWxDeviceStateTreeComponent::HandleBeginApplyTransition(const FStateTreeExecutionExtension::FContextParameters& Context, const FStateTreeTransitionResult& Transition)
{
	// 앞 전이로 들어간 상태를 다음 ExitState가 지우기 전에 확보한다. 같은 틱의 진입→완료도 포함된다.
	ObserveActiveState();
	bPendingReselect = false;
	if (Transition.TargetState.IsCompletionState())
	{
		return;
	}

	// 기본 선택 규칙은 재선택 시 인스턴스 ID를 유지한다. 태그 상태/조상을 직접 대상으로 삼은
	// 적용 전이만 별도로 기록한다. 태그 아래 자식 사이 이동은 태그 상태의 재진입이 아니다.
	for (const FStateTreeExecutionFrame& Frame : Context.InstanceData.GetExecutionState().ActiveFrames)
	{
		for (int32 Index = 0; Index < Frame.ActiveStates.Num(); ++Index)
		{
			if (Frame.FrameID == Transition.SourceFrameID && Frame.ActiveStates[Index] == Transition.TargetState)
			{
				bPendingReselect = true;
			}
			if (Frame.FrameID == ObservedFrameID && Frame.ActiveStates.StateIDs[Index] == ObservedStateID)
			{
				return;
			}
		}
	}
}

void UWxDeviceStateTreeComponent::ObserveActiveState()
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
			if (!State || !State->Tag.IsValid())
			{
				continue;
			}

			if (Frame.FrameID != ObservedFrameID || Frame.ActiveStates.StateIDs[Index] != ObservedStateID || bPendingReselect)
			{
				LastEnteredTag = State->Tag;
				ObservedFrameID = Frame.FrameID;
				ObservedStateID = Frame.ActiveStates.StateIDs[Index];
				++LocalEntrySerial;
				UE_LOG(LogWxWorld, VeryVerbose, TEXT("Device(%s): observed entry %u tag=%s"), *GetNameSafe(GetOwner()), LocalEntrySerial, *LastEnteredTag.ToString());
			}
			bPendingReselect = false;
			return;
		}
	}
	
	// 마지막 유효 태그는 완료 스냅샷에 필요하다. 현재 활성 프레임 ID만 비운다.
	ObservedFrameID = UE::StateTree::FActiveFrameID();
	ObservedStateID = UE::StateTree::FActiveStateID();
	bPendingReselect = false;
}

void UWxDeviceStateTreeComponent::Synchronize()
{
	if (bEndingPlay)
	{
		return;
	}
	
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (InitialTarget.IsValid())
		{
			if (LastEnteredTag == InitialTarget)
			{
				InitialTarget = FGameplayTag();
				bRequestPending = false;
				SyncAttempts = 0;
			}
			else
			{
				RequestState(InitialTarget);
				return;
			}
		}
		PublishAuthorityState();
	}
	else
	{
		FollowAuthorityState();
	}
}

void UWxDeviceStateTreeComponent::PublishAuthorityState()
{
	const EStateTreeRunStatus Status = GetStateTreeRunStatus();
	const bool bNewEntry = PublishedLocalEntrySerial != LocalEntrySerial;
	if (!bNewEntry && StateSnapshot.EntrySerial != 0 && StateSnapshot.RunStatus == Status)
	{
		return;
	}

	if (bNewEntry || StateSnapshot.EntrySerial == 0)
	{
		++StateSnapshot.EntrySerial;
		if (StateSnapshot.EntrySerial == 0)
		{
			++StateSnapshot.EntrySerial;
		}
	}
	
	PublishedLocalEntrySerial = LocalEntrySerial;
	StateSnapshot.StateTagName = LastEnteredTag.GetTagName();
	StateSnapshot.RunStatus = Status;
	const AWxDevice* Device = Cast<AWxDevice>(GetOwner());
	StateSnapshot.Interactor = Device ? Device->GetInteractingCharacter() : nullptr;
	StateSnapshot.bHasInteractor = StateSnapshot.Interactor != nullptr;
	GetOwner()->ForceNetUpdate();
	UE_LOG(LogWxWorld, Verbose, TEXT("Device publish: %s"), *DescribeSynchronization());
}

void UWxDeviceStateTreeComponent::OnRep_StateSnapshot(const FWxDeviceStateSnapshot& Previous)
{
	if (Previous.EntrySerial != StateSnapshot.EntrySerial || Previous.StateTagName != StateSnapshot.StateTagName)
	{
		SyncAttempts = 0;
		SyncFailure.Reset();
		bRequestPending = false;
	}
	else if (Previous.RunStatus != StateSnapshot.RunStatus)
	{
		// 동일 진입의 완료 통지가 뒤따라 와도 이미 요청한 진입을 다시 요청하지 않는다.
		SyncAttempts = 0;
		SyncFailure.Reset();
	}
	
	ApplyInteractor();
	// 수신만으로 정지된 트리를 틱하지 않는다. 복구가 필요하면 Restart/RequestTransition이 틱을 예약한다.
	// 실행 중 재진입한 통지는 현재 틱 끝의 Synchronize에서 처리한다.
	if ((HasBegunPlay() || GetStateTreeRunStatus() != EStateTreeRunStatus::Unset)
		&& InstanceData.GetExecutionState()->CurrentPhase == EStateTreeUpdatePhase::Unset && !bRequestPending)
	{
		ObserveActiveState();
		Synchronize();
	}
	UE_LOG(LogWxWorld, Verbose, TEXT("Device receive: %s"), *DescribeSynchronization());
}

void UWxDeviceStateTreeComponent::ApplyInteractor()
{
	if (StateSnapshot.bHasInteractor && !IsValid(StateSnapshot.Interactor))
	{
		// 참조 해소를 기다리는 동안 현재 실행 중인 상태의 당사자를 덮지 않는다.
		return;
	}
	
	if (GetOwnerRole() != ROLE_Authority && StateSnapshot.EntrySerial != 0)
	{
		if (AWxDevice* Device = Cast<AWxDevice>(GetOwner()))
		{
			Device->InteractingCharacter = StateSnapshot.bHasInteractor ? StateSnapshot.Interactor.Get() : nullptr;
		}
	}
}

void UWxDeviceStateTreeComponent::FollowAuthorityState()
{
	if (StateSnapshot.EntrySerial == 0 || !SyncFailure.IsEmpty())
	{
		return;
	}
	
	if (StateSnapshot.bHasInteractor && !IsValid(StateSnapshot.Interactor))
	{
		// 기본 객체 프로퍼티 복제가 참조를 추적한다. 매핑 완료 후 RepNotify가 다시 적용한다.
		return;
	}
	
	ApplyInteractor();
	const FGameplayTag TargetTag = GetStateTag();
	if (!TargetTag.IsValid())
	{
		if (StateSnapshot.RunStatus != EStateTreeRunStatus::Running)
		{
			Super::StopLogic(TEXT("Authority completed without a tagged state"));
		}
		return;
	}
	
	if (!HasState(TargetTag))
	{
		FailSynchronization(TEXT("Authority tag is missing from the local root asset"));
		return;
	}

	const bool bRequestApplied = bRequestPending && LocalEntrySerial != RequestedAtLocalEntry && LastEnteredTag == TargetTag;
	const bool bNewEntry = bHasAppliedSnapshot && AppliedEntrySerial != StateSnapshot.EntrySerial;
	const bool bAtTarget = LastEnteredTag == TargetTag;
	if (bRequestApplied || (bAtTarget && !bNewEntry && !bRequestPending))
	{
		bHasAppliedSnapshot = true;
		AppliedEntrySerial = StateSnapshot.EntrySerial;
		bRequestPending = false;
		if (StateSnapshot.RunStatus != EStateTreeRunStatus::Running)
		{
			// 마지막 상태의 진입 태스크까지 적용한 뒤 정지한다.
			Super::StopLogic(TEXT("Authority completed"));
			return;
		}
		if (IsRunning())
		{
			return;
		}
	}
	
	// 이미 적용한 진입 안의 미태그 시퀀스는 과거 태그로 되감지 않는다.
	if (!bNewEntry && bHasAppliedSnapshot && AppliedEntrySerial == StateSnapshot.EntrySerial && IsRunning() && !ObservedStateID.IsValid())
	{
		return;
	}
	RequestState(TargetTag);
}

void UWxDeviceStateTreeComponent::RequestState(FGameplayTag TargetTag)
{
	if (!SyncFailure.IsEmpty())
	{
		return;
	}
	
	if (!HasState(TargetTag))
	{
		FailSynchronization(TEXT("Target tag is missing from the root asset"));
		return;
	}
	
	if (SyncAttempts >= MaxSyncAttempts)
	{
		FailSynchronization(TEXT("Transition rejected or local tree repeatedly completed (attempt limit)"));
		return;
	}
	
	bRequestIsLive = GetOwnerRole() != ROLE_Authority && bHasAppliedSnapshot && AppliedEntrySerial != StateSnapshot.EntrySerial && SyncAttempts == 0;
	++SyncAttempts;

	if (!IsRunning())
	{
		TGuardValue<bool> RestoreGuard(bRestoringState, true);
		// 엔진 전이 요청은 완료된 트리를 시작하지 않는다. public RestartLogic의 재시도 초기화는 피한다.
		Super::RestartLogic();
		InstallExecutionObserver();
		ObserveActiveState();
	}
	
	if (!IsRunning())
	{
		FailSynchronization(TEXT("StateTree could not restart; check asset, schema and start-state tasks"));
		return;
	}

	const UStateTree* Asset = StateTreeRef.GetStateTree();
	FStateTreeExecutionContext Context(*GetOwner(), *Asset, InstanceData);
	if (!SetContextRequirements(Context))
	{
		FailSynchronization(TEXT("StateTree context requirements failed"));
		return;
	}
	
	RequestedAtLocalEntry = LocalEntrySerial;
	bRequestPending = true;
	Context.RequestTransition(Asset->GetStateHandleFromGameplayTag(TargetTag, UStateTree::EStateGameplayTagQueryMethod::MatchesExact), EStateTreeTransitionPriority::Critical);
	UE_LOG(LogWxWorld, Verbose, TEXT("Device request target=%s: %s"), *TargetTag.ToString(), *DescribeSynchronization());
}

void UWxDeviceStateTreeComponent::FailSynchronization(const FString& Reason)
{
	if (SyncFailure.IsEmpty())
	{
		SyncFailure = Reason;
		UE_LOG(LogWxWorld, Error, TEXT("Device synchronization stopped: %s"), *DescribeSynchronization());
	}
}

bool UWxDeviceStateTreeComponent::HasState(FGameplayTag Tag) const
{
	const UStateTree* Asset = StateTreeRef.GetStateTree();
	return Asset && Tag.IsValid() && Asset->GetStateHandleFromGameplayTag(Tag, UStateTree::EStateGameplayTagQueryMethod::MatchesExact).IsValid();
}

FString UWxDeviceStateTreeComponent::DescribeSynchronization() const
{
	return FString::Printf(TEXT("%s role=%s local=%s/%u target=%s/%u applied=%u run=%s authorityRun=%s attempts=%u interactor=%s waitingInteractor=%d initial=%s error=%s"),
		*GetNameSafe(GetOwner()), GetOwnerRole() == ROLE_Authority ? TEXT("Authority") : TEXT("Client"),
		*LastEnteredTag.ToString(), LocalEntrySerial, *StateSnapshot.StateTagName.ToString(), StateSnapshot.EntrySerial,
		AppliedEntrySerial, *UEnum::GetValueAsString(GetStateTreeRunStatus()), *UEnum::GetValueAsString(StateSnapshot.RunStatus),
		SyncAttempts, *GetNameSafe(StateSnapshot.Interactor), StateSnapshot.bHasInteractor && !IsValid(StateSnapshot.Interactor),
		*InitialTarget.ToString(), *SyncFailure);
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

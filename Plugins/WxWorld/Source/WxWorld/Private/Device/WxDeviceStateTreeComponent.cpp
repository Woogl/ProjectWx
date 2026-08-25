// Copyright Woogle. All Rights Reserved.

#include "Device/WxDeviceStateTreeComponent.h"

#include "Net/UnrealNetwork.h"
#include "StateTree.h"
#include "StateTreeExecutionContext.h"
#include "WxWorldModule.h"

namespace
{
	/** InitialState 의 예약어 — 에셋의 루트 기본 상태에서 시작한다는 뜻. 상태 Tag 는 계층 이름(Device.x.y)이라 이 한 단어와 겹치지 않는다. */
	const FName RootInitialStateName = TEXT("Root");
}

UWxDeviceStateTreeComponent::UWxDeviceStateTreeComponent()
{
	// StateTag 복제와 재진입 멀티캐스트가 이 컴포넌트를 타므로 복제 서브오브젝트로 등록한다.
	SetIsReplicatedByDefault(true);

	InitialState = RootInitialStateName;
}

void UWxDeviceStateTreeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWxDeviceStateTreeComponent, StateTag);
}

FGameplayTag UWxDeviceStateTreeComponent::GetStateTag() const
{
	return StateTag;
}

void UWxDeviceStateTreeComponent::NotifyInteractionPending()
{
	bPendingInteractResolve = true;
}

void UWxDeviceStateTreeComponent::NotifySaveRestored()
{
	// 복원은 서버 권위에서만 상태를 움직인다 — 클라 수렴은 복제 추종이 담당한다. 태그 없는 기록은 따라갈 곳이 없다.
	if (GetOwnerRole() != ROLE_Authority || !StateTag.IsValid())
	{
		return;
	}

	bFollowRestoredState = true;

	// 월드 초기화 복원(BeginPlay 전)은 곧 BeginPlay 가 트리를 열며 동기화를 시작한다. 실행 중(스트리밍 인 복원)이면 잠든 틱만 깨워 같은 수렴 경로에 태운다.
	ConditionalEnableTick();
}

void UWxDeviceStateTreeComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SyncStateWithTree();
}

void UWxDeviceStateTreeComponent::StopLogic(const FString& Reason)
{
	// 종착 상태에 들어간 그 틱에 트리가 멈추는 경우까지 잡으려면 정지 전에 한 번 더 알려야 한다.
	if (bIsRunning)
	{
		SyncStateWithTree();
	}

	Super::StopLogic(Reason);
}

void UWxDeviceStateTreeComponent::BeginPlay()
{
	// 저장된 상태가 없으면 초기 상태를 권위 값으로 삼는다 — 이후는 세이브 복원과 같은 수렴 경로다.
	// 월드 초기 로드 복원은 BeginPlay 전에 끝나므로 여기서 비어 있으면 저장이 없는 것이다.
	// 스트리밍-인 복원은 이보다 늦게 와 StateTag 를 덮고 같은 추종으로 갈아탄다.
	if (GetOwnerRole() == ROLE_Authority && !StateTag.IsValid() && InitialState != RootInitialStateName)
	{
		const FGameplayTag InitialTag = FGameplayTag::RequestGameplayTag(InitialState, false);
		if (HasState(InitialTag))
		{
			StateTag = InitialTag;
			bFollowRestoredState = true;
		}
		else
		{
			UE_LOG(LogWxWorld, Warning, TEXT("Device(%s): 초기 상태 %s 를 에셋에서 찾지 못했다 — 에셋을 바꿨거나 상태 Tag 가 지워졌다. 루트 기본 상태로 시작한다."), *GetNameSafe(GetOwner()), *InitialState.ToString());
		}
	}

	Super::BeginPlay();

	if (!bIsRunning)
	{
		UE_LOG(LogWxWorld, Error, TEXT("Device(%s): State Tree 가 열리지 않았다 — 에셋 미지정, 스키마 불일치, 혹은 Start Logic Automatically 꺼짐."), *GetNameSafe(GetOwner()));
		return;
	}

	SyncStateWithTree();
}

void UWxDeviceStateTreeComponent::OnRep_StateTag()
{
	// 권위가 있다고 말하는 상태가 내 에셋에 없다 — 이 장치는 여기서부터 영영 어긋난 채로 남는다. 값이 바뀐 이 한 번에만 알린다(추종은 매 틱이라 거기서 알리면 로그가 잠긴다).
	if (StateTag.IsValid() && !HasState(StateTag))
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Device(%s): 권위 상태 %s 를 로컬 에셋에서 찾지 못했다 — 상태 Tag 가 지워졌거나 서버와 다른 에셋이다."), *GetNameSafe(GetOwner()), *StateTag.ToString());
	}

	// 아직 시작 전이면 곧 BeginPlay 가 이 값으로 동기화를 시작하므로 건드리지 않는다.
	if (!bIsRunning)
	{
		return;
	}

	ConditionalEnableTick();
}

void UWxDeviceStateTreeComponent::Multicast_ReenterState_Implementation(FGameplayTag ReenteredTag)
{
	// 권위는 자기 트리에서 이미 재진입했다.
	if (GetOwnerRole() == ROLE_Authority || !bIsRunning)
	{
		return;
	}

	// 그 사이 권위 상태가 더 움직였으면(도착 지연·유실) 지나간 상태의 재진입을 되살릴 이유가 없다 — 추종이 현재 상태로 데려간다.
	if (ReenteredTag != StateTag)
	{
		return;
	}

	UE_LOG(LogWxWorld, Verbose, TEXT("Device(%s): [클라] 권위 재진입 통지 — 상태 %s 재선택 요청"), *GetNameSafe(GetOwner()), *StateTag.ToString());
	RequestState(StateTag);
}

void UWxDeviceStateTreeComponent::SyncStateWithTree()
{
	if (GetOwnerRole() == ROLE_Authority && !bFollowRestoredState)
	{
		PublishAuthorityState();
	}
	else
	{
		FollowStateTag();
	}
}

void UWxDeviceStateTreeComponent::PublishAuthorityState()
{
	// 태그 없는 상태에 머무는 동안엔 마지막 유효 값을 유지한다 — 트리 정지·미태그 구간에서 저장 값이 지워지지 않게 한다.
	// 재진입 판정도 플래그를 든 채로 미룬다. 태그 있는 상태로 돌아오는 그 순간이 곧 그 상호작용이 만든 재진입이다.
	const FGameplayTag ActiveTag = GetActiveStateTag();
	if (!ActiveTag.IsValid())
	{
		return;
	}

	if (ActiveTag != StateTag)
	{
		UE_LOG(LogWxWorld, Verbose, TEXT("Device(%s): [권위] 상태 %s → %s"), *GetNameSafe(GetOwner()), *StateTag.ToString(), *ActiveTag.ToString());
		StateTag = ActiveTag;
	}
	else if (bPendingInteractResolve)
	{
		UE_LOG(LogWxWorld, Verbose, TEXT("Device(%s): [권위] 상태 %s 재진입"), *GetNameSafe(GetOwner()), *StateTag.ToString());
		Multicast_ReenterState(StateTag);
	}

	// 플래그가 선 뒤 처음 도는 이 지점은 트리가 발행을 소화한 뒤다 — 나머지 호출부(정지)에선 소화할 발행 자체가 없다.
	bPendingInteractResolve = false;
}

void UWxDeviceStateTreeComponent::FollowStateTag()
{
	// 복원 태그가 로컬 에셋에 없으면 영영 수렴할 수 없다 — 추종을 접고 트리 상태 발행으로 복귀해 장치가 조용히 죽는 것을 막는다.
	if (bFollowRestoredState && !HasState(StateTag))
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Device(%s): 복원 상태 %s 를 에셋에서 찾지 못했다 — 복원을 포기하고 트리 상태를 발행한다."), *GetNameSafe(GetOwner()), *StateTag.ToString());
		bFollowRestoredState = false;
		return;
	}

	// 태그가 한쪽이라도 비어 있으면 대조할 근거가 없다.
	//  - StateTag 가 비었다(첫 플레이 클라): 따라갈 곳이 없다.
	//  - 내가 태그 없는 구간에 있다: 수렴 이동 중이거나, 권위의 StateTag 도 마지막 유효 값(과거)을 들고 있어 대조하면 이미 지나온 상태로 되감긴다.
	const FGameplayTag ActiveTag = GetActiveStateTag();
	if (!StateTag.IsValid() || !ActiveTag.IsValid())
	{
		return;
	}

	if (ActiveTag == StateTag)
	{
		bFollowRestoredState = false;
		return;
	}

	// 어긋난 원인(지연·발행 유실·전이 조건 불일치·복원)을 가리지 않고 StateTag 값으로 끌어온다.
	UE_LOG(LogWxWorld, Verbose, TEXT("Device(%s): 로컬 %s ≠ 목표 %s — 추종 전이 요청"), *GetNameSafe(GetOwner()), *ActiveTag.ToString(), *StateTag.ToString());
	RequestState(StateTag);
}

FGameplayTag UWxDeviceStateTreeComponent::GetActiveStateTag()
{
	const UStateTree* Asset = StateTreeRef.GetStateTree();
	if (!Asset || !bIsRunning)
	{
		return FGameplayTag();
	}

	FStateTreeReadOnlyExecutionContext Context(GetOwner(), Asset, InstanceData);

	// 시퀀스를 자식 상태로 쪼갠 장치(엘리베이터)는 태그가 그 시퀀스를 감싼 상위 상태에 붙으므로, leaf 만 보면 놓친다.
	const TConstArrayView<FStateTreeExecutionFrame> Frames = Context.GetActiveFrames();
	for (int32 FrameIndex = Frames.Num() - 1; FrameIndex >= 0; --FrameIndex)
	{
		const FStateTreeExecutionFrame& Frame = Frames[FrameIndex];
		if (!Frame.StateTree)
		{
			continue;
		}

		for (int32 StateIndex = Frame.ActiveStates.Num() - 1; StateIndex >= 0; --StateIndex)
		{
			const FCompactStateTreeState* State = Frame.StateTree->GetStateFromHandle(Frame.ActiveStates[StateIndex]);
			if (State && State->Tag.IsValid())
			{
				return State->Tag;
			}
		}
	}

	return FGameplayTag();
}

void UWxDeviceStateTreeComponent::RequestState(FGameplayTag InStateTag)
{
	const UStateTree* Asset = StateTreeRef.GetStateTree();
	if (!Asset || !bIsRunning)
	{
		return;
	}

	const FStateTreeStateHandle TargetState = Asset->GetStateHandleFromGameplayTag(InStateTag, UStateTree::EStateGameplayTagQueryMethod::MatchesExact);
	if (!TargetState.IsValid())
	{
		return;
	}

	// 요청은 다음 트리 틱의 전이 처리 맨 앞에서 소비되며, 잠들어 있으면 엔진이 실행 확장을 통해 틱을 깨운다.
	FStateTreeExecutionContext Context(*GetOwner(), *Asset, InstanceData);
	if (SetContextRequirements(Context))
	{
		Context.RequestTransition(TargetState, EStateTreeTransitionPriority::Critical);
	}
}

bool UWxDeviceStateTreeComponent::HasState(FGameplayTag InStateTag) const
{
	const UStateTree* Asset = StateTreeRef.GetStateTree();
	if (!InStateTag.IsValid() || !Asset)
	{
		return false;
	}

	return Asset->GetStateHandleFromGameplayTag(InStateTag, UStateTree::EStateGameplayTagQueryMethod::MatchesExact).IsValid();
}

#if WITH_EDITOR
TArray<FPropertyTextFName> UWxDeviceStateTreeComponent::GetInitialStateOptions() const
{
	TArray<FPropertyTextFName> Options;
	Options.Add({ .ValueString = RootInitialStateName, .DisplayName = FText::FromName(RootInitialStateName) });

	// 컴파일된 상태 목록을 읽으므로 에셋이 미지정이거나 컴파일 전이면 Root 만 남는다.
	const UStateTree* Asset = StateTreeRef.GetStateTree();
	if (!Asset)
	{
		return Options;
	}

	for (const FCompactStateTreeState& State : Asset->GetStates())
	{
		if (State.Tag.IsValid())
		{
			Options.Add({ .ValueString = State.Tag.GetTagName(), .DisplayName = FText::FromName(State.Name) });
		}
	}

	return Options;
}
#endif

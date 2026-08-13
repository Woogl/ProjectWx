// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxGimmickStateTreeComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "StateTree.h"
#include "StateTreeExecutionContext.h"
#include "StructUtils/InstancedStruct.h"
#include "WxWorldModule.h"

#if WITH_EDITOR
#include "UObject/ObjectSaveContext.h"
#endif

void FWxGimmickStateTreeExecutionExtension::ScheduleNextTick(const FContextParameters& Context, const FNextTickArguments& Args)
{
	if (ensure(Component))
	{
		Component->NotifyTickRequested();
	}
}

UWxGimmickStateTreeComponent::UWxGimmickStateTreeComponent()
{
	// 초기 진입 스냅(위치·포즈·애니)은 각 태스크가 자체 수행한다.
	// 따라서 자동 시작에 맡기고 호스트는 명시 StartLogic 을 호출하지 않는다.
	SetStartLogicAutomatically(true);

	// 상태는 서버 권위이고 클라는 복제 값만 보고 따라간다 — 복제가 꺼지면 클라 기믹이 통째로 멈춘다.
	SetIsReplicatedByDefault(true);
}

void UWxGimmickStateTreeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWxGimmickStateTreeComponent, StateTag);
	DOREPLIFETIME(UWxGimmickStateTreeComponent, InteractingCharacter);
}

void UWxGimmickStateTreeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 상태 전이는 전부 트리 틱 안에서 일어나므로, 틱 직후 한 번 보면 모든 변화를 잡는다.
	// 권위와 클라가 같은 자리에서 방향만 반대로 움직인다 — 권위는 트리 → 복제 값, 클라는 복제 값 → 트리.
	// 클라 쪽을 OnRep 이 아니라 여기에 둔 것이 핵심이다. 도착 즉시 대조하면 대기 중인 발행을 아직 소화하지 못한 트리를 어긋난 것으로 오판한다.
	const AActor* Owner = GetOwner();
	if (Owner && Owner->HasAuthority())
	{
		PublishAuthorityState();
	}
	else
	{
		FollowAuthorityState();
	}
}

#if WITH_EDITOR
// 런타임엔 안정적인 액터 식별자가 없다(ActorGuid 는 에디터 전용 데이터). 그래서 에디터에서 값을 심어 에셋에 직렬화하고, 런타임은 그것을 읽기만 한다.
//
// 생성·등록 훅이 아니라 직렬화 직전에 심는 것이 핵심이다. 에디터의 액터 복제(Ctrl+W·Alt-드래그)와 붙여넣기는 T3D 텍스트 경로라 ImportObjectProperties 가 원본의 SaveId 를 그대로 덮어쓰는데,
// 저장 직전에 오너의 ActorGuid 로 재확정하면 생성 경로가 무엇이든 상관없어진다.
void UWxGimmickStateTreeComponent::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
	Super::PreSave(ObjectSaveContext);

	// 쿠킹·EditorDomain 등 사용자 편집이 개입할 수 없는 저장은 건드리지 않는다. 값은 맵 저장 때 이미 확정되어 패키지에 실려 있다.
	if (ObjectSaveContext.IsProceduralSave())
	{
		return;
	}

	// 오너 없이 저장되는 것은 블루프린트 클래스의 컴포넌트 템플릿이다. 배치 인스턴스가 아니라 키를 심을 대상이 아니고, 심으면 기존 값만 지운다.
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Modify() 는 부르지 않는다. 이미 저장 중이라 트랜잭션에 남길 이유가 없고, 오히려 저장 직후 패키지가 dirty 로 남는다.
	SaveId = Owner->GetActorGuid();
}
#endif

void UWxGimmickStateTreeComponent::StartLogic()
{
	StartTreeAtSavedState();
}

void UWxGimmickStateTreeComponent::RestartLogic()
{
	StartTreeAtSavedState();
}

void UWxGimmickStateTreeComponent::StopLogic(const FString& Reason)
{
	// 정지하면 활성 상태가 비어 더는 읽을 수 없다. 종착 상태에 들어간 그 틱에 트리가 멈추는 경우까지 잡으려면 여기서 한 번 더 기록해야 한다.
	PublishAuthorityState();

	Super::StopLogic(Reason);
}

bool UWxGimmickStateTreeComponent::IsInteractionMeshActive(const UPrimitiveComponent* Mesh) const
{
	// 맵에 파괴된 항목이 남아 있을 수 있으므로 null 끼리 맞아떨어지지 않게 막는다.
	if (!Mesh)
	{
		return false;
	}

	// 맵 키가 비const 포인터라 조회용으로만 const 를 벗긴다(맵을 통해 대상을 수정하지 않는다).
	return InteractionRegions.Contains(const_cast<UPrimitiveComponent*>(Mesh));
}

void UWxGimmickStateTreeComponent::OnInteracted(AActor* Interactor, const UActorComponent* Source)
{
	// 상호작용 어빌리티가 서버 권위에서만 부르지만, 상태를 움직이는 것은 권위 트리뿐이므로 한 번 더 가른다.
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	// 멈춘 트리엔 발행이 닿지 않는다. 소화될 일 없는 재진입 판정을 예약해 두면 다음 시작 때 헛재진입으로 새어 나간다.
	if (!IsRunning())
	{
		return;
	}

	// 맵 키가 비const 포인터라 조회용으로만 const 를 벗긴다(맵을 통해 대상을 수정하지 않는다).
	UPrimitiveComponent* Mesh = const_cast<UPrimitiveComponent*>(Cast<UPrimitiveComponent>(Source));

	// 지금 켜져 있는 영역만 발행할 자리를 가진다. 꺼진 영역은 애초에 스캔 후보에서 빠지지만, 여기서도 같은 기준으로 걸러 상태를 건드리지 않는다.
	const FWxGimmickInteractionRegion* Region = InteractionRegions.Find(Mesh);
	if (!Region)
	{
		return;
	}

	// 당사자는 복제로 각 피어에 전해진다 — 이동·몽타주 태스크가 모든 머신에서 같은 대상을 본다.
	InteractingCharacter = Cast<ACharacter>(Interactor);

	// 이 상호작용이 상태를 바꾸는지 아닌지는 트리가 발행을 소화해 봐야 안다. PublishAuthorityState 가 그 결과를 보고 판정한다.
	bPendingInteractResolve = true;

	// 눌린 영역의 발행자를 그대로 발행한다 — 영역마다 발행자가 달라 어느 상태로 갈지는 그 발행자를 지목한 전이가 혼자 정한다.
	// 트리 틱 밖에서 부르는 경로라 태스크가 남긴 약참조 컨텍스트를 쓴다. 잠든 트리는 이 발행이 깨우고, 발행 표식은 다음 전이 처리까지 보존된다.
	Region->Context.BroadcastDelegate(Region->Dispatcher);
}

FText UWxGimmickStateTreeComponent::GetInteractionPrompt(const UActorComponent* Source) const
{
	// 맵 키가 비const 포인터라 조회용으로만 const 를 벗긴다(맵을 통해 대상을 수정하지 않는다).
	UPrimitiveComponent* Mesh = const_cast<UPrimitiveComponent*>(Cast<UPrimitiveComponent>(Source));

	// ST 가 이 영역에 세팅한 상태별 프롬프트가 전부다. 태스크에서 문구를 지정하지 않았으면 표시할 것이 없으므로 공백을 답한다.
	const FWxGimmickInteractionRegion* Region = InteractionRegions.Find(Mesh);
	return Region ? Region->Prompt : FText::GetEmpty();
}

FGuid UWxGimmickStateTreeComponent::GetSaveId() const
{
	return SaveId;
}

void UWxGimmickStateTreeComponent::OnSaveRestored()
{
	// 스트리밍 인 복원은 StateTag 가 트리 시작 이후 직접 직렬화로 들어온다. 실행 중이면 저장된 상태에서 다시 열어 그 상태로 스냅 진입한다.
	// 미실행(월드 초기화 복원)이면 곧 BeginPlay 가 같은 경로로 시작하므로 건드리지 않는다(이중 처리 방지).
	if (IsRunning())
	{
		RestartLogic();
	}
}

void UWxGimmickStateTreeComponent::SetInteractionEnabled(UPrimitiveComponent* Mesh, bool bEnabled, const FWxGimmickInteractionRegion& Region)
{
	if (!Mesh)
	{
		return;
	}

	// 맵의 멤버십이 곧 활성 상태다. 꺼진 영역은 스캐너의 후보 수집에서 빠져 다음 스캔에 프롬프트·외곽선이 정리된다.
	if (!bEnabled)
	{
		InteractionRegions.Remove(Mesh);
		return;
	}

	InteractionRegions.Add(Mesh, Region);
}

ACharacter* UWxGimmickStateTreeComponent::GetInteractingCharacter() const
{
	return InteractingCharacter;
}

FGameplayTag UWxGimmickStateTreeComponent::GetActiveStateTag()
{
	const UStateTree* Asset = StateTreeRef.GetStateTree();
	if (!Asset || !IsRunning())
	{
		return FGameplayTag();
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return FGameplayTag();
	}

	FStateTreeReadOnlyExecutionContext Context(Owner, Asset, InstanceData);

	// 가장 깊은 활성 상태에서 위로 올라가며 처음 만나는 태그를 답한다.
	// 시퀀스를 자식 상태로 쪼갠 기믹(엘리베이터)은 태그가 그 시퀀스를 감싼 상위 상태에 붙으므로, leaf 만 보면 놓친다.
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

void UWxGimmickStateTreeComponent::NotifyTickRequested()
{
	ConditionalEnableTick();
}

void UWxGimmickStateTreeComponent::BeginPlay()
{
	Super::BeginPlay();

	// 오너가 복제되지 않으면 StateTag 도 당사자도 나가지 않아 클라 수렴이 통째로 죽는다.
	// 로컬 플레이에선 정상으로 보여 발견이 늦으므로 여기서 알린다.
	const AActor* Owner = GetOwner();
	if (Owner && !Owner->GetIsReplicated())
	{
		UE_LOG(LogWxWorld, Error, TEXT("Gimmick: %s 의 Replicates 가 꺼져 있어 상태 복제가 동작하지 않는다 — 클라 기믹이 서버를 따라가지 못한다."), *Owner->GetName());
	}
}

void UWxGimmickStateTreeComponent::OnRep_StateTag()
{
	// 권위가 있다고 말하는 상태가 내 에셋에 없다 — 이 기믹은 여기서부터 영영 어긋난 채로 남는다. 값이 바뀐 이 한 번에만 알린다(추종은 매 틱이라 거기서 알리면 로그가 잠긴다).
	if (StateTag.IsValid() && !ResolveStateTag().IsValid())
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Gimmick(%s): 권위 상태 %s 를 에셋 %s 에서 찾지 못했다 — 상태 Tag 가 지워졌거나 서버와 다른 에셋이다."),
			*GetNameSafe(GetOwner()), *StateTag.ToString(), *GetNameSafe(StateTreeRef.GetStateTree()));
	}

	// 아직 시작 전이면 곧 BeginPlay 가 이 값으로 시작하므로 건드리지 않는다.
	if (!IsRunning())
	{
		return;
	}

	// 여기서 대조하지 않는다 — 대기 중인 발행을 아직 소화하지 못한 트리를 어긋난 것으로 오판하게 된다.
	// 잠들어 틱이 꺼져 있으면 추종 판정이 돌 자리가 없으므로 깨우기만 하고, 대조는 트리 틱 뒤 FollowAuthorityState 에 맡긴다.
	ConditionalEnableTick();
}

void UWxGimmickStateTreeComponent::Multicast_ReenterState_Implementation(FGameplayTag ReenteredTag)
{
	// 권위는 자기 트리에서 이미 재진입했다.
	const AActor* Owner = GetOwner();
	if (!Owner || Owner->HasAuthority() || !IsRunning())
	{
		return;
	}

	// 그 사이 권위 상태가 더 움직였으면(도착 지연·유실) 지나간 상태의 재진입을 되살릴 이유가 없다 — 추종이 현재 상태로 데려간다.
	if (ReenteredTag != StateTag)
	{
		return;
	}

	UE_LOG(LogWxWorld, Verbose, TEXT("Gimmick(%s): [클라] 권위 재진입 통지 — 상태 %s 재선택 요청"), *GetNameSafe(Owner), *StateTag.ToString());
	EnterReplicatedState();
}

void UWxGimmickStateTreeComponent::StartTreeAtSavedState()
{
	// 저장된 상태가 없거나 에셋에서 그 태그를 찾지 못하면 순정 시작(루트 선택)에 맡긴다.
	const UStateTree* Asset = StateTreeRef.GetStateTree();
	if (!ResolveStateTag().IsValid())
	{
		Super::StartLogic();
		PublishAuthorityState();
		return;
	}

	if (HasValidStateTreeReference().HasError())
	{
		bIsRunning = false;
		DisableTick();
		return;
	}

	FStateTreeExecutionContext Context(*GetOwner(), *Asset, InstanceData);
	if (!SetContextRequirements(Context, /*bLogErrors*/ true))
	{
		DisableTick();
		return;
	}

	const EStateTreeRunStatus PreviousRunStatus = Context.GetStateTreeRunStatus();

	FWxGimmickStateTreeExecutionExtension Extension;
	Extension.Component = this;

	FStateTreeExecutionContext::FStartParameters StartParameters;
	StartParameters.InitialGlobalParameters = StateTreeRef.GetGlobalParameters();
	StartParameters.ExecutionExtension = TInstancedStruct<FWxGimmickStateTreeExecutionExtension>::Make(MoveTemp(Extension));

	FStateTreeExecutionContext::FStartParameters::FStateToSelectOverrideArgs SelectStateArgs;
	SelectStateArgs.StateTag = StateTag;
	SelectStateArgs.TagQueryMethod = UStateTree::EStateGameplayTagQueryMethod::MatchesExact;
	StartParameters.SelectStateOverrideArgs = SelectStateArgs;

	const EStateTreeRunStatus CurrentRunStatus = Context.Start(MoveTemp(StartParameters));

	bIsRunning = CurrentRunStatus == EStateTreeRunStatus::Running;
	ScheduleTickFrame(Context.GetNextScheduledTick());

	if (CurrentRunStatus != PreviousRunStatus)
	{
		OnStateTreeRunStatusChanged.Broadcast(CurrentRunStatus);
	}

	// 시작 직후의 상태를 한 번 기록한다(폴백 경로와 짝). 이후 갱신은 틱이 맡는다.
	PublishAuthorityState();
}

FStateTreeStateHandle UWxGimmickStateTreeComponent::ResolveStateTag() const
{
	const UStateTree* Asset = StateTreeRef.GetStateTree();
	if (!StateTag.IsValid() || !Asset)
	{
		return FStateTreeStateHandle::Invalid;
	}

	// 상태 Tag 는 에셋 안에서 유일하다는 계약이라 정확 일치로만 찾는다.
	return Asset->GetStateHandleFromGameplayTag(StateTag, UStateTree::EStateGameplayTagQueryMethod::MatchesExact);
}

void UWxGimmickStateTreeComponent::PublishAuthorityState()
{
	// 상태 값은 서버 권위다. 클라는 복제된 StateTag 를 추종하기만 한다.
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	// 태그 없는 상태에 머무는 동안엔 마지막 유효 값을 유지한다 — 트리 정지·미태그 구간에서 저장 값이 지워지지 않게 한다.
	// 재진입 판정도 플래그를 든 채로 미룬다. 태그 있는 상태로 돌아오는 그 순간이 곧 그 상호작용이 만든 재진입이다.
	const FGameplayTag ActiveTag = GetActiveStateTag();
	if (!ActiveTag.IsValid())
	{
		return;
	}

	if (ActiveTag != StateTag)
	{
		UE_LOG(LogWxWorld, Verbose, TEXT("Gimmick(%s): [권위] 상태 %s → %s"), *GetNameSafe(Owner), *StateTag.ToString(), *ActiveTag.ToString());
		StateTag = ActiveTag;
	}
	else if (bPendingInteractResolve)
	{
		// 상호작용을 받고도 태그가 그대로 = 자기 자신으로 가는 전이. 값 복제로는 안 보이는 유일한 경우라 여기서만 따로 알린다.
		UE_LOG(LogWxWorld, Verbose, TEXT("Gimmick(%s): [권위] 상태 %s 재진입"), *GetNameSafe(Owner), *StateTag.ToString());
		Multicast_ReenterState(StateTag);
	}

	// 플래그가 선 뒤 처음 도는 이 지점은 트리가 이벤트를 소화한 뒤다 — 나머지 호출부(시작·정지)에선 소화할 이벤트 자체가 없다.
	bPendingInteractResolve = false;
}

void UWxGimmickStateTreeComponent::FollowAuthorityState()
{
	if (!IsRunning())
	{
		return;
	}

	// 태그가 한쪽이라도 비어 있으면 대조할 근거가 없다.
	//  - 권위 값이 아직 안 왔다: 따라갈 곳이 없다.
	//  - 내가 태그 없는 구간에 있다: 그 구간에선 권위의 StateTag 도 마지막 유효 값(과거)을 들고 있어, 대조하면 이미 지나온 상태로 되감긴다.
	const FGameplayTag ActiveTag = GetActiveStateTag();
	if (!StateTag.IsValid() || !ActiveTag.IsValid())
	{
		return;
	}

	// 이미 권위와 같은 상태면 할 일이 없다(정상 경로).
	if (ActiveTag == StateTag)
	{
		return;
	}

	// 어긋난 원인(지연·발행 유실·전이 조건 불일치)을 가리지 않고 권위 값으로 끌어온다.
	UE_LOG(LogWxWorld, Verbose, TEXT("Gimmick(%s): [클라] 로컬 %s ≠ 권위 %s — 추종 전이 요청"), *GetNameSafe(GetOwner()), *ActiveTag.ToString(), *StateTag.ToString());
	EnterReplicatedState();
}

void UWxGimmickStateTreeComponent::EnterReplicatedState()
{
	const UStateTree* Asset = StateTreeRef.GetStateTree();
	AActor* Owner = GetOwner();
	if (!Asset || !Owner)
	{
		return;
	}

	// 로컬 에셋에 없는 태그는 갈 곳이 없다. 여기서 걸러도 조용히 어긋난 채로 남으므로, 요란한 경고는 OnRep_StateTag 가 값이 바뀐 그 한 번에만 남긴다.
	const FStateTreeStateHandle TargetState = ResolveStateTag();
	if (!TargetState.IsValid())
	{
		return;
	}

	// 재시작이 아니라 전이 요청이다 — 인스턴스 데이터·대기 중인 발행이 보존되고, 진입이 초기 진입이 아니라 라이브 전이로 잡혀 트리거형 태스크가 정상 발동한다.
	// 요청은 다음 트리 틱의 전이 처리 맨 앞에서 소비되며, 잠들어 있으면 엔진이 실행 확장을 통해 틱을 깨운다.
	// 권위 값이므로 에셋이 저작한 어떤 전이보다 우선한다.
	FStateTreeExecutionContext Context(*Owner, *Asset, InstanceData);
	Context.RequestTransition(TargetState, EStateTreeTransitionPriority::Critical);
}

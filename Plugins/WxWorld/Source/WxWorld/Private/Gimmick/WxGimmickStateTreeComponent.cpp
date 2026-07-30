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
#include "StructUtils/StructView.h"
#include "WxGameplayTags.h"

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

	// 상태 태그를 복제해 늦게 관련성을 얻은 피어까지 수렴시킨다.
	SetIsReplicatedByDefault(true);
}

void UWxGimmickStateTreeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWxGimmickStateTreeComponent, StateTag);
}

void UWxGimmickStateTreeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 상태 전이는 전부 트리 틱 안에서 일어나므로, 틱 직후 한 번 읽으면 모든 변화를 잡는다.
	RefreshStateTag();
}

void UWxGimmickStateTreeComponent::OnRegister()
{
	Super::OnRegister();

#if WITH_EDITOR
	// 런타임엔 안정적인 액터 식별자가 없다(ActorGuid 는 에디터 전용 데이터). 그래서 에디터 월드에서 값을 심어 에셋에 직렬화하고, 런타임은 그것을 읽기만 한다.
	const UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return;
	}

	// 등록할 때마다 오너와 대조한다 — 신규 배치·복제(붙여넣은 액터는 새 ActorGuid 를 받는다)·기존 배치 마이그레이션이 이 한 경로로 처리되고, 값이 같아지면 노옵이다.
	// 액터 단위 사전 등록 훅은 쓸 수 없다. 월드파티션 셀 스트리밍이 타는 증분 등록 경로가 그 함수를 호출하지 않는다.
	const AActor* Owner = GetOwner();
	const FGuid OwnerGuid = Owner ? Owner->GetActorGuid() : FGuid();
	if (!OwnerGuid.IsValid() || SaveId == OwnerGuid)
	{
		return;
	}

	Modify();
	SaveId = OwnerGuid;
#endif
}

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
	RefreshStateTag();

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
	// 상호작용 어빌리티가 서버 권위에서만 부르지만, 멀티캐스트는 권위에서만 유효하므로 한 번 더 가른다.
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	// 눌린 영역이 선언한 이벤트 태그로 발행한다. 영역마다 갈 곳이 다른 기믹은 이 태그로 갈리고, 선언이 없으면 공용 태그를 쓴다.
	UPrimitiveComponent* SourceMesh = const_cast<UPrimitiveComponent*>(Cast<UPrimitiveComponent>(Source));
	const FWxGimmickInteractionRegion* Region = InteractionRegions.Find(SourceMesh);
	const FGameplayTag EventTag = (Region && Region->InteractEvent.IsValid()) ? Region->InteractEvent : WxGameplayTags::StateTree_Interact;

	Multicast_Interact(SourceMesh, Interactor, EventTag);
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

void UWxGimmickStateTreeComponent::SetInteractionEnabled(UPrimitiveComponent* Mesh, bool bEnabled, const FText& Prompt, FGameplayTag InteractEvent)
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

	FWxGimmickInteractionRegion Region;
	Region.Prompt = Prompt;
	Region.InteractEvent = InteractEvent;
	InteractionRegions.Add(Mesh, MoveTemp(Region));
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

	// 자동 시작이 끝난 시점의 상태를 한 번 기록한다. 이후 갱신은 틱이 맡는다.
	RefreshStateTag();
}

void UWxGimmickStateTreeComponent::OnRep_StateTag()
{
	// 아직 시작 전이면 곧 BeginPlay 가 이 값으로 시작하므로 건드리지 않는다.
	if (!IsRunning())
	{
		return;
	}

	// 멀티캐스트 이벤트로 이미 같은 상태에 도달했으면 할 일이 없다(정상 경로).
	if (GetActiveStateTag() == StateTag)
	{
		return;
	}

	RestartLogic();
}

void UWxGimmickStateTreeComponent::Multicast_Interact_Implementation(UPrimitiveComponent* Source, AActor* Interactor, FGameplayTag EventTag)
{
	// 당사자는 복제 프로퍼티가 아니라 이 호출이 각 피어에 나른다 — 이동·몽타주 태스크가 모든 머신에서 같은 대상을 본다.
	InteractingCharacter = Cast<ACharacter>(Interactor);

	FWxGimmickInteractEvent Payload;
	Payload.Source = Source;
	Payload.Interactor = Interactor;

	SendStateTreeEvent(EventTag, FConstStructView::Make(Payload));
}

void UWxGimmickStateTreeComponent::StartTreeAtSavedState()
{
	// 저장된 상태가 없거나 에셋에서 그 태그를 찾지 못하면 순정 시작(루트 선택)에 맡긴다.
	const UStateTree* Asset = StateTreeRef.GetStateTree();
	const bool bCanSelectSavedState = StateTag.IsValid() && Asset
		&& Asset->GetStateHandleFromGameplayTag(StateTag, UStateTree::EStateGameplayTagQueryMethod::MatchesExact).IsValid();
	if (!bCanSelectSavedState)
	{
		Super::StartLogic();
		RefreshStateTag();
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
}

void UWxGimmickStateTreeComponent::RefreshStateTag()
{
	// 상태 값은 서버 권위다. 클라는 복제된 StateTag 를 추종하기만 한다.
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	// 태그 없는 상태에 머무는 동안엔 마지막 유효 값을 유지한다 — 트리 정지·미태그 구간에서 저장 값이 지워지지 않게 한다.
	const FGameplayTag ActiveTag = GetActiveStateTag();
	if (ActiveTag.IsValid())
	{
		StateTag = ActiveTag;
	}
}

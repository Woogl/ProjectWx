// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxGimmick.h"

#include "Components/ArrowComponent.h"
#include "Components/StateTreeComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "WxGameplayTags.h"

AWxGimmick::AWxGimmick()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	StateTree = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTree"));
	// 초기 진입 스냅(위치·포즈·애니)은 각 태스크가 자체 수행한다.
	// 따라서 자동 시작에 맡기고 자식은 명시 StartLogic 을 호출하지 않는다.
	StateTree->SetStartLogicAutomatically(true);

#if WITH_EDITORONLY_DATA
	ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	if (ArrowComponent)
	{
		ArrowComponent->SetupAttachment(SceneRoot);
		ArrowComponent->ArrowColor = FColor(150, 200, 255);
		ArrowComponent->ArrowSize = 1.0f;
		ArrowComponent->bTreatAsASprite = true;
		ArrowComponent->bIsScreenSizeScaled = true;
	}
#endif
}

void AWxGimmick::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxGimmick, State);
	DOREPLIFETIME(AWxGimmick, InteractingCharacter);
}

void AWxGimmick::CommitGimmickState(FGameplayTag NewState)
{
	// State 쓰기는 무조건 서버 권위. 클라는 복제된 State 의 OnRep 이 동일 이벤트를 발행해 추종한다(서버 권위 우선).
	if (!HasAuthority())
	{
		return;
	}

	State = NewState;

	// 권위 측에선 OnRep 이 자동 발화하지 않으므로 직접 호출해 서버·클라가 같은 통지를 공유한다(RepNotify 관용구).
	OnRep_GimmickState();
}

void AWxGimmick::SetInteractingCharacter(AActor* InActor)
{
	// 상호작용 당사자 쓰기는 권위 전용(State 와 같은 서버 권위 규약). 클라는 복제된 InteractingCharacter 를 추종한다.
	if (!HasAuthority())
	{
		return;
	}

	// 캐릭터가 아니면(비캐릭터 상호작용) null 을 저장해 상호작용 이동/몽타주 태스크가 안전하게 no-op 하도록 둔다.
	InteractingCharacter = Cast<ACharacter>(InActor);
}

FText AWxGimmick::GetInteractionPrompt() const
{
	return InteractionPrompt;
}

FGuid AWxGimmick::GetSaveId() const
{
	return SaveId;
}

void AWxGimmick::OnWxSaveRestored()
{
	// 스트리밍 인 복원은 State 가 ST 시작 이후 직접 직렬화로 들어온다. 실행 중이면 재시작 후 저장된 상태 태그를 복원 진입으로 재송출해 그 상태로 스냅 진입한다.
	// 미실행(월드 초기화 복원)이면 곧 BeginPlay 가 자동 시작 후 동일 송출을 하므로 건드리지 않는다(이중 처리 방지).
	if (StateTree && StateTree->IsRunning())
	{
		StateTree->RestartLogic();
		SendGimmickStateEvent(/*bRestoreEntry*/ true);
	}
}

#if WITH_EDITOR
// 에디터 전용 GetActorGuid() 를 런타임 가용 UPROPERTY 로 복사한다.
// ActorGuid 는 에디터에서 액터별로 안정·고유하고, 부여된 WxSaveId 는 에셋 저장 시 직렬화되어 쿠커가 그대로 읽으므로 런타임 키가 보장된다.
void AWxGimmick::PostActorCreated()
{
	Super::PostActorCreated();

	SaveId = GetActorGuid();
}

void AWxGimmick::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

	// 복제 시 엔진이 새 ActorGuid 를 부여하므로 그대로 따라가면 원본과 충돌하지 않는다.
	SaveId = GetActorGuid();
}
#endif

void AWxGimmick::BeginPlay()
{
	Super::BeginPlay();

	// 자동 시작 직후, 현재(복원되었으면 저장된) State 태그를 복원 진입으로 발행한다.
	// 기본 상태면 매칭 전이가 없어 노옵이고, 비기본 상태(복원)면 그 상태로 스냅 진입한다.
	SendGimmickStateEvent(/*bRestoreEntry*/ true);
}

void AWxGimmick::OnRep_GimmickState()
{
	// 라이브 변경: 현재 상태 태그를 ST 이벤트로 보내 그 상태의 Required Event 전이를 구동한다.
	SendGimmickStateEvent(/*bRestoreEntry*/ false);
}

void AWxGimmick::SendGimmickStateEvent(bool bRestoreEntry)
{
	// 트리 미실행 중엔 보낼 수 없다 — 초기 시작은 BeginPlay 가, 스트리밍 복원은 OnWxSaveRestored 가 실행 보장 후 호출한다.
	if (!StateTree || !StateTree->IsRunning())
	{
		return;
	}

	// 현재 상태 태그가 곧 그 상태의 Required Event to Enter 다.
	StateTree->SendStateTreeEvent(State);

	// 복원 진입이면 마커를 함께 보내, 일회성 노드가 라이브 발동이 아닌 복원(스냅·스킵)으로 처리하게 한다.
	if (bRestoreEntry)
	{
		StateTree->SendStateTreeEvent(WxGameplayTags::Gimmick_Restore);
	}
}

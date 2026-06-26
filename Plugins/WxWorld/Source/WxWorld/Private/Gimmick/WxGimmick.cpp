// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxGimmick.h"

#include "Components/ArrowComponent.h"
#include "Components/StateTreeComponent.h"
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
		ArrowComponent->ArrowColor = FColor(255, 200, 0);
		ArrowComponent->bTreatAsASprite = true;
	}
#endif
}

void AWxGimmick::CommitGimmickState(uint8 NewStateValue)
{
	// State 쓰기는 무조건 서버 권위. 클라는 복제된 State 의 OnRep 이 동일 이벤트를 발행해 추종한다(서버 권위 우선).
	if (!HasAuthority())
	{
		return;
	}

	SetGimmickState(NewStateValue);

	// 권위 측에선 OnRep 이 자동 발화하지 않으므로 직접 호출해 서버·클라가 같은 통지를 공유한다(RepNotify 관용구).
	OnRep_GimmickState();
}

FGuid AWxGimmick::GetWxSaveId() const
{
	return WxSaveId;
}

void AWxGimmick::OnWxSaveRestored()
{
	// 스트리밍 인 복원은 State 가 ST 시작 이후 직접 직렬화로 들어온다. 실행 중이면 재시작해 복원값으로 다시 초기 선택(=스냅)한다.
	// 미실행(월드 초기화 복원)이면 곧 BeginPlay 자동 시작이 복원값을 선택하므로 건드리지 않는다(이중 시작 방지).
	if (StateTree && StateTree->IsRunning())
	{
		StateTree->RestartLogic();
	}
}

#if WITH_EDITOR
// 에디터 전용 GetActorGuid() 를 런타임 가용 UPROPERTY 로 복사한다. ActorGuid 는 에디터에서 액터별로 안정·고유하고,
// 부여된 WxSaveId 는 에셋 저장 시 직렬화되어 쿠커가 그대로 읽으므로 런타임 키가 보장된다.
void AWxGimmick::PostActorCreated()
{
	Super::PostActorCreated();

	WxSaveId = GetActorGuid();
}

void AWxGimmick::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

	// 복제 시 엔진이 새 ActorGuid 를 부여하므로 그대로 따라가면 원본과 충돌하지 않는다.
	WxSaveId = GetActorGuid();
}
#endif

void AWxGimmick::OnRep_GimmickState()
{
	// 실행 중일 때만 라이브 이벤트를 보낸다. 미실행이면 초기 진입 enter 조건 선택이, 복원이면 OnWxSaveRestored 의 RestartLogic 이 처리한다.
	if (StateTree && StateTree->IsRunning())
	{
		StateTree->SendStateTreeEvent(WxGameplayTags::Event_GimmickStateChanged);
	}
}

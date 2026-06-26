// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxCutsceneTrigger.h"

#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "Net/UnrealNetwork.h"

AWxCutsceneTrigger::AWxCutsceneTrigger()
{
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);

	InteractionComponent = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(MeshComponent);
}

void AWxCutsceneTrigger::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxCutsceneTrigger, State);
}

void AWxCutsceneTrigger::BeginPlay()
{
	Super::BeginPlay();

	InteractionComponent->OnInteracted.AddDynamic(this, &AWxCutsceneTrigger::HandleInteracted);
}

void AWxCutsceneTrigger::SetGimmickState(uint8 NewStateValue)
{
	// 베이스 CommitGimmickState(권위)가 호출하는 State 쓰기 훅. 재생·입력차단·인터랙션 토글은 ST 가 State 변화를 추종해 적용한다.
	State = static_cast<EWxCutsceneTriggerState>(NewStateValue);
}

void AWxCutsceneTrigger::HandleInteracted(AActor* InstigatorActor)
{
	// 권위 측만 State 를 Playing 으로 확정한다. 클라는 복제 State 의 OnRep 이벤트가 ST 재선택을 구동하므로 비권위는 노옵.
	// 재생 종료 후 Idle 복귀는 Wx Play Level Sequence 태스크의 HandleLevelSequenceFinished 통지가 맡는다. 재생 중엔 ST 가 인터랙션을 비활성화해 재진입을 막는다.
	if (HasAuthority())
	{
		CommitGimmickState(static_cast<uint8>(EWxCutsceneTriggerState::Playing));
	}
}

void AWxCutsceneTrigger::HandleLevelSequenceFinished()
{
	// Wx Play Level Sequence 태스크가 재생 종료 시 권위 측에서 호출한다(권위 가드는 태스크·CommitGimmickState 양쪽에 있음). State 를 Idle 로 되돌리면 클라는 복제로 추종한다.
	CommitGimmickState(static_cast<uint8>(EWxCutsceneTriggerState::Idle));
}

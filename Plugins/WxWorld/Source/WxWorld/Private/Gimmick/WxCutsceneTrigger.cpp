// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxCutsceneTrigger.h"

#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "WxGameplayTags.h"

AWxCutsceneTrigger::AWxCutsceneTrigger()
{
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);

	InteractionComponent = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(MeshComponent);

	State = WxGameplayTags::Gimmick_CutsceneTrigger_Idle;
}

void AWxCutsceneTrigger::BeginPlay()
{
	// Playing 은 일시 상태라 복원하지 않는다 — SaveGame 으로 끌려온 값을 무시하고 항상 Idle 로 시작한다(베이스 BeginPlay 가 Idle 을 ST 에 발행).
	if (HasAuthority())
	{
		State = WxGameplayTags::Gimmick_CutsceneTrigger_Idle;
	}

	Super::BeginPlay();

	InteractionComponent->OnInteracted.AddDynamic(this, &AWxCutsceneTrigger::HandleInteracted);
}

void AWxCutsceneTrigger::OnWxSaveRestored()
{
	// 스트리밍 인 복원도 마찬가지로 Playing 을 무시하고 Idle 로 리셋한 뒤, Super 가 RestartLogic + Idle 발행으로 ST 를 Idle 로 진입시킨다.
	if (HasAuthority())
	{
		State = WxGameplayTags::Gimmick_CutsceneTrigger_Idle;
	}

	Super::OnWxSaveRestored();
}

void AWxCutsceneTrigger::HandleInteracted(AActor* InstigatorActor)
{
	// 권위 측만 State 를 Playing 으로 확정한다. 클라는 복제 State 의 OnRep 이벤트가 ST 진입을 구동하므로 비권위는 노옵.
	// 재생 종료 후 Idle 복귀는 Wx Play Level Sequence 태스크의 HandleLevelSequenceFinished 통지가 맡는다. 재생 중엔 ST 가 인터랙션을 비활성화해 재진입을 막는다.
	if (HasAuthority())
	{
		CommitGimmickState(WxGameplayTags::Gimmick_CutsceneTrigger_Playing);
	}
}

void AWxCutsceneTrigger::HandleLevelSequenceFinished()
{
	// Wx Play Level Sequence 태스크가 재생 종료 시 권위 측에서 호출한다(권위 가드는 태스크·CommitGimmickState 양쪽에 있음). State 를 Idle 로 되돌리면 클라는 복제로 추종한다.
	CommitGimmickState(WxGameplayTags::Gimmick_CutsceneTrigger_Idle);
}

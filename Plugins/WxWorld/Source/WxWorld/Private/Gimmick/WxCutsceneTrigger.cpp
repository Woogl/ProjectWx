// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxCutsceneTrigger.h"

#include "Components/StaticMeshComponent.h"
#include "WxGameplayTags.h"

AWxCutsceneTrigger::AWxCutsceneTrigger()
{
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	// 이 메시가 곧 상호작용 영역이며, 기본 활성으로 시작한다. 이후 활성/비활성은 ST 의 Enable Interaction 이 이 집합에 넣고 빼 가른다.
	ActiveInteractionMeshes.Add(MeshComponent);

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

void AWxCutsceneTrigger::OnInteracted(AActor* Interactor, const UActorComponent* Source)
{
	// 서버 권위에서만 호출된다. State 를 Playing 으로 확정하면 클라는 복제 State 의 OnRep 이 ST 진입을 구동한다.
	// 재생 종료 후 Idle 복귀는 Play Level Sequence 태스크의 HandleLevelSequenceFinished 통지가 맡는다. 재생 중엔 ST 가 인터랙션을 비활성화해 재진입을 막는다.
	CommitGimmickState(WxGameplayTags::Gimmick_CutsceneTrigger_Playing);
}

void AWxCutsceneTrigger::HandleLevelSequenceFinished()
{
	// Play Level Sequence 태스크가 재생 종료 시 권위 측에서 호출한다(권위 가드는 태스크·CommitGimmickState 양쪽에 있음). State 를 Idle 로 되돌리면 클라는 복제로 추종한다.
	CommitGimmickState(WxGameplayTags::Gimmick_CutsceneTrigger_Idle);
}

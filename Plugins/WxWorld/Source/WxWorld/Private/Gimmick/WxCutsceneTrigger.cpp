// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxCutsceneTrigger.h"

#include "Components/StateTreeComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"

AWxCutsceneTrigger::AWxCutsceneTrigger()
{
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);

	InteractionComponent = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(MeshComponent);
}

void AWxCutsceneTrigger::BeginPlay()
{
	Super::BeginPlay();

	InteractionComponent->OnInteracted.AddDynamic(this, &AWxCutsceneTrigger::HandleInteracted);
}

void AWxCutsceneTrigger::HandleInteracted(AActor* InstigatorActor)
{
	// 상호작용은 MulticastInteracted 로 모든 피어에서 발화한다. 각 피어가 로컬 StateTree 에 이벤트를 보내 Playing 으로 전이시킨다(복제 State 불필요).
	// 재생 종료 복귀는 Wx Play Level Sequence 의 Succeeded → OnComplete 전이가 맡는다. 재생 중엔 ST 가 인터랙션을 비활성화해 재진입을 막는다.
	if (PlayEventTag.IsValid())
	{
		StateTree->SendStateTreeEvent(PlayEventTag);
	}
}

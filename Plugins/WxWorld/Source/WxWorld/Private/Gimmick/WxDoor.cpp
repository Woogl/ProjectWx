// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxDoor.h"

#include "Components/StaticMeshComponent.h"
#include "WxGameplayTags.h"

AWxDoor::AWxDoor()
{
	DoorLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorLeft"));
	DoorLeft->SetupAttachment(SceneRoot);

	DoorRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorRight"));
	DoorRight->SetupAttachment(SceneRoot);

	Console = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Console"));
	Console->SetupAttachment(SceneRoot);
	// 이 메시가 곧 상호작용 영역이며, 기본 활성으로 시작한다. 이후 활성/비활성은 ST 의 Enable Interaction 이 이 집합에 넣고 빼 가른다.
	ActiveInteractionMeshes.Add(Console);

	State = WxGameplayTags::Gimmick_Door_Close;
}

void AWxDoor::OnInteracted(AActor* Interactor, const UActorComponent* Source)
{
	// 서버 권위에서만 호출된다. 현재 상태의 반대 목표로 확정한다(슬라이드는 StateTree 의 Component Move 가 비주얼로 처리).
	// 닫기(Open→Close)는 준비돼 있으나, Open 상태의 인터랙션 태스크가 에셋에서 비활성이라 Open 에선 호출되지 않아 현재 단방향으로 동작한다.
	const FGameplayTag NextState = (State == WxGameplayTags::Gimmick_Door_Close) ? WxGameplayTags::Gimmick_Door_Open : WxGameplayTags::Gimmick_Door_Close;
	CommitGimmickState(NextState);
}

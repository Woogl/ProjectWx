// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxCheckPoint.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AWxCheckPoint::AWxCheckPoint()
{
	// 컴포넌트는 베이스(AWxGimmick)가 만든 SceneRoot 에 부착한다. bReplicates·SceneRoot·StateTree 는 베이스가 제공한다.
	// 이 메시가 곧 상호작용 영역이다. 언제 켜지고 어디로 가는지는 ST 의 각 상태가 선언한다.
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetRelativeLocation(FVector(-90.f, 0.f, 0.f));

	// 부활 자리. 루트와 같은 자리에서 시작하며, 메시와 형제라 토치를 두고 이 컴포넌트만 옮겨 서는 위치·방향을 잡는다.
	ResumePoint = CreateDefaultSubobject<USceneComponent>(TEXT("ResumePoint"));
	ResumePoint->SetupAttachment(SceneRoot);
}

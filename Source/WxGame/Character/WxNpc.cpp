// Copyright Woogle. All Rights Reserved.

#include "Character/WxNpc.h"

#include "Character/WxMetaHumanComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

AWxNpc::AWxNpc()
{
	// 캐릭터(ACharacter)와 같은 구성이라 캡슐 크기·프리셋도 그 기본값을 따른다.
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	SetRootComponent(CapsuleComponent);

	CapsuleComponent->InitCapsuleSize(34.f, 88.f);
	CapsuleComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	CapsuleComponent->CanCharacterStepUpOn = ECB_No;
	CapsuleComponent->SetCanEverAffectNavigation(false);
	CapsuleComponent->bDynamicObstacle = true;
	CapsuleComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	// 레벨에 끌어다 놓을 때 캡슐 바닥이 지면에 맞도록.
	// 폰(APawn)이 켜는 플래그이며, 이게 꺼져 있으면 배치 범위가 0 이라 원점이 지면에 붙어 발이 묻힌다.
	bCollideWhenPlacing = true;

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CapsuleComponent);
	// 캐릭터 계열은 이 정렬을 BP 에서 주지만, NPC 는 BP 마다 반복시키지 않고 여기서 준다.
	MeshComponent->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0.f, -90.f, 0.f));

	// 활성 판정은 베이스가 들지만 감지·사거리를 콜리전 형상으로 재므로 쿼리 콜리전은 켜 둔다.
	// 몸통 충돌은 캡슐이 맡으므로 응답은 전부 Ignore 다 — 스캐너의 오버랩도 사거리 판정도 오브젝트 타입만 보고 응답 매트릭스를 보지 않는다.
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetGenerateOverlapEvents(false);

	MetaHumanComponent = CreateDefaultSubobject<UWxMetaHumanComponent>(TEXT("MetaHumanComponent"));
}

// Copyright Woogle. All Rights Reserved.

#include "WxNpc.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "WxDialogueComponent.h"
#include "WxDialogueSessionComponent.h"

AWxNpc::AWxNpc()
{
	// 캡슐 루트 + 그 자식 메시. 캐릭터(ACharacter)와 같은 구성이라 크기·프리셋도 그 기본값을 따른다.
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	SetRootComponent(CapsuleComponent);

	CapsuleComponent->InitCapsuleSize(34.f, 88.f);
	CapsuleComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	CapsuleComponent->CanCharacterStepUpOn = ECB_No;
	CapsuleComponent->SetCanEverAffectNavigation(false);
	CapsuleComponent->bDynamicObstacle = true;
	CapsuleComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	// 레벨에 끌어다 놓을 때 캡슐 바닥이 지면에 맞도록. 폰(APawn)이 켜는 플래그이며, 이게 꺼져 있으면 배치 범위가 0 이라 원점이 지면에 붙어 발이 묻힌다.
	bCollideWhenPlacing = true;

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CapsuleComponent);
	// 캐릭터 계열은 이 정렬을 BP 에서 주지만, NPC 는 BP 마다 반복시키지 않고 여기서 준다.
	MeshComponent->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0.f, -90.f, 0.f));

	// 이 메시가 곧 상호작용 영역이다. 대상 자격은 IsInteractionMeshActive 가 정하지만 감지·사거리를 콜리전 형상으로 재므로 쿼리 콜리전은 켜 둔다.
	// 몸통 충돌은 캡슐이 맡으므로 응답은 전부 Ignore 다 — 스캐너의 오버랩도 사거리 판정도 오브젝트 타입만 보고 응답 매트릭스를 보지 않는다.
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetGenerateOverlapEvents(false);

	DialogueComponent = CreateDefaultSubobject<UWxDialogueComponent>(TEXT("DialogueComponent"));

	DialogueCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("DialogueCameraComponent"));
	// 메시가 아니라 루트(캡슐)에 붙인다. 메시엔 캐릭터 정렬 보정(Z -90, Yaw -90)이 걸려 있어 거기 기준으로는 구도 수치가 직관과 어긋난다.
	DialogueCameraComponent->SetupAttachment(CapsuleComponent);

	// 정면(+X)에서 살짝 우측으로 빠진 3/4 미디엄 샷. 눈높이(캡슐 중심 +70)에 두고 같은 높이의 머리를 겨눠 피치는 0 이다.
	// Yaw -160 은 (110, 40) 에서 원점을 향하는 각이며, 약 117cm 거리는 기본 화각(90도)에서 허리 위가 화면을 채우는 거리다.
	// 어디까지나 기본값이다 — 실제 구도는 NPC BP·레벨 인스턴스에서 조정한다.
	DialogueCameraComponent->SetRelativeLocationAndRotation(FVector(110.f, 40.f, 70.f), FRotator(0.f, -160.f, 0.f));
}

bool AWxNpc::IsInteractionMeshActive(const UPrimitiveComponent* Mesh) const
{
	// NPC 는 항상 말을 걸 수 있다. 대화 중 차단은 상호작용 어빌리티의 State.Dialogue 차단 태그가 맡는다.
	return Mesh == MeshComponent;
}

void AWxNpc::OnInteracted(AActor* Interactor, const UActorComponent* Source)
{
	const APawn* Pawn = Cast<APawn>(Interactor);
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	UWxDialogueSessionComponent* Session = Controller ? Controller->FindComponentByClass<UWxDialogueSessionComponent>() : nullptr;
	if (!Session)
	{
		return;
	}

	Session->StartDialogue(DialogueComponent);
}

FText AWxNpc::GetInteractionPrompt(const UActorComponent* Source) const
{
	return FText::Format(NSLOCTEXT("WxNpc", "TalkPromptFormat", "Talk to {0}"), NpcName);
}

// Copyright Woogle. All Rights Reserved.

#include "WxNpc.h"
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

	// 이 메시가 곧 상호작용 영역이다. 대상 자격은 GetActiveInteractionMeshes 가 정하지만 사거리는 콜리전 형상으로 재므로 쿼리 콜리전은 켜 둔다.
	// 몸통 충돌은 캡슐이 맡으므로 응답은 전부 Ignore 다 — 사거리 판정은 바디에 직접 던지는 테스트라 응답을 보지 않는다.
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetGenerateOverlapEvents(false);

	DialogueComponent = CreateDefaultSubobject<UWxDialogueComponent>(TEXT("DialogueComponent"));
}

void AWxNpc::GetActiveInteractionMeshes(TArray<UPrimitiveComponent*>& OutMeshes) const
{
	// NPC 는 항상 말을 걸 수 있다. 대화 중 차단은 상호작용 어빌리티의 State.Dialogue 차단 태그가 맡는다.
	OutMeshes.Add(MeshComponent);
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

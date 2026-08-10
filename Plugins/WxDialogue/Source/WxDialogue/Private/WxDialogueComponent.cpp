// Copyright Woogle. All Rights Reserved.

#include "WxDialogueComponent.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "WxDialogueSessionComponent.h"

const FDataTableRowHandle& UWxDialogueComponent::GetStartRow() const
{
	return StartRow;
}

void UWxDialogueComponent::SetAreaMesh(UPrimitiveComponent* Mesh)
{
	AreaMesh = Mesh;
}

bool UWxDialogueComponent::IsInteractionMeshActive(const UPrimitiveComponent* Mesh) const
{
	// 콜리전을 여기서 함께 보는 것은 서버 검증 순서 때문이다 — 활성 검증이 사거리 판정보다 앞서므로, 잠긴 대상은 콜리전이 꺼진 메시를 나무라는 사거리 판정의 ensure 에 닿기 전에 걸러진다.
	// 대화 중 차단은 여기가 아니라 상호작용 어빌리티의 State.Dialogue 차단 태그가 맡는다.
	return AreaMesh && Mesh == AreaMesh && AreaMesh->IsQueryCollisionEnabled();
}

void UWxDialogueComponent::OnInteracted(AActor* Interactor, const UActorComponent* Source)
{
	const APawn* Pawn = Cast<APawn>(Interactor);
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	UWxDialogueSessionComponent* Session = Controller ? Controller->FindComponentByClass<UWxDialogueSessionComponent>() : nullptr;
	if (!Session)
	{
		return;
	}

	Session->StartDialogue(this);
}

FText UWxDialogueComponent::GetInteractionPrompt(const UActorComponent* Source) const
{
	return FText::Format(NSLOCTEXT("WxDialogueComponent", "TalkPromptFormat", "Talk to {0}"), SpeakerName);
}

void UWxDialogueComponent::SetInteractionEnabled(bool bEnabled)
{
	if (!AreaMesh)
	{
		return;
	}

	// 켜는 쪽은 QueryOnly 로 되돌린다 — 감지·사거리 판정에 필요한 것은 쿼리뿐이다.
	AreaMesh->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

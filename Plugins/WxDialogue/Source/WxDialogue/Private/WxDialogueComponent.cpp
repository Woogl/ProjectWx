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

bool UWxDialogueComponent::IsInteractionEnabled() const
{
	// 감지가 액터 단위라 영역 메시를 꺼도 호스트의 다른 형상(캡슐 등)이 계속 스캔에 걸린다 — 잠금의 실질은 이 명시 판정이다.
	// 대화 중 차단은 여기가 아니라 상호작용 어빌리티의 State.Dialogue 차단 태그가 맡는다.
	return AreaMesh && AreaMesh->IsQueryCollisionEnabled();
}

void UWxDialogueComponent::SetInteractionEnabled(bool bEnabled)
{
	if (!AreaMesh)
	{
		return;
	}

	AreaMesh->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void UWxDialogueComponent::OnInteracted(AActor* Interactor)
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

FText UWxDialogueComponent::GetInteractionPrompt() const
{
	return FText::Format(NSLOCTEXT("WxDialogueComponent", "TalkPromptFormat", "Talk to {0}"), SpeakerName);
}

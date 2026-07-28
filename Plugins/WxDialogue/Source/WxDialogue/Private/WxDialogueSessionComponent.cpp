// Copyright Woogle. All Rights Reserved.

#include "WxDialogueSessionComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "WxDialogueComponent.h"
#include "WxDialogueTableRow.h"
#include "WxGameplayTags.h"

void UWxDialogueSessionComponent::StartDialogue(UWxDialogueComponent* Dialogue)
{
	if (!Dialogue)
	{
		return;
	}

	// 정의 컴포넌트는 행과 대상을 꺼내기 위한 껍데기다 — 세션은 행만 안다.
	StartDialogueRow(Dialogue->GetStartRow(), Dialogue->GetOwner());
}

void UWxDialogueSessionComponent::StartDialogueRow(const FDataTableRowHandle& StartRow, AActor* Target)
{
	if (!StartRow.DataTable || StartRow.RowName.IsNone())
	{
		return;
	}

	ClientStartDialogue(StartRow, Target);
}

void UWxDialogueSessionComponent::Advance()
{
	if (!CurrentRow)
	{
		return;
	}

	// 선택지가 있으면 Choose 가 진행을 이어받는다.
	if (!CurrentRow->Choices.IsEmpty())
	{
		return;
	}

	if (CurrentRow->NextDialogue.IsNone() || !EnterRow(CurrentRow->NextDialogue))
	{
		EndDialogue();
		return;
	}

	PublishCurrentLine();
}

void UWxDialogueSessionComponent::Choose(int32 ChoiceIndex)
{
	if (!CurrentRow || !CurrentRow->Choices.IsValidIndex(ChoiceIndex))
	{
		return;
	}

	const FName TargetDialogue = CurrentRow->Choices[ChoiceIndex].TargetDialogue;
	if (TargetDialogue.IsNone() || !EnterRow(TargetDialogue))
	{
		EndDialogue();
		return;
	}

	PublishCurrentLine();
}

AActor* UWxDialogueSessionComponent::GetCurrentDialogueTarget() const
{
	return CurrentTarget.Get();
}

FText UWxDialogueSessionComponent::GetCurrentSpeaker() const
{
	return CurrentRow ? CurrentRow->Speaker : FText::GetEmpty();
}

FText UWxDialogueSessionComponent::GetCurrentLine() const
{
	return CurrentRow ? CurrentRow->Line : FText::GetEmpty();
}

void UWxDialogueSessionComponent::ClientStartDialogue_Implementation(const FDataTableRowHandle& StartRow, AActor* Target)
{
	Table = StartRow.DataTable;
	if (!EnterRow(StartRow.RowName))
	{
		Table = nullptr;
		return;
	}

	// 관찰자에게 노출할 대화 대상. 세션이 실제로 열린 뒤에만 기억한다. 대상 없는 대사(나레이션)면 그대로 비어 있다.
	CurrentTarget = Target;

	// 대화 중 상태를 폰 ASC 에 발행한다. 상호작용 어빌리티가 이 태그로 차단되고 스캐너 표시 게이트(프롬프트·하이라이트)도 함께 닫힌다.
	const AController* Controller = Cast<AController>(GetOwner());
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
	{
		ASC->AddLooseGameplayTag(WxGameplayTags::State_Dialogue);
		TaggedAbilitySystem = ASC;
	}

	// 구독자(PC)가 여기서 대화 위젯을 푸시하고, 위젯의 뷰모델이 현재 대사를 pull 해 시드한다.
	OnDialogueStarted.Broadcast();
}

bool UWxDialogueSessionComponent::EnterRow(FName RowName)
{
	const FWxDialogueTableRow* Row = Table ? Table->FindRow<FWxDialogueTableRow>(RowName, TEXT("WxDialogueSession")) : nullptr;
	if (!Row || Row->Line.IsEmpty())
	{
		return false;
	}

	CurrentRow = Row;

	return true;
}

void UWxDialogueSessionComponent::PublishCurrentLine()
{
	OnLineChanged.Broadcast(GetCurrentSpeaker(), GetCurrentLine());
}

void UWxDialogueSessionComponent::EndDialogue()
{
	Table = nullptr;
	CurrentRow = nullptr;
	CurrentTarget.Reset();

	// 시작 때 발행한 대화 상태 태그를 같은 ASC 에서 되돌려 프롬프트 표시·상호작용을 복귀시킨다.
	if (UAbilitySystemComponent* ASC = TaggedAbilitySystem.Get())
	{
		ASC->RemoveLooseGameplayTag(WxGameplayTags::State_Dialogue);
	}
	TaggedAbilitySystem.Reset();

	OnDialogueEnded.Broadcast();
}

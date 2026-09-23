// Copyright Woogle. All Rights Reserved.

#include "UI/WxDialogueScreen.h"
#include "GameFramework/PlayerController.h"
#include "MVVM/WxViewModel_Dialogue.h"
#include "View/MVVMView.h"
#include "WxDialogueSessionComponent.h"

void UWxDialogueScreen::NativeConstruct()
{
	Super::NativeConstruct();
	// 생성 전에 활성화된 화면도 MVVM의 Construct가 끝난 뒤 연결한다.
	if (IsActivated() && !DialogueViewModel.IsValid())
	{
		BindDialogue();
	}
}

void UWxDialogueScreen::NativeDestruct()
{
	UnbindDialogue();
	Super::NativeDestruct();
}

void UWxDialogueScreen::NativeOnActivated()
{
	BindDialogue();
	Super::NativeOnActivated();
}

void UWxDialogueScreen::NativeOnDeactivated()
{
	UnbindDialogue();
	Super::NativeOnDeactivated();
}

void UWxDialogueScreen::BindDialogue()
{
	UnbindDialogue();
	const UMVVMView* View = GetExtension<UMVVMView>();
	if (!View || !View->AreSourcesInitialized())
	{
		return;
	}

	UWxViewModel_Dialogue* ViewModel = Cast<UWxViewModel_Dialogue>(View->GetViewModel(TEXT("WxViewModel_Dialogue")).GetObject());
	if (!ensureMsgf(ViewModel, TEXT("DialogueScreen requires a WxViewModel_Dialogue source with Create Instance.")))
	{
		return;
	}
	DialogueViewModel = ViewModel;

	const APlayerController* PC = GetOwningPlayer();
	UWxDialogueSessionComponent* Session = PC ? PC->FindComponentByClass<UWxDialogueSessionComponent>() : nullptr;
	ObservedSession = Session;
	if (Session)
	{
		Session->OnLineChanged.AddUniqueDynamic(ViewModel, &UWxViewModel_Dialogue::SetLine);
		// 비활성 동안 놓친 대사와, 위젯 생성 전에 발행된 첫 대사를 함께 보충한다.
		ViewModel->SetLine(Session->GetCurrentSpeaker(), Session->GetCurrentLine());
	}
	else
	{
		ViewModel->SetLine(FText::GetEmpty(), FText::GetEmpty());
	}
}

void UWxDialogueScreen::UnbindDialogue()
{
	UWxDialogueSessionComponent* Session = ObservedSession.Get();
	UWxViewModel_Dialogue* ViewModel = DialogueViewModel.Get();
	if (Session && ViewModel)
	{
		Session->OnLineChanged.RemoveDynamic(ViewModel, &UWxViewModel_Dialogue::SetLine);
	}
	ObservedSession.Reset();
	DialogueViewModel.Reset();
}

void UWxDialogueScreen::RequestAdvance()
{
	UWxDialogueSessionComponent* Session = ObservedSession.Get();
	if (IsActivated() && Session)
	{
		Session->Advance();
	}
}

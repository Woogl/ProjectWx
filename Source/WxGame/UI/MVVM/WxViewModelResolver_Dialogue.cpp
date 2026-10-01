// Copyright Woogle. All Rights Reserved.

#include "UI/MVVM/WxViewModelResolver_Dialogue.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "UI/MVVM/WxViewModel_Dialogue.h"
#include "Dialogue/WxDialogueSessionComponent.h"

UObject* UWxViewModelResolver_Dialogue::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	UWxViewModel_Dialogue* ViewModel = NewObject<UWxViewModel_Dialogue>(const_cast<UUserWidget*>(UserWidget));
	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	if (UWxDialogueSessionComponent* Session = PC ? PC->FindComponentByClass<UWxDialogueSessionComponent>() : nullptr)
	{
		Session->OnLineChanged.AddUniqueDynamic(ViewModel, &UWxViewModel_Dialogue::SetLine);
		// 창보다 먼저 발행된 첫 대사도 보여 준다.
		ViewModel->SetLine(Session->GetCurrentSpeaker(), Session->GetCurrentLine());
		ViewModel->SetSession(Session);
	}
	return ViewModel;
}

void UWxViewModelResolver_Dialogue::DestroyInstance(UObject* ViewModel, const UMVVMView* View) const
{
	const UUserWidget* UserWidget = ViewModel ? ViewModel->GetTypedOuter<UUserWidget>() : nullptr;
	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	if (UWxDialogueSessionComponent* Session = PC ? PC->FindComponentByClass<UWxDialogueSessionComponent>() : nullptr)
	{
		Session->OnLineChanged.RemoveAll(ViewModel);
	}
}

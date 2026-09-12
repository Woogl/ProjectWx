// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Dialogue.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "WxDialogueSessionComponent.h"

void UWxViewModel_Dialogue::Initialize(UWxDialogueSessionComponent* InSession)
{
	Deinitialize();
	if (!InSession)
	{
		return;
	}

	CachedSession = InSession;

	InSession->OnLineChanged.AddDynamic(this, &ThisClass::HandleLineChanged);

	// 세션 시드가 구독보다 먼저 끝나 있으므로 현재 대사로 시드한다.
	HandleLineChanged(InSession->GetCurrentSpeaker(), InSession->GetCurrentLine());
}

void UWxViewModel_Dialogue::Deinitialize()
{
	if (UWxDialogueSessionComponent* Session = CachedSession.Get())
	{
		Session->OnLineChanged.RemoveDynamic(this, &ThisClass::HandleLineChanged);
	}
	CachedSession.Reset();

	Super::Deinitialize();
	if (!HasAnyFlags(RF_BeginDestroyed))
	{
		HandleLineChanged(FText::GetEmpty(), FText::GetEmpty());
	}
}

void UWxViewModel_Dialogue::HandleLineChanged(const FText& InSpeaker, const FText& InLine)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(Speaker, InSpeaker))
	{
		// HasSpeaker 는 Speaker 에서 파생되므로 원본이 바뀔 때 함께 알린다.
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(HasSpeaker);
	}
	UE_MVVM_SET_PROPERTY_VALUE(LineText, InLine);
}

void UWxViewModel_Dialogue::RequestAdvance()
{
	if (UWxDialogueSessionComponent* Session = CachedSession.Get())
	{
		Session->Advance();
	}
}

bool UWxViewModel_Dialogue::HasSpeaker() const
{
	return !Speaker.IsEmpty();
}

UObject* UWxViewModelResolver_Dialogue::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	if (!UserWidget || !ExpectedType || !ExpectedType->IsChildOf(UWxViewModel_Dialogue::StaticClass()) || ExpectedType->HasAnyClassFlags(CLASS_Abstract))
	{
		return nullptr;
	}

	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	UWxDialogueSessionComponent* Session = PC ? PC->FindComponentByClass<UWxDialogueSessionComponent>() : nullptr;

	// 세션이 늦게 준비되면 호출 측에서 이 인스턴스에 Initialize 로 주입한다.
	UWxViewModel_Dialogue* ViewModel = NewObject<UWxViewModel_Dialogue>(const_cast<UUserWidget*>(UserWidget), ExpectedType);
	ViewModel->Initialize(Session);
	return ViewModel;
}

void UWxViewModelResolver_Dialogue::DestroyInstance(UObject* ViewModel, const UMVVMView* View) const
{
	if (UWxViewModel_Dialogue* Dialogue = Cast<UWxViewModel_Dialogue>(ViewModel))
	{
		Dialogue->Deinitialize();
	}
}

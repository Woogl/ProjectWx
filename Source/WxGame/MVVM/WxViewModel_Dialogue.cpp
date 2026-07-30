// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Dialogue.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "WxDialogueSessionComponent.h"

void UWxViewModel_Dialogue::Initialize(UWxDialogueSessionComponent* InSession)
{
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
	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	UWxDialogueSessionComponent* Session = PC ? PC->FindComponentByClass<UWxDialogueSessionComponent>() : nullptr;
	if (!Session)
	{
		return nullptr;
	}

	// 위젯이 아닌 데이터 소스(세션 컴포넌트)를 Outer 로 생성한다.
	// 세션은 PC 소유라 폰 리스폰에도 생존하며, 수명은 뷰의 강참조와 BeginDestroy 의 Deinitialize 가 관리한다.
	UWxViewModel_Dialogue* ViewModel = NewObject<UWxViewModel_Dialogue>(Session);
	ViewModel->Initialize(Session);
	return ViewModel;
}

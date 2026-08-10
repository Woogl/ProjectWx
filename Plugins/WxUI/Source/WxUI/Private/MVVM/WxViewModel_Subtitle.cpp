// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Subtitle.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "MVVMGameSubsystem.h"
#include "Types/MVVMViewModelCollection.h"
#include "Types/MVVMViewModelContext.h"

namespace
{
	/** 등록·조회가 같은 값을 써야 하므로 문맥을 한 곳에서 만든다. */
	FMVVMViewModelContext MakeSubtitleContext()
	{
		FMVVMViewModelContext Context;
		Context.ContextClass = UWxViewModel_Subtitle::StaticClass();
		Context.ContextName = TEXT("VM_Subtitle");
		return Context;
	}
}

UWxViewModel_Subtitle* UWxViewModel_Subtitle::GetOrCreate(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UMVVMGameSubsystem* ViewModelSubsystem = GameInstance ? GameInstance->GetSubsystem<UMVVMGameSubsystem>() : nullptr;
	UMVVMViewModelCollectionObject* ViewModelCollection = ViewModelSubsystem ? ViewModelSubsystem->GetViewModelCollection() : nullptr;
	if (!ViewModelCollection)
	{
		return nullptr;
	}

	const FMVVMViewModelContext Context = MakeSubtitleContext();
	if (UMVVMViewModelBase* Registered = ViewModelCollection->FindViewModelInstance(Context))
	{
		return CastChecked<UWxViewModel_Subtitle>(Registered);
	}

	// 컬렉션이 참조를 들어 주므로 게임 인스턴스 수명 동안 같은 인스턴스가 유지된다.
	UWxViewModel_Subtitle* SubtitleViewModel = NewObject<UWxViewModel_Subtitle>(ViewModelCollection);
	ViewModelCollection->AddViewModelInstance(Context, SubtitleViewModel);

	return SubtitleViewModel;
}

int32 UWxViewModel_Subtitle::ShowSubtitle(const FText& InSpeakerText, const FText& InSubtitleText)
{
	CurrentHandle = NextHandle++;

	if (!bHasSubtitle)
	{
		bHasSubtitle = true;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bHasSubtitle);
	}

	if (!SpeakerText.IdenticalTo(InSpeakerText))
	{
		SpeakerText = InSpeakerText;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SpeakerText);
	}

	if (!SubtitleText.IdenticalTo(InSubtitleText))
	{
		SubtitleText = InSubtitleText;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SubtitleText);
	}

	return CurrentHandle;
}

void UWxViewModel_Subtitle::HideSubtitle(int32 InSubtitleHandle)
{
	if (InSubtitleHandle == INDEX_NONE || InSubtitleHandle != CurrentHandle)
	{
		return;
	}

	CurrentHandle = INDEX_NONE;

	if (bHasSubtitle)
	{
		bHasSubtitle = false;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bHasSubtitle);
	}

	if (!SpeakerText.IsEmpty())
	{
		SpeakerText = FText::GetEmpty();
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SpeakerText);
	}

	if (!SubtitleText.IsEmpty())
	{
		SubtitleText = FText::GetEmpty();
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SubtitleText);
	}
}

UObject* UWxViewModelResolver_Subtitle::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	return UWxViewModel_Subtitle::GetOrCreate(UserWidget);
}

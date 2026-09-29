// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Subtitle.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "MVVMGameSubsystem.h"
#include "Types/MVVMViewModelCollection.h"
#include "Types/MVVMViewModelContext.h"

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

	FMVVMViewModelContext Context;
	Context.ContextClass = UWxViewModel_Subtitle::StaticClass();
	Context.ContextName = TEXT("VM_Subtitle");

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
	UE_MVVM_SET_PROPERTY_VALUE(SpeakerText, InSpeakerText);
	UE_MVVM_SET_PROPERTY_VALUE(SubtitleText, InSubtitleText);
	return CurrentHandle;
}

void UWxViewModel_Subtitle::HideSubtitle(int32 InSubtitleHandle)
{
	if (InSubtitleHandle == INDEX_NONE || InSubtitleHandle != CurrentHandle)
	{
		return;
	}

	CurrentHandle = INDEX_NONE;
	UE_MVVM_SET_PROPERTY_VALUE(SpeakerText, FText::GetEmpty());
	UE_MVVM_SET_PROPERTY_VALUE(SubtitleText, FText::GetEmpty());
}

UObject* UWxViewModelResolver_Subtitle::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	return UWxViewModel_Subtitle::GetOrCreate(UserWidget);
}

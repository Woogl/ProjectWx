// Copyright Woogle. All Rights Reserved.

#include "System/WxUIManagerSubsystem.h"
#include "System/WxPrimaryGameLayout.h"
#include "System/WxUIDeveloperSettings.h"
#include "Widget/WxGamePopup.h"
#include "WxGameplayTags.h"
#include "MVVM/WxViewModel_Selection.h"
#include "MVVMGameSubsystem.h"
#include "Types/MVVMViewModelCollection.h"
#include "Types/MVVMViewModelContext.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"

void UWxUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	// 전 위젯이 공유하는 "현재 선택" 글로벌 뷰모델을 생성해 MVVM 글로벌 컬렉션에 등록한다.
	// 초기화 순서를 보장하기 위해 UMVVMGameSubsystem 을 먼저 확정 초기화한다.
	Collection.InitializeDependency(UMVVMGameSubsystem::StaticClass());
	if (UMVVMGameSubsystem* ViewModelSubsystem = GameInstance->GetSubsystem<UMVVMGameSubsystem>())
	{
		if (UMVVMViewModelCollectionObject* ViewModelCollection = ViewModelSubsystem->GetViewModelCollection())
		{
			SelectionViewModel = NewObject<UWxViewModel_Selection>(this);

			FMVVMViewModelContext Context;
			Context.ContextClass = UWxViewModel_Selection::StaticClass();
			Context.ContextName = TEXT("VM_Selection");
			ViewModelCollection->AddViewModelInstance(Context, SelectionViewModel);
		}
	}

	for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
	{
		HandleLocalPlayerAdded(LocalPlayer);
	}

	GameInstance->OnLocalPlayerAddedEvent.AddUObject(this, &ThisClass::HandleLocalPlayerAdded);
}

void UWxUIManagerSubsystem::Deinitialize()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		GameInstance->OnLocalPlayerAddedEvent.RemoveAll(this);

		for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
		{
			if (LocalPlayer)
			{
				LocalPlayer->OnPlayerControllerChanged().RemoveAll(this);
			}
		}

		// 글로벌 선택 뷰모델을 컬렉션에서 등록 해제한다.
		if (UMVVMGameSubsystem* ViewModelSubsystem = GameInstance->GetSubsystem<UMVVMGameSubsystem>())
		{
			if (UMVVMViewModelCollectionObject* ViewModelCollection = ViewModelSubsystem->GetViewModelCollection())
			{
				FMVVMViewModelContext Context;
				Context.ContextClass = UWxViewModel_Selection::StaticClass();
				Context.ContextName = TEXT("VM_Selection");
				ViewModelCollection->RemoveViewModel(Context);
			}
		}
	}

	SelectionViewModel = nullptr;

	Super::Deinitialize();
}

UCommonActivatableWidget* UWxUIManagerSubsystem::PushContentToLayer(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	if (!PrimaryGameLayout || !WidgetClass)
	{
		return nullptr;
	}
	return PrimaryGameLayout->PushWidgetToLayerStack(LayerTag, WidgetClass);
}

UCommonActivatableWidget* UWxUIManagerSubsystem::PushWidgetInstanceToLayer(FGameplayTag LayerTag, UCommonActivatableWidget* WidgetInstance)
{
	if (!PrimaryGameLayout || !WidgetInstance)
	{
		return nullptr;
	}
	return PrimaryGameLayout->PushWidgetInstanceToLayerStack(LayerTag, WidgetInstance);
}

void UWxUIManagerSubsystem::ShowConfirmation(UWxGamePopupDescriptor* Descriptor, FWxPopupResultDelegate ResultCallback)
{
	const UWxUIDeveloperSettings* Settings = GetDefault<UWxUIDeveloperSettings>();
	PushPopup(Settings->ConfirmationPopupClass, Descriptor, ResultCallback);
}

void UWxUIManagerSubsystem::ShowError(UWxGamePopupDescriptor* Descriptor, FWxPopupResultDelegate ResultCallback)
{
	const UWxUIDeveloperSettings* Settings = GetDefault<UWxUIDeveloperSettings>();
	PushPopup(Settings->ErrorPopupClass, Descriptor, ResultCallback);
}

void UWxUIManagerSubsystem::PushPopup(const TSoftClassPtr<UWxGamePopup>& PopupClass, UWxGamePopupDescriptor* Descriptor, FWxPopupResultDelegate ResultCallback)
{
	if (!PrimaryGameLayout || !Descriptor || PopupClass.IsNull())
	{
		return;
	}

	TSubclassOf<UWxGamePopup> LoadedClass = PopupClass.LoadSynchronous();
	if (!LoadedClass)
	{
		return;
	}

	// 활성화 이전에 호출되는 초기화 콜백에서 SetupPopup 를 실행해 표시 전에 내용을 채운다.
	PrimaryGameLayout->PushWidgetToLayerStack<UWxGamePopup>(WxGameplayTags::UI_Layer_Modal, LoadedClass,
		[Descriptor, ResultCallback](UWxGamePopup& Popup)
		{
			Popup.SetupPopup(Descriptor, ResultCallback);
		});
}

UWxPrimaryGameLayout* UWxUIManagerSubsystem::GetPrimaryGameLayout() const
{
	return PrimaryGameLayout;
}

void UWxUIManagerSubsystem::HandleLocalPlayerAdded(ULocalPlayer* LocalPlayer)
{
	if (!LocalPlayer)
	{
		return;
	}

	if (APlayerController* PC = LocalPlayer->GetPlayerController(GetWorld()))
	{
		HandlePlayerControllerSet(PC);
	}

	LocalPlayer->OnPlayerControllerChanged().AddUObject(this, &ThisClass::HandlePlayerControllerSet);
}

void UWxUIManagerSubsystem::HandlePlayerControllerSet(APlayerController* PC)
{
	// PC 가 들어올 때마다 layout 을 무조건 재생성한다.
	// stale 식별을 시도하지 않는 이유: widget 의 GetOwningPlayer/GetWorld/GetOuter 가 모두
	// 유지되는 LocalPlayer/GameInstance 를 따라 자동으로 새 값을 반환하므로, widget 만 보고는 stale 여부를 알 수 없다.
	// layout 은 빈 컨테이너이고 컨텐츠는 PC 의 OnPossess/OnRep_Pawn 에서 다시 push 되므로 매번 재생성해도 비용이 작다.
	if (PrimaryGameLayout)
	{
		PrimaryGameLayout->RemoveFromParent();
		PrimaryGameLayout = nullptr;
	}

	if (!PC)
	{
		return;
	}

	CreateLayoutForPlayer(PC);
}

void UWxUIManagerSubsystem::CreateLayoutForPlayer(APlayerController* PC)
{
	const UWxUIDeveloperSettings* UISettings = GetDefault<UWxUIDeveloperSettings>();
	if (!UISettings || UISettings->LayoutClass.IsNull())
	{
		return;
	}

	TSubclassOf<UWxPrimaryGameLayout> LayoutClass = UISettings->LayoutClass.LoadSynchronous();
	if (!LayoutClass)
	{
		return;
	}

	PrimaryGameLayout = CreateWidget<UWxPrimaryGameLayout>(PC, LayoutClass);
	if (PrimaryGameLayout)
	{
		PrimaryGameLayout->AddToPlayerScreen();
	}
}

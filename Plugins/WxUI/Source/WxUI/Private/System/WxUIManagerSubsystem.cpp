// Copyright Woogle. All Rights Reserved.

#include "System/WxUIManagerSubsystem.h"
#include "CommonActivatableWidget.h"
#include "System/WxPrimaryGameLayout.h"
#include "System/WxUIDeveloperSettings.h"
#include "Widget/WxGamePopup.h"
#include "Widget/WxActivatableWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "WxGameplayTags.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"

void UWxUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 팝업 요청 시 서술자를 비동기 작업에 보관하지 않도록 클래스를 미리 로드한다.
	ConfirmationPopupClass = GetDefault<UWxUIDeveloperSettings>()->ConfirmationPopupClass.LoadSynchronous();

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	// 서브시스템 초기화가 로컬 플레이어 생성보다 앞서 보통은 아래 이벤트로 들어오고, 이 호출은 이미 만들어진 뒤에 초기화된 경우를 따라잡는다(null 은 핸들러가 거른다).
	HandleLocalPlayerAdded(GameInstance->GetFirstGamePlayer());

	GameInstance->OnLocalPlayerAddedEvent.AddUObject(this, &ThisClass::HandleLocalPlayerAdded);
}

void UWxUIManagerSubsystem::Deinitialize()
{
	TrackedPlayerController.Reset();

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		// 로컬 플레이어의 PC 교체 구독은 끊지 않는다 — GameInstance 는 로컬 플레이어를 모두 제거한 뒤에야 서브시스템을 내리므로 여기 도달했을 땐 그 로컬 플레이어가 이미 사라진 뒤다.
		GameInstance->OnLocalPlayerAddedEvent.RemoveAll(this);
	}

	Super::Deinitialize();
}

UCommonActivatableWidget* UWxUIManagerSubsystem::PushWidgetInstanceToLayer(FGameplayTag LayerTag, UCommonActivatableWidget* WidgetInstance)
{
	if (!PrimaryGameLayout || !WidgetInstance)
	{
		return nullptr;
	}
	UCommonActivatableWidget* Widget = PrimaryGameLayout->PushWidgetInstanceToLayerStack(LayerTag, WidgetInstance);
	ObserveWidgetForGamePause(Widget);
	return Widget;
}

void UWxUIManagerSubsystem::ShowConfirmation(UWxGamePopupDescriptor* Descriptor, FWxPopupResultDelegate ResultCallback)
{
	if (!Descriptor || !PrimaryGameLayout || !ConfirmationPopupClass)
	{
		ResultCallback.ExecuteIfBound(EWxPopupResult::Killed);
		return;
	}

	APlayerController* OwningPlayer = PrimaryGameLayout->GetOwningPlayer();
	if (!OwningPlayer)
	{
		ResultCallback.ExecuteIfBound(EWxPopupResult::Killed);
		return;
	}

	UWxGamePopup* Popup = CreateWidget<UWxGamePopup>(OwningPlayer, ConfirmationPopupClass);
	if (!Popup)
	{
		ResultCallback.ExecuteIfBound(EWxPopupResult::Killed);
		return;
	}

	// 레이어에 추가하면 활성화되므로 내용을 먼저 채운다.
	Popup->SetupPopup(Descriptor, ResultCallback);
	if (!PushWidgetInstanceToLayer(WxGameplayTags::UI_Layer_Modal, Popup))
	{
		ResultCallback.ExecuteIfBound(EWxPopupResult::Killed);
	}
}

UWxPrimaryGameLayout* UWxUIManagerSubsystem::GetPrimaryGameLayout() const
{
	return PrimaryGameLayout;
}

bool UWxUIManagerSubsystem::IsMenuLayerActive() const
{
	// GameMenu 는 화면을 덮지 않는 자리라 메뉴로 세지 않는다.
	return HasActiveWidgetInLayer(WxGameplayTags::UI_Layer_Menu) || HasActiveWidgetInLayer(WxGameplayTags::UI_Layer_Modal);
}

bool UWxUIManagerSubsystem::HasActiveWidgetInLayer(FGameplayTag LayerTag) const
{
	if (!PrimaryGameLayout)
	{
		return false;
	}

	const UCommonActivatableWidgetStack* Stack = PrimaryGameLayout->GetLayerWidgetStack(LayerTag);
	if (!Stack)
	{
		return false;
	}

	// 스택에 위젯이 하나뿐이면 비활성화 후에도 GetActiveWidget 이 그 위젯을 반환할 수 있어 IsActivated 로 걸러낸다.
	const UCommonActivatableWidget* ActiveWidget = Stack->GetActiveWidget();
	return ActiveWidget && ActiveWidget->IsActivated();
}

void UWxUIManagerSubsystem::ObserveWidgetForGamePause(UCommonActivatableWidget* Widget)
{
	if (!Widget)
	{
		return;
	}

	// 위젯은 CommonUI 풀에서 재사용될 수 있어 이미 구독돼 있을 수 있으므로, 중복 없이 다시 건다.
	Widget->OnActivated().RemoveAll(this);
	Widget->OnDeactivated().RemoveAll(this);
	Widget->OnActivated().AddUObject(this, &ThisClass::HandleObservedWidgetActivationChanged);
	Widget->OnDeactivated().AddUObject(this, &ThisClass::HandleObservedWidgetActivationChanged);

	// push 과정에서 이미 활성화됐을 수 있으므로 즉시 1회 재평가한다.
	RefreshGamePause();
}

void UWxUIManagerSubsystem::HandleObservedWidgetActivationChanged()
{
	RefreshGamePause();
}

void UWxUIManagerSubsystem::RefreshGamePause()
{
	UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	if (!World || !World->IsNetMode(NM_Standalone))
	{
		return;
	}

	APlayerController* PC = TrackedPlayerController.Get();
	if (!PC)
	{
		return;
	}

	// 게임모드는 해제 때 대리자를 건 정지만 되묻고 대리자 없는 정지는 그냥 지운다.
	// 그래서 남의 해제는 우리 정지를 지우지 못하지만, 다른 정지 주체도 대리자를 걸어야 우리 해제에 지워지지 않는다.
	if (WantsGamePause())
	{
		PC->SetPause(true, FCanUnpause::CreateUObject(this, &ThisClass::HandleCanUnpause));
	}
	else
	{
		PC->SetPause(false);
	}
}

bool UWxUIManagerSubsystem::WantsGamePause() const
{
	if (!PrimaryGameLayout)
	{
		return false;
	}

	// 스택에 위젯이 하나뿐이면 비활성화 후에도 GetActiveWidget 이 그 위젯을 반환할 수 있어 IsActivated 로 걸러낸다.
	for (const TPair<FGameplayTag, TObjectPtr<UCommonActivatableWidgetStack>>& Layer : PrimaryGameLayout->GetLayerMap())
	{
		UCommonActivatableWidgetStack* Stack = Layer.Value;
		if (!Stack)
		{
			continue;
		}

		UWxActivatableWidget* ActiveWidget = Cast<UWxActivatableWidget>(Stack->GetActiveWidget());
		if (ActiveWidget && ActiveWidget->IsActivated() && ActiveWidget->ShouldPauseGame())
		{
			return true;
		}
	}

	return false;
}

bool UWxUIManagerSubsystem::HandleCanUnpause()
{
	return !WantsGamePause();
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
	TrackedPlayerController.Reset();

	// widget 의 GetOwningPlayer/GetWorld/GetOuter 가 유지되는 LocalPlayer/GameInstance 를 따라 새 값을 반환해 stale 여부를 알 수 없고, layout 은 빈 컨테이너라 매번 재생성해도 비용이 작다.
	if (PrimaryGameLayout)
	{
		PrimaryGameLayout->RemoveFromParent();
		PrimaryGameLayout = nullptr;
	}

	if (!PC)
	{
		return;
	}

	// 빈 layout 을 채우는 화면(HUD·사망·대화)은 컨트롤러의 UWxPlayerLayoutComponent 가 띄운다.
	CreateLayoutForPlayer(PC);
	TrackedPlayerController = PC;
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

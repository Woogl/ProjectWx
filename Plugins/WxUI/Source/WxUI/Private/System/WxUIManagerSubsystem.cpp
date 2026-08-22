// Copyright Woogle. All Rights Reserved.

#include "System/WxUIManagerSubsystem.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "System/WxPrimaryGameLayout.h"
#include "System/WxUIDeveloperSettings.h"
#include "Widget/WxGamePopup.h"
#include "Widget/WxActivatableWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Kismet/GameplayStatics.h"
#include "WxGameplayTags.h"
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

	for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
	{
		HandleLocalPlayerAdded(LocalPlayer);
	}

	GameInstance->OnLocalPlayerAddedEvent.AddUObject(this, &ThisClass::HandleLocalPlayerAdded);
}

void UWxUIManagerSubsystem::Deinitialize()
{
	// GameInstance 서브시스템이라 월드를 넘어 산다 — 붙잡아 둔 PC·ASC 구독을 여기서 끊는다.
	if (APlayerController* TrackedPC = TrackedPlayerController.Get())
	{
		TrackedPC->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	}
	TrackedPlayerController.Reset();
	WatchPawnTags(nullptr);

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

	}

	Super::Deinitialize();
}

UCommonActivatableWidget* UWxUIManagerSubsystem::PushContentToLayer(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	if (!PrimaryGameLayout || !WidgetClass)
	{
		return nullptr;
	}
	UCommonActivatableWidget* Widget = PrimaryGameLayout->PushWidgetToLayerStack(LayerTag, WidgetClass);
	ObserveWidgetForGamePause(Widget);
	return Widget;
}

UCommonActivatableWidget* UWxUIManagerSubsystem::PushSoftContentToLayer(FGameplayTag LayerTag, const TSoftClassPtr<UCommonActivatableWidget>& WidgetClass)
{
	if (WidgetClass.IsNull())
	{
		return nullptr;
	}

	return PushContentToLayer(LayerTag, WidgetClass.LoadSynchronous());
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
	const TSoftClassPtr<UWxGamePopup>& PopupClass = GetDefault<UWxUIDeveloperSettings>()->ConfirmationPopupClass;
	if (!PrimaryGameLayout || !Descriptor || PopupClass.IsNull())
	{
		return;
	}

	TSubclassOf<UWxGamePopup> LoadedClass = PopupClass.LoadSynchronous();
	if (!LoadedClass)
	{
		return;
	}

	UWxGamePopup* PushedPopup = PrimaryGameLayout->PushWidgetToLayerStack<UWxGamePopup>(WxGameplayTags::UI_Layer_Modal, LoadedClass,
		[Descriptor, ResultCallback](UWxGamePopup& Popup)
		{
			Popup.SetupPopup(Descriptor, ResultCallback);
		});
	ObserveWidgetForGamePause(PushedPopup);
}

UWxPrimaryGameLayout* UWxUIManagerSubsystem::GetPrimaryGameLayout() const
{
	return PrimaryGameLayout;
}

void UWxUIManagerSubsystem::SetGameHUDClass(const TSoftClassPtr<UWxHUDLayout>& InGameHUDClass)
{
	GameHUDClass = InGameHUDClass;
}

const TSoftClassPtr<UWxHUDLayout>& UWxUIManagerSubsystem::GetGameHUDClass() const
{
	return GameHUDClass;
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
	if (!PrimaryGameLayout)
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	if (!World || !World->IsNetMode(NM_Standalone))
	{
		return;
	}

	// 스택에 위젯이 하나뿐이면 비활성화 후에도 GetActiveWidget 이 그 위젯을 반환할 수 있어 IsActivated 로 걸러낸다.
	bool bWantsPause = false;
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
			bWantsPause = true;
			break;
		}
	}

	UGameplayStatics::SetGamePaused(World, bWantsPause);
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
	if (APlayerController* PreviousPC = TrackedPlayerController.Get())
	{
		PreviousPC->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	}
	TrackedPlayerController.Reset();
	WatchPawnTags(nullptr);

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

	CreateLayoutForPlayer(PC);

	// 빈 layout 을 채우는 컨텐츠(HUD)는 주입된 컴포넌트가 띄우고, 여기서는 폰 상태 태그 관찰만 빙의를 따라간다.
	PC->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	TrackedPlayerController = PC;

	// 이미 빙의를 마친 PC 일 수 있다(layout 재생성 경로). 그때는 신호가 다시 오지 않으므로 지금 폰으로 따라잡는다.
	HandlePossessedPawnChanged(nullptr, PC->GetPawn());
}

void UWxUIManagerSubsystem::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	WatchPawnTags(NewPawn);
}

void UWxUIManagerSubsystem::WatchPawnTags(APawn* Pawn)
{
	if (UAbilitySystemComponent* PreviousASC = WatchedAbilitySystem.Get())
	{
		PreviousASC->RegisterGameplayTagEvent(WxGameplayTags::Ability_Death, EGameplayTagEventType::NewOrRemoved).Remove(DeathTagHandle);
		PreviousASC->RegisterGameplayTagEvent(WxGameplayTags::State_Dialogue, EGameplayTagEventType::NewOrRemoved).Remove(DialogueTagHandle);
	}
	WatchedAbilitySystem.Reset();
	DeathTagHandle.Reset();
	DialogueTagHandle.Reset();

	// 관찰을 놓는 순간 대화 태그가 걷히는 것을 볼 수 없게 되므로, 열려 있던 대화 창은 여기서 닫는다.
	CloseDialogueScreen();

	// 캐릭터의 ASC 는 기본 서브오브젝트라 폰이 있으면 곧바로 잡힌다 — 늦은 도착을 기다릴 필요가 없다.
	// 태그는 WxCore 라 WxUI 가 다른 플러그인 타입을 알지 않아도 되므로, 사망·대화를 도메인 델리게이트가 아니라 태그로 듣는다.
	UAbilitySystemComponent* AbilitySystem = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!AbilitySystem)
	{
		return;
	}

	DeathTagHandle = AbilitySystem->RegisterGameplayTagEvent(WxGameplayTags::Ability_Death, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleDeathTagChanged);
	DialogueTagHandle = AbilitySystem->RegisterGameplayTagEvent(WxGameplayTags::State_Dialogue, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleDialogueTagChanged);
	WatchedAbilitySystem = AbilitySystem;
}

void UWxUIManagerSubsystem::HandleDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount <= 0)
	{
		return;
	}

	// 사망 화면은 걷어내지 않는다. 부활은 월드 리로드(TravelFromSaveFile)이고, 그때 layout 이 통째로 재생성된다.
	PushSoftContentToLayer(WxGameplayTags::UI_Layer_Menu, GetDefault<UWxUIDeveloperSettings>()->DeathScreenClass);
}

void UWxUIManagerSubsystem::HandleDialogueTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount <= 0)
	{
		CloseDialogueScreen();
		return;
	}

	// 대화 위젯은 Game 레이어 스택 top 에 얹혀 HUD 를 잠시 가리고, 닫히면 HUD 가 복귀한다.
	// 위젯의 뷰모델이 생성 시점에 세션의 현재 대사를 pull 하므로, 세션이 다 채워진 뒤에 오는 이 신호로 띄운다.
	DialogueScreen = PushSoftContentToLayer(WxGameplayTags::UI_Layer_Game, GetDefault<UWxUIDeveloperSettings>()->DialogueScreenClass);
}

void UWxUIManagerSubsystem::CloseDialogueScreen()
{
	// 띄운 쪽에서 닫는다. 태그가 걷히는 어느 경로로 끝나든 창이 남지 않는다.
	if (UCommonActivatableWidget* Screen = DialogueScreen.Get())
	{
		Screen->DeactivateWidget();
	}
	DialogueScreen.Reset();
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

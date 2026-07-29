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
	// GameInstance 서브시스템이라 월드를 넘어 산다 — 붙잡아 둔 PC·ASC 구독을 여기서 끊는다.
	if (APlayerController* TrackedPC = TrackedPlayerController.Get())
	{
		TrackedPC->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	}
	TrackedPlayerController.Reset();
	WatchPawnDeath(nullptr);

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

	// 활성화 이전에 호출되는 초기화 콜백에서 SetupPopup 를 실행해 표시 전에 내용을 채운다.
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

	// 전 레이어의 현재 활성 위젯 중 정지를 원하는 것이 하나라도 있으면 정지한다.
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
	// 이전 PC 의 빙의 구독을 끊는다. 그 폰의 사망 관찰도 함께 정리한다.
	if (APlayerController* PreviousPC = TrackedPlayerController.Get())
	{
		PreviousPC->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	}
	TrackedPlayerController.Reset();
	WatchPawnDeath(nullptr);

	// PC 가 들어올 때마다 layout 을 무조건 재생성한다.
	// stale 식별을 시도하지 않는 이유: widget 의 GetOwningPlayer/GetWorld/GetOuter 가 모두 유지되는 LocalPlayer/GameInstance 를 따라 자동으로 새 값을 반환하므로, widget 만 보고는 stale 여부를 알 수 없다.
	// layout 은 빈 컨테이너이고 컨텐츠는 아래에서 다시 push 되므로 매번 재생성해도 비용이 작다.
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

	// 빈 layout 에 컨텐츠를 채우는 것은 빙의를 따라간다. 예전엔 PC 가 OnPossess/OnRep_Pawn 에서 직접 push 했다.
	PC->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	TrackedPlayerController = PC;

	// 이미 빙의를 마친 PC 일 수 있다(layout 재생성 경로). 그때는 신호가 다시 오지 않으므로 지금 폰으로 따라잡는다.
	HandlePossessedPawnChanged(nullptr, PC->GetPawn());
}

void UWxUIManagerSubsystem::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	WatchPawnDeath(NewPawn);

	if (!NewPawn)
	{
		return;
	}

	PushSoftContentToLayer(WxGameplayTags::UI_Layer_Game, GetDefault<UWxUIDeveloperSettings>()->GameHUDClass);
}

void UWxUIManagerSubsystem::WatchPawnDeath(APawn* Pawn)
{
	if (UAbilitySystemComponent* PreviousASC = WatchedAbilitySystem.Get())
	{
		PreviousASC->RegisterGameplayTagEvent(WxGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved).Remove(DeathTagHandle);
	}
	WatchedAbilitySystem.Reset();
	DeathTagHandle.Reset();

	// 캐릭터의 ASC 는 기본 서브오브젝트라 폰이 있으면 곧바로 잡힌다 — 늦은 도착을 기다릴 필요가 없다.
	// 사망을 캐릭터 델리게이트가 아니라 태그로 듣는 이유: 그 델리게이트도 결국 이 태그가 출처이고, 태그는 WxCore 라 WxUI 가 게임 모듈 타입을 알지 않아도 된다.
	UAbilitySystemComponent* AbilitySystem = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!AbilitySystem)
	{
		return;
	}

	DeathTagHandle = AbilitySystem->RegisterGameplayTagEvent(WxGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleDeathTagChanged);
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

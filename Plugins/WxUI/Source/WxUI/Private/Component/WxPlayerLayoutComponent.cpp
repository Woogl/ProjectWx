// Copyright Woogle. All Rights Reserved.

#include "Component/WxPlayerLayoutComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "CommonActivatableWidget.h"
#include "GameFramework/PlayerController.h"
#include "System/WxPrimaryGameLayout.h"
#include "Widget/WxActivatableWidget.h"
#include "Widget/WxAsyncAction_PushWidgetToLayer.h"
#include "Widget/WxHUDLayout.h"
#include "WxGameplayTags.h"
#include "WxUILibrary.h"

void UWxPlayerLayoutComponent::BeginPlay()
{
	Super::BeginPlay();

	// 띄울 화면이 없는 원격 사본(데디 서버가 든 PC)은 여기서 걸러낸다.
	APlayerController* OwningController = Cast<APlayerController>(GetOwner());
	if (!OwningController || !OwningController->IsLocalController())
	{
		return;
	}

	OwningController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::HandlePossessedPawnChanged);

	// BeginPlay 가 빙의보다 늦으면 신호가 다시 오지 않으므로, 지금 폰으로 따라잡는다.
	HandlePossessedPawnChanged(nullptr, OwningController->GetPawn());
}

void UWxPlayerLayoutComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* OwningController = Cast<APlayerController>(GetOwner()))
	{
		OwningController->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	}

	WatchPawnTags(nullptr);
	ClearLayout();

	Super::EndPlay(EndPlayReason);
}

void UWxPlayerLayoutComponent::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	// ViewModel은 생성 당시 Pawn의 ASC를 소유자로 삼으므로 빙의 해제·교체 때 함께 걷는다.
	if (OldPawn != NewPawn)
	{
		ClearLayout();
		WatchPawnTags(NewPawn);
	}
	if (!NewPawn || LayoutClass.IsNull())
	{
		return;
	}

	UWxPrimaryGameLayout* Layout = UWxUILibrary::GetPrimaryGameLayout(this);
	if (!Layout)
	{
		return;
	}

	// 같은 Pawn 알림은 기존 HUD를 유지한다. CommonUI 풀에 남은 참조는 스택 포함 여부로 구분한다.
	const UCommonActivatableWidgetStack* GameStack = Layout->GetLayerWidgetStack(WxGameplayTags::UI_Layer_Game);
	if (GameStack && GameStack->GetWidgetList().Contains(LayoutWidget.Get()))
	{
		return;
	}
	if (PendingLayoutPush)
	{
		return;
	}

	PendingLayoutPush = UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer(this, WxGameplayTags::UI_Layer_Game, LayoutClass);
	PendingLayoutPush->SetCompletionCallback(
		FWxPushWidgetToLayerNativeDelegate::CreateUObject(this, &ThisClass::HandleLayoutPushCompleted));
	PendingLayoutPush->Activate();
}

void UWxPlayerLayoutComponent::HandleLayoutPushCompleted(UCommonActivatableWidget* Widget)
{
	PendingLayoutPush = nullptr;
	LayoutWidget = Widget;
}

void UWxPlayerLayoutComponent::ClearLayout()
{
	if (PendingLayoutPush)
	{
		PendingLayoutPush->Cancel();
		PendingLayoutPush = nullptr;
	}
	UWxPrimaryGameLayout* Layout = UWxUILibrary::GetPrimaryGameLayout(this);
	UCommonActivatableWidgetStack* Stack = Layout ? Layout->GetLayerWidgetStack(WxGameplayTags::UI_Layer_Game) : nullptr;
	if (UCommonActivatableWidget* Widget = LayoutWidget.Get())
	{
		Widget->DeactivateWidget();
		if (Stack)
		{
			Stack->RemoveWidget(*Widget);
		}
	}
	LayoutWidget.Reset();
}

void UWxPlayerLayoutComponent::WatchPawnTags(APawn* Pawn)
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
	// 사망 화면은 닫지 않는다 — 부활이 폰을 교체하며, 부활 요청이 완료 시 사망 화면을 비활성화한다.
	CloseDialogueScreen();

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

void UWxPlayerLayoutComponent::HandleDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount <= 0)
	{
		return;
	}

	UWxAsyncAction_PushWidgetToLayer* PushAction = UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer(
		this, WxGameplayTags::UI_Layer_Menu, DeathScreenClass);
	PushAction->Activate();
}

void UWxPlayerLayoutComponent::HandleDialogueTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount <= 0)
	{
		CloseDialogueScreen();
		return;
	}

	// 대화 위젯은 Game 레이어 스택 top 에 얹혀 HUD 를 잠시 가리고, 닫히면 HUD 가 복귀한다.
	// 대화 화면이 활성화될 때 현재 대사로 표시를 채우므로, 세션이 다 채워진 뒤에 오는 이 신호로 띄운다.
	PendingDialogueScreenPush = UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer(
		this, WxGameplayTags::UI_Layer_Game, DialogueScreenClass);
	PendingDialogueScreenPush->SetCompletionCallback(
		FWxPushWidgetToLayerNativeDelegate::CreateUObject(this, &ThisClass::HandleDialogueScreenPushCompleted));
	PendingDialogueScreenPush->Activate();
}

void UWxPlayerLayoutComponent::HandleDialogueScreenPushCompleted(UCommonActivatableWidget* Widget)
{
	PendingDialogueScreenPush = nullptr;
	if (!Widget)
	{
		return;
	}

	UAbilitySystemComponent* AbilitySystem = WatchedAbilitySystem.Get();
	if (!AbilitySystem || !AbilitySystem->HasMatchingGameplayTag(WxGameplayTags::State_Dialogue))
	{
		Widget->DeactivateWidget();
		return;
	}

	DialogueScreen = Widget;
}

void UWxPlayerLayoutComponent::CloseDialogueScreen()
{
	if (PendingDialogueScreenPush)
	{
		PendingDialogueScreenPush->Cancel();
		PendingDialogueScreenPush = nullptr;
	}

	// 띄운 쪽에서 닫는다. 태그가 걷히는 어느 경로로 끝나든 창이 남지 않는다.
	if (UCommonActivatableWidget* Screen = DialogueScreen.Get())
	{
		Screen->DeactivateWidget();
	}
	DialogueScreen.Reset();
}

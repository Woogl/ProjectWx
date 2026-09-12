// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "UObject/StrongObjectPtr.h"
#include "Widget/WxGamePopup.h"
#include "WxUIManagerSubsystem.generated.h"

class APawn;
class UAbilitySystemComponent;
class UWxPrimaryGameLayout;
class UCommonActivatableWidget;
class UWxGamePopupDescriptor;
class UWxAsyncAction_PushWidgetToLayer;

UCLASS()
class WXUI_API UWxUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	UCommonActivatableWidget* PushWidgetInstanceToLayer(FGameplayTag LayerTag, UCommonActivatableWidget* WidgetInstance);

	/** 확인 팝업을 Modal 레이어에 띄운다. */
	void ShowConfirmation(UWxGamePopupDescriptor* Descriptor, FWxPopupResultDelegate ResultCallback = FWxPopupResultDelegate());

	UWxPrimaryGameLayout* GetPrimaryGameLayout() const;

	/** 화면을 차지하는 메뉴(Menu·Modal 레이어)가 떠 있는지. */
	bool IsMenuLayerActive() const;

private:
	bool HasActiveWidgetInLayer(FGameplayTag LayerTag) const;

	void HandleLocalPlayerAdded(ULocalPlayer* LocalPlayer);

	void HandlePlayerControllerSet(APlayerController* PC);

	void CreateLayoutForPlayer(APlayerController* PC);

	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	/** 폰 ASC 의 상태 태그(사망·대화)를 관찰하기 시작한다. 이전 관찰은 먼저 끊는다 — 폰이 null 이면 끊기만 한다. */
	void WatchPawnTags(APawn* Pawn);

	/** 사망 태그가 부여되면 사망 화면을 띄운다. */
	void HandleDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	/** 대화 세션이 열리면 대화 창을 띄우고, 닫히면 걷는다. */
	void HandleDialogueTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	/** 팝업은 활성화되기 전에 내용을 채워야 하므로, push 직전에 서술자를 넘긴다. */
	void HandleConfirmationPopupReady(UCommonActivatableWidget* Widget, TStrongObjectPtr<UWxGamePopupDescriptor> Descriptor, FWxPopupResultDelegate ResultCallback);

	void HandleDialogueScreenPushCompleted(UCommonActivatableWidget* Widget);

	void CloseDialogueScreen();

	/** 위젯은 서브시스템을 알지 못하므로, 활성/비활성 델리게이트를 이쪽에서 구독한다. */
	void ObserveWidgetForGamePause(UCommonActivatableWidget* Widget);

	void HandleObservedWidgetActivationChanged();

	void RefreshGamePause();

	/** 전 레이어의 활성 위젯을 순회해, 정지를 원하는 위젯이 하나라도 있는지 본다. */
	bool WantsGamePause() const;

	/** 정지를 풀어도 되는지 게임모드가 해제 직전 되묻는 콜백. */
	bool HandleCanUnpause();

	/**
	 * 로컬 플레이어 하나를 전제로 레이아웃과 아래 추적 상태를 단수로 둔다.
	 * 스플릿스크린이 필요해지면 이것들을 ULocalPlayer 키로 묶어야 한다 — 지금 구조에선 나중에 붙은 플레이어가 앞의 것을 갈아치운다.
	 */
	UPROPERTY()
	TObjectPtr<UWxPrimaryGameLayout> PrimaryGameLayout;

	/** 빙의를 구독해 둔 로컬 PC. 교체·종료 때 같은 PC 에서 끊기 위해 기억한다. */
	TWeakObjectPtr<APlayerController> TrackedPlayerController;

	/** 상태 태그를 구독해 둔 폰 ASC. 폰이 바뀌면 같은 ASC 에서 끊기 위해 기억한다. */
	TWeakObjectPtr<UAbilitySystemComponent> WatchedAbilitySystem;

	FDelegateHandle DeathTagHandle;

	FDelegateHandle DialogueTagHandle;

	/** 대화 중 띄워 둔 대화 창. 세션이 끝날 때 이 창을 닫기 위해 기억한다. */
	TWeakObjectPtr<UCommonActivatableWidget> DialogueScreen;

	/** 대화 태그가 먼저 걷히면 화면이 뒤늦게 나타나지 않도록 취소할 진행 중인 요청. */
	UPROPERTY()
	TObjectPtr<UWxAsyncAction_PushWidgetToLayer> PendingDialogueScreenPush;
};

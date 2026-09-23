// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Widget/WxGamePopup.h"
#include "WxUIManagerSubsystem.generated.h"

class UWxPrimaryGameLayout;
class UCommonActivatableWidget;
class UWxGamePopupDescriptor;

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

	/** 위젯은 서브시스템을 알지 못하므로, 활성/비활성 델리게이트를 이쪽에서 구독한다. */
	void ObserveWidgetForGamePause(UCommonActivatableWidget* Widget);

	void HandleObservedWidgetActivationChanged();

	void RefreshGamePause();

	bool WantsGamePause() const;

	/** 정지를 풀어도 되는지 게임모드가 해제 직전 되묻는 콜백. */
	bool HandleCanUnpause();

	/**
	 * 로컬 플레이어 하나를 전제로 레이아웃과 아래 추적 상태를 단수로 둔다.
	 * 스플릿스크린이 필요해지면 이것들을 ULocalPlayer 키로 묶어야 한다 — 지금 구조에선 나중에 붙은 플레이어가 앞의 것을 갈아치운다.
	 */
	UPROPERTY()
	TObjectPtr<UWxPrimaryGameLayout> PrimaryGameLayout;

	UPROPERTY(Transient)
	TSubclassOf<UWxGamePopup> ConfirmationPopupClass;

	/** 게임 일시정지를 거는 로컬 PC. */
	TWeakObjectPtr<APlayerController> TrackedPlayerController;
};

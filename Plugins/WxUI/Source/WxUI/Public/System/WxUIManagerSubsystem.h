// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Widget/WxGamePopup.h"
#include "WxUIManagerSubsystem.generated.h"

class UWxPrimaryGameLayout;
class UCommonActivatableWidget;
class UWxViewModel_Selection;
class UWxGamePopupDescriptor;

UCLASS()
class WXUI_API UWxUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	UCommonActivatableWidget* PushContentToLayer(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass);

	UCommonActivatableWidget* PushWidgetInstanceToLayer(FGameplayTag LayerTag, UCommonActivatableWidget* WidgetInstance);

	/** 확인 팝업을 Modal 레이어에 띄운다. 결과는 ResultCallback 으로 돌려준다. */
	void ShowConfirmation(UWxGamePopupDescriptor* Descriptor, FWxPopupResultDelegate ResultCallback = FWxPopupResultDelegate());

	/** 에러 팝업을 Modal 레이어에 띄운다. 결과는 ResultCallback 으로 돌려준다. */
	void ShowError(UWxGamePopupDescriptor* Descriptor, FWxPopupResultDelegate ResultCallback = FWxPopupResultDelegate());

	UWxPrimaryGameLayout* GetPrimaryGameLayout() const;

	/** 전 위젯이 공유하는 범용 "현재 선택" 글로벌 뷰모델. 도메인 소스(상호작용/인벤토리 등)가 표시 데이터를 push 한다. */
	UWxViewModel_Selection* GetSelectionViewModel() const { return SelectionViewModel; }

private:
	void HandleLocalPlayerAdded(ULocalPlayer* LocalPlayer);

	void HandlePlayerControllerSet(APlayerController* PC);

	void CreateLayoutForPlayer(APlayerController* PC);

	/** ShowConfirmation/ShowError 공통 경로. 클래스를 로드해 Modal 레이어에 push 하고 SetupPopup 를 호출한다. */
	void PushPopup(const TSoftClassPtr<UWxGamePopup>& PopupClass, UWxGamePopupDescriptor* Descriptor, FWxPopupResultDelegate ResultCallback);

	/** push 된 위젯의 활성/비활성 델리게이트를 구독해, 상태가 바뀔 때 정지 재평가가 돌게 한다. 위젯은 서브시스템을 알지 못한다. */
	void ObserveWidgetForGamePause(UCommonActivatableWidget* Widget);

	/** 구독한 위젯이 활성/비활성될 때 호출된다. */
	void HandleObservedWidgetActivationChanged();

	/** 전 레이어의 활성 위젯을 순회해, 정지를 원하는 활성 위젯이 하나라도 있으면 게임을 정지(아니면 해제)한다. */
	void RefreshGamePause();

	UPROPERTY()
	TObjectPtr<UWxPrimaryGameLayout> PrimaryGameLayout;

	/** 글로벌 컬렉션(UMVVMGameSubsystem)에 "VM_Selection" 으로 등록되는 공유 선택 뷰모델. Initialize 생성, Deinitialize 해제. */
	UPROPERTY()
	TObjectPtr<UWxViewModel_Selection> SelectionViewModel;
};

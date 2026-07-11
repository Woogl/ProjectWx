// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Widget/WxGameDialog.h"
#include "WxUIManagerSubsystem.generated.h"

class UWxPrimaryGameLayout;
class UCommonActivatableWidget;
class UWxViewModel_Selection;
class UWxGameDialogDescriptor;

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
	void ShowConfirmation(UWxGameDialogDescriptor* Descriptor, FWxMessagingResultDelegate ResultCallback = FWxMessagingResultDelegate());

	/** 에러 팝업을 Modal 레이어에 띄운다. 결과는 ResultCallback 으로 돌려준다. */
	void ShowError(UWxGameDialogDescriptor* Descriptor, FWxMessagingResultDelegate ResultCallback = FWxMessagingResultDelegate());

	UWxPrimaryGameLayout* GetPrimaryGameLayout() const;

	/** 전 위젯이 공유하는 범용 "현재 선택" 글로벌 뷰모델. 도메인 소스(상호작용/인벤토리 등)가 표시 데이터를 push 한다. */
	UWxViewModel_Selection* GetSelectionViewModel() const { return SelectionViewModel; }

private:
	void HandleLocalPlayerAdded(ULocalPlayer* LocalPlayer);

	void HandlePlayerControllerSet(APlayerController* PC);

	void CreateLayoutForPlayer(APlayerController* PC);

	/** ShowConfirmation/ShowError 공통 경로. 클래스를 로드해 Modal 레이어에 push 하고 SetupDialog 를 호출한다. */
	void PushDialog(const TSoftClassPtr<UWxGameDialog>& DialogClass, UWxGameDialogDescriptor* Descriptor, FWxMessagingResultDelegate ResultCallback);

	UPROPERTY()
	TObjectPtr<UWxPrimaryGameLayout> PrimaryGameLayout;

	/** 글로벌 컬렉션(UMVVMGameSubsystem)에 "VM_Selection" 으로 등록되는 공유 선택 뷰모델. Initialize 생성, Deinitialize 해제. */
	UPROPERTY()
	TObjectPtr<UWxViewModel_Selection> SelectionViewModel;
};

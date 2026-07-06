// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "WxUIManagerSubsystem.generated.h"

class UWxPrimaryGameLayout;
class UCommonActivatableWidget;
class UWxViewModel_Selection;

UCLASS()
class WXUI_API UWxUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	UCommonActivatableWidget* PushContentToLayer(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass);

	UCommonActivatableWidget* PushWidgetInstanceToLayer(FGameplayTag LayerTag, UCommonActivatableWidget* WidgetInstance);

	UWxPrimaryGameLayout* GetPrimaryGameLayout() const;

	/** 전 위젯이 공유하는 범용 "현재 선택" 글로벌 뷰모델. 도메인 소스(상호작용/인벤토리 등)가 표시 데이터를 push 한다. */
	UWxViewModel_Selection* GetSelectionViewModel() const { return SelectionViewModel; }

private:
	void HandleLocalPlayerAdded(ULocalPlayer* LocalPlayer);

	void HandlePlayerControllerSet(APlayerController* PC);

	void CreateLayoutForPlayer(APlayerController* PC);

	UPROPERTY()
	TObjectPtr<UWxPrimaryGameLayout> PrimaryGameLayout;

	/** 글로벌 컬렉션(UMVVMGameSubsystem)에 "VM_Selection" 으로 등록되는 공유 선택 뷰모델. Initialize 생성, Deinitialize 해제. */
	UPROPERTY()
	TObjectPtr<UWxViewModel_Selection> SelectionViewModel;
};

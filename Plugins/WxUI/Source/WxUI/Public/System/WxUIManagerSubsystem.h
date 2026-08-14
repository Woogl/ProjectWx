// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Widget/WxGamePopup.h"
#include "WxUIManagerSubsystem.generated.h"

class APawn;
class UAbilitySystemComponent;
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

	/** 소프트 클래스를 동기 로드해 레이어에 push 한다. */
	UCommonActivatableWidget* PushSoftContentToLayer(FGameplayTag LayerTag, const TSoftClassPtr<UCommonActivatableWidget>& WidgetClass);

	UCommonActivatableWidget* PushWidgetInstanceToLayer(FGameplayTag LayerTag, UCommonActivatableWidget* WidgetInstance);

	/** 확인 팝업을 Modal 레이어에 띄운다. */
	void ShowConfirmation(UWxGamePopupDescriptor* Descriptor, FWxPopupResultDelegate ResultCallback = FWxPopupResultDelegate());

	UWxPrimaryGameLayout* GetPrimaryGameLayout() const;

	/**
	 * 전 위젯이 공유하는 범용 "현재 선택" 글로벌 뷰모델.
	 * 도메인 소스(상호작용/인벤토리 등)가 표시 데이터를 push 한다.
	 */
	UWxViewModel_Selection* GetSelectionViewModel() const;

private:
	void HandleLocalPlayerAdded(ULocalPlayer* LocalPlayer);

	void HandlePlayerControllerSet(APlayerController* PC);

	void CreateLayoutForPlayer(APlayerController* PC);

	/** 이 신호는 빙의가 끝난 뒤에 오므로, HUD 뷰모델 리졸버가 생성 시점에 빙의 폰의 ASC 를 읽는다는 전제가 지켜진다. */
	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	/** 폰 ASC 의 상태 태그(사망·대화)를 관찰하기 시작한다. 이전 관찰은 먼저 끊는다 — 폰이 null 이면 끊기만 한다. */
	void WatchPawnTags(APawn* Pawn);

	/** 사망 태그가 부여되면 사망 화면을 띄운다. */
	void HandleDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	/** 대화 세션이 열리면 대화 창을 띄우고, 닫히면 걷는다. */
	void HandleDialogueTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	void CloseDialogueScreen();

	/** 위젯은 서브시스템을 알지 못하므로, 활성/비활성 델리게이트를 이쪽에서 구독한다. */
	void ObserveWidgetForGamePause(UCommonActivatableWidget* Widget);

	void HandleObservedWidgetActivationChanged();

	/** 전 레이어의 활성 위젯을 순회해, 정지를 원하는 활성 위젯이 하나라도 있으면 게임을 정지(아니면 해제)한다. */
	void RefreshGamePause();

	UPROPERTY()
	TObjectPtr<UWxPrimaryGameLayout> PrimaryGameLayout;

	/** 글로벌 컬렉션(UMVVMGameSubsystem)에 "VM_Selection" 으로 등록되는 공유 선택 뷰모델. */
	UPROPERTY()
	TObjectPtr<UWxViewModel_Selection> SelectionViewModel;

	/** 빙의를 구독해 둔 로컬 PC. 교체·종료 때 같은 PC 에서 끊기 위해 기억한다. */
	TWeakObjectPtr<APlayerController> TrackedPlayerController;

	/** 상태 태그를 구독해 둔 폰 ASC. 폰이 바뀌면 같은 ASC 에서 끊기 위해 기억한다. */
	TWeakObjectPtr<UAbilitySystemComponent> WatchedAbilitySystem;

	FDelegateHandle DeathTagHandle;

	FDelegateHandle DialogueTagHandle;

	/** 대화 중 띄워 둔 대화 창. 세션이 끝날 때 이 창을 닫기 위해 기억한다. */
	TWeakObjectPtr<UCommonActivatableWidget> DialogueScreen;

	/** Game 레이어에 띄워 둔 HUD. 폰이 갈아탈 때마다 다시 push 하지 않고 이 인스턴스를 그대로 쓰기 위해 기억한다. */
	TWeakObjectPtr<UCommonActivatableWidget> GameHUD;
};

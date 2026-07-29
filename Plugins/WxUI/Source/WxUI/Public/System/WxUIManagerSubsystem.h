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

	/**
	 * 소프트 클래스를 동기 로드해 레이어에 push 한다.
	 * 클래스 미지정·로드 실패면 아무 것도 하지 않는다. 로드 지연이 문제되는 경로는 UWxAsyncAction_PushWidgetToLayer 를 쓴다.
	 */
	UCommonActivatableWidget* PushSoftContentToLayer(FGameplayTag LayerTag, const TSoftClassPtr<UCommonActivatableWidget>& WidgetClass);

	UCommonActivatableWidget* PushWidgetInstanceToLayer(FGameplayTag LayerTag, UCommonActivatableWidget* WidgetInstance);

	/**
	 * 확인 팝업을 Modal 레이어에 띄운다.
	 * 결과는 ResultCallback 으로 돌려준다.
	 */
	void ShowConfirmation(UWxGamePopupDescriptor* Descriptor, FWxPopupResultDelegate ResultCallback = FWxPopupResultDelegate());

	UWxPrimaryGameLayout* GetPrimaryGameLayout() const;

	/**
	 * 전 위젯이 공유하는 범용 "현재 선택" 글로벌 뷰모델.
	 * 도메인 소스(상호작용/인벤토리 등)가 표시 데이터를 push 한다.
	 */
	UWxViewModel_Selection* GetSelectionViewModel() const { return SelectionViewModel; }

private:
	void HandleLocalPlayerAdded(ULocalPlayer* LocalPlayer);

	void HandlePlayerControllerSet(APlayerController* PC);

	void CreateLayoutForPlayer(APlayerController* PC);

	/**
	 * 추적 중인 PC 가 빙의한 폰을 갈아탔다. 그 폰 기준으로 HUD 를 세우고 사망 관찰을 옮긴다.
	 * 이 신호는 빙의가 끝난 뒤에 오므로, HUD 뷰모델 리졸버가 생성 시점에 빙의 폰의 ASC 를 읽는다는 전제가 지켜진다.
	 */
	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	/** 폰 ASC 의 사망 태그를 관찰하기 시작한다. 이전 관찰은 먼저 끊는다 — 폰이 null 이면 끊기만 한다. */
	void WatchPawnDeath(APawn* Pawn);

	/** 사망 태그 변화 콜백. 부여되면 사망 화면을 띄운다. */
	void HandleDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	/**
	 * push 된 위젯의 활성/비활성 델리게이트를 구독해, 상태가 바뀔 때 정지 재평가가 돌게 한다.
	 * 위젯은 서브시스템을 알지 못한다.
	 */
	void ObserveWidgetForGamePause(UCommonActivatableWidget* Widget);

	/** 구독한 위젯이 활성/비활성될 때 호출된다. */
	void HandleObservedWidgetActivationChanged();

	/** 전 레이어의 활성 위젯을 순회해, 정지를 원하는 활성 위젯이 하나라도 있으면 게임을 정지(아니면 해제)한다. */
	void RefreshGamePause();

	UPROPERTY()
	TObjectPtr<UWxPrimaryGameLayout> PrimaryGameLayout;

	/**
	 * 글로벌 컬렉션(UMVVMGameSubsystem)에 "VM_Selection" 으로 등록되는 공유 선택 뷰모델.
	 * Initialize 생성, Deinitialize 해제.
	 */
	UPROPERTY()
	TObjectPtr<UWxViewModel_Selection> SelectionViewModel;

	/** 빙의를 구독해 둔 로컬 PC. 교체·종료 때 같은 PC 에서 끊기 위해 기억한다. */
	TWeakObjectPtr<APlayerController> TrackedPlayerController;

	/** 사망 태그를 구독해 둔 폰 ASC. 폰이 바뀌면 같은 ASC 에서 끊기 위해 기억한다. */
	TWeakObjectPtr<UAbilitySystemComponent> WatchedAbilitySystem;

	FDelegateHandle DeathTagHandle;
};

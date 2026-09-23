// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "WxPlayerLayoutComponent.generated.h"

class APawn;
class UAbilitySystemComponent;
class UCommonActivatableWidget;
class UWxActivatableWidget;
class UWxAsyncAction_PushWidgetToLayer;
class UWxHUDLayout;

/**
 * 로컬 플레이어의 화면(HUD·사망·대화)을 띄우고, 컴포넌트가 걷힐 때 함께 걷는 컨트롤러 컴포넌트.
 * 클래스를 비운 화면은 띄우지 않는다.
 */
UCLASS()
class WXUI_API UWxPlayerLayoutComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** 이 신호는 빙의가 끝난 뒤에 오므로, HUD 뷰모델 리졸버가 생성 시점에 빙의 폰의 ASC 를 읽는다는 전제가 지켜진다. */
	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	void HandleLayoutPushCompleted(UCommonActivatableWidget* Widget);
	void ClearLayout();

	/** 폰 ASC 의 상태 태그(사망·대화)를 관찰한다. 이전 관찰은 먼저 끊고, 폰이 null 이면 끊기만 한다. */
	void WatchPawnTags(APawn* Pawn);

	void HandleDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	void HandleDialogueTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	void HandleDialogueScreenPushCompleted(UCommonActivatableWidget* Widget);
	void CloseDialogueScreen();

	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TSoftClassPtr<UWxHUDLayout> LayoutClass;

	/** 빙의된 캐릭터 사망 시 Menu 레이어에 띄울 위젯. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TSoftClassPtr<UWxActivatableWidget> DeathScreenClass;

	/** 대화 세션이 열리면 Game 레이어에 띄울 위젯. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TSoftClassPtr<UWxActivatableWidget> DialogueScreenClass;

	/** 현재 빙의 Pawn의 ViewModel을 사용하는 HUD. */
	TWeakObjectPtr<UCommonActivatableWidget> LayoutWidget;

	/** 컴포넌트가 먼저 걷히면 HUD 가 뒤늦게 나타나지 않도록 취소할 진행 중인 요청. */
	UPROPERTY()
	TObjectPtr<UWxAsyncAction_PushWidgetToLayer> PendingLayoutPush;

	/** 상태 태그를 구독해 둔 폰 ASC. 폰이 바뀌면 같은 ASC 에서 끊기 위해 기억한다. */
	TWeakObjectPtr<UAbilitySystemComponent> WatchedAbilitySystem;

	FDelegateHandle DeathTagHandle;

	FDelegateHandle DialogueTagHandle;

	/** 세션이 끝날 때 닫기 위해 기억해 두는, 대화 중 띄운 창. */
	TWeakObjectPtr<UCommonActivatableWidget> DialogueScreen;

	/** 대화 태그가 먼저 걷히면 화면이 뒤늦게 나타나지 않도록 취소할 진행 중인 요청. */
	UPROPERTY()
	TObjectPtr<UWxAsyncAction_PushWidgetToLayer> PendingDialogueScreenPush;
};

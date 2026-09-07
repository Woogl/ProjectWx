// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WxPlayerLayoutComponent.generated.h"

class APawn;
class UCommonActivatableWidget;
class UWxAsyncAction_PushWidgetToLayer;
class UWxHUDLayout;

/**
 * 로컬 플레이어의 화면을 Game 레이어에 띄우고, 컴포넌트가 걷힐 때 함께 걷는 컨트롤러 컴포넌트.
 * 컨트롤러 BP 마다 LayoutClass로 전투 HUD 또는 프론트엔드 메뉴를 지정한다. 비우면 띄우지 않는다.
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

	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TSoftClassPtr<UWxHUDLayout> LayoutClass;

	/** 현재 빙의 Pawn의 ViewModel을 사용하는 HUD. */
	TWeakObjectPtr<UCommonActivatableWidget> LayoutWidget;

	/** 컴포넌트가 먼저 걷히면 HUD 가 뒤늦게 나타나지 않도록 취소할 진행 중인 요청. */
	UPROPERTY()
	TObjectPtr<UWxAsyncAction_PushWidgetToLayer> PendingLayoutPush;
};

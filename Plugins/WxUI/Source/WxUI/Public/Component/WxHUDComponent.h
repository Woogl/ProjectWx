// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "WxHUDComponent.generated.h"

class APawn;
class UCommonActivatableWidget;
class UWxAsyncAction_PushWidgetToLayer;

/**
 * 로컬 플레이어의 HUD 를 Game 레이어에 띄우고, 컴포넌트가 걷힐 때 함께 걷는 컨트롤러 컴포넌트.
 * 띄울 HUD 는 Experience 가 UI 매니저에 발행한 값을 쓰고, 부착도 Experience 에셋의 주입 목록으로 한다(컨트롤러는 본 클래스를 모른다).
 */
UCLASS()
class WXUI_API UWxHUDComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** 이 신호는 빙의가 끝난 뒤에 오므로, HUD 뷰모델 리졸버가 생성 시점에 빙의 폰의 ASC 를 읽는다는 전제가 지켜진다. */
	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	void HandleHUDPushCompleted(UCommonActivatableWidget* Widget);

	/** 폰이 갈아탈 때마다 다시 push 하지 않고 이 인스턴스를 그대로 쓴다. */
	TWeakObjectPtr<UCommonActivatableWidget> HUDWidget;

	/** 컴포넌트가 먼저 걷히면 HUD 가 뒤늦게 나타나지 않도록 취소할 진행 중인 요청. */
	UPROPERTY()
	TObjectPtr<UWxAsyncAction_PushWidgetToLayer> PendingHUDPush;
};

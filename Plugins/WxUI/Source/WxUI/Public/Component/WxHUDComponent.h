// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WxHUDComponent.generated.h"

class APawn;
class UCommonActivatableWidget;
class UWxAsyncAction_PushWidgetToLayer;
class UWxHUDLayout;

/**
 * 로컬 플레이어의 HUD 를 Game 레이어에 띄우고, 컴포넌트가 걷힐 때 함께 걷는 컨트롤러 컴포넌트.
 * 띄울 HUD 는 자기 프로퍼티다 — 컨트롤러 BP 마다 다른 화면(전투 HUD·프론트엔드 메뉴)을 지정한다. 비우면 띄우지 않는다.
 */
UCLASS()
class WXUI_API UWxHUDComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	const TSoftClassPtr<UWxHUDLayout>& GetGameHUDClass() const;

private:
	/** 이 신호는 빙의가 끝난 뒤에 오므로, HUD 뷰모델 리졸버가 생성 시점에 빙의 폰의 ASC 를 읽는다는 전제가 지켜진다. */
	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	void HandleHUDPushCompleted(UCommonActivatableWidget* Widget);
	void ClearHUD();

	/** 빙의한 로컬 플레이어에게 띄울 HUD. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TSoftClassPtr<UWxHUDLayout> GameHUDClass;

	/** 현재 빙의 Pawn의 ViewModel을 사용하는 HUD. 빙의 대상 변경 시 교체한다. */
	TWeakObjectPtr<UCommonActivatableWidget> HUDWidget;

	/** 컴포넌트가 먼저 걷히면 HUD 가 뒤늦게 나타나지 않도록 취소할 진행 중인 요청. */
	UPROPERTY()
	TObjectPtr<UWxAsyncAction_PushWidgetToLayer> PendingHUDPush;
};

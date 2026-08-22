// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "WxHUDComponent.generated.h"

class APawn;
class UCommonActivatableWidget;

/**
 * 로컬 플레이어의 HUD 를 Game 레이어에 띄우고, 컴포넌트가 걷힐 때 함께 걷는 컨트롤러 컴포넌트.
 * 어떤 HUD 인지는 UI 밖(Experience)이 정해 UI 매니저에 발행한 값을 그대로 쓴다 — 콘텐츠마다 다른 HUD 를 끼우는 자리가 거기다.
 * 부착은 코드가 아니라 Experience 에셋의 주입 목록으로 한다(컨트롤러는 본 클래스를 모른다).
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

	/** 띄워 둔 HUD. 폰이 갈아탈 때마다 다시 push 하지 않고 이 인스턴스를 그대로 쓰기 위해 기억한다. */
	TWeakObjectPtr<UCommonActivatableWidget> HUDWidget;
};

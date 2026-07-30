// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WxPlayerController.generated.h"

/**
 * 플레이어 컨트롤러.
 * 게임플레이 입력(이동/시선/어빌리티)은 AWxPlayerCharacter가, 메뉴 토글 입력은 UWxHUDLayout(CommonUI 액션)이 소유한다.
 *
 * ModularGameplay 컴포넌트 receiver 다 — Experience 주입으로 요청 등록된 컨트롤러 컴포넌트(인벤토리·상호작용 스캐너·대화 세션·PlayerSpawn 등)를 자동 주입받으며, 어떤 컴포넌트가 붙는지 알지 않는다.
 * 주입 컴포넌트를 쓰는 쪽(뷰모델 등)이 직접 조회하고 늦은 도착까지 스스로 감당한다 — 본 클래스는 그 컴포넌트들을 중개하지 않는다.
 *
 * 화면도 중개하지 않는다 — HUD·사망 화면·대화 창은 UWxUIManagerSubsystem 이 이 컨트롤러의 빙의와 폰 상태 태그를 직접 따라가며 띄운다.
 */
UCLASS()
class WXGAME_API AWxPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AWxPlayerController(const FObjectInitializer& ObjectInitializer);

	//~ Begin AActor
	virtual void PreInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor
};

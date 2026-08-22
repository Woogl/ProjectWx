// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WxPlayerController.generated.h"

/**
 * 게임플레이 입력(이동/시선/어빌리티)은 AWxPlayerCharacter가, 메뉴 토글 입력은 UWxHUDLayout(CommonUI 액션)이 소유한다.
 * Experience 가 요청 등록한 컨트롤러 컴포넌트(인벤토리·상호작용 스캐너·대화 세션·PlayerSpawn 등)를 ModularGameplay 컴포넌트 receiver를 통해서 주입받는다.
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

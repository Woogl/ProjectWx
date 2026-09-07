// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WxPlayerController.generated.h"

class UWxDialogueSessionComponent;
class UWxHUDComponent;
class UWxInteractionScannerComponent;
class UWxInventoryComponent;

/**
 * 게임플레이 입력(이동/시선/어빌리티)은 AWxPlayerCharacter가, 메뉴 토글 입력은 UWxHUDLayout(CommonUI 액션)이 소유한다.
 * 인벤토리·상호작용 스캐너·대화 세션·HUD 컴포넌트를 기본 서브오브젝트로 소유한다 — 폰 리스폰에도 살아남아야 하는 플레이어 단위 상태다.
 * 프론트엔드 컨트롤러에도 같은 컴포넌트가 붙지만 인벤토리는 비어 있고 스캐너·대화는 자기 가드로 무동작이다. 띄울 HUD 는 BP 마다 HUD 컴포넌트에서 지정한다.
 */
UCLASS()
class WXGAME_API AWxPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AWxPlayerController(const FObjectInitializer& ObjectInitializer);

private:
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionScannerComponent> InteractionScannerComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxDialogueSessionComponent> DialogueSessionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxHUDComponent> HUDComponent;
};

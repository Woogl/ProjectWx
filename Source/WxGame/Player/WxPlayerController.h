// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WxPlayerController.generated.h"

class UWxDialogueSessionComponent;
class UWxPlayerLayoutComponent;
class UWxInteractionScannerComponent;
class UWxInventoryComponent;
class UWxNameplateManagerComponent;
class UWxViewModel_Character;
class UWxViewModel_Inventory;

/**
 * 게임플레이 입력(이동/시선/어빌리티)은 AWxPlayerCharacter가, 메뉴 토글 입력은 UWxHUDLayout(CommonUI 액션)이 소유한다.
 * 컴포넌트들은 폰 리스폰에도 살아남아야 하는 플레이어 단위 상태라 컨트롤러가 소유한다.
 * 프론트엔드 컨트롤러에도 같은 컴포넌트가 붙지만 인벤토리는 비어 있고 스캐너·대화는 자기 가드로 무동작이다.
 * 전투 HUD는 레이아웃 컴포넌트에서 지정하고, 프론트엔드 메뉴는 컨트롤러 BP에서 Push한다.
 * 로컬 컨트롤러는 플레이어 공유 뷰모델을 만들어 엔진 Global Collection 에 등록하고 값을 넣는다.
 */
UCLASS()
class WXGAME_API AWxPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AWxPlayerController(const FObjectInitializer& ObjectInitializer);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetPawn(APawn* InPawn) override;

protected:
	virtual void BeginPlay() override;

private:
	void RefreshPlayerCharacterViewModel();

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionScannerComponent> InteractionScannerComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxDialogueSessionComponent> DialogueSessionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxPlayerLayoutComponent> PlayerLayoutComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxNameplateManagerComponent> NameplateManagerComponent;

	/** 로컬 컨트롤러만 만든다. Global Collection 이 강하게 들고, 여기서는 값을 넣기 위해 참조한다. */
	UPROPERTY(Transient)
	TObjectPtr<UWxViewModel_Character> PlayerCharacterViewModel;

	/** PlayerCharacterViewModel 과 같은 방식으로 만들고 등록한다. */
	UPROPERTY(Transient)
	TObjectPtr<UWxViewModel_Inventory> InventoryViewModel;
};

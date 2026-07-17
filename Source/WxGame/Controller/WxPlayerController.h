// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WxPlayerController.generated.h"

class AWxCharacterBase;
class AWxPlayerCharacter;
class UWxActivatableWidget;
class UWxInventoryManagerComponent;

/**
 * 플레이어 컨트롤러.
 * 게임플레이 입력(이동/시선/어빌리티)은 AWxPlayerCharacter가, 메뉴 토글 입력은 UWxHUDLayout(CommonUI 액션)이 소유한다.
 *
 * 인벤토리(UWxInventoryManagerComponent)를 소유 클라이언트 단위로 관리한다.
 * Pawn 라이프사이클(사망/리스폰)과 디커플링되며, 다른 클라이언트에는 복제되지 않아 네트워크 효율적이다.
 *
 * ModularGameplay 컴포넌트 receiver 다 — GameMode 가 요청 등록한 컨트롤러 컴포넌트(PlayerSpawn 등)를 자동 주입받으며, 어떤 컴포넌트가 붙는지 알지 않는다.
 */
UCLASS()
class WXGAME_API AWxPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AWxPlayerController(const FObjectInitializer& ObjectInitializer);

	UWxInventoryManagerComponent* GetInventoryManager() const;
	
	//~ Begin AActor
	virtual void PreInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor

	virtual void OnRep_Pawn() override;
protected:
	virtual void OnPossess(APawn* InPawn) override;

	/** 소유 클라이언트의 인벤토리. 서버 권한, 소유 연결로만 복제된다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Inventory")
	TObjectPtr<UWxInventoryManagerComponent> InventoryManager;

	/** 플레이어 캐릭터에 빙의하면 Game 레이어에 띄울 HUD. 미지정이면 동작 없음. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|UI")
	TSubclassOf<UWxActivatableWidget> GameHUDWidgetClass;

	/** 빙의된 캐릭터 사망 시 Menu 레이어에 띄울 위젯. 미지정이면 동작 없음. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|UI")
	TSoftClassPtr<UWxActivatableWidget> DeathScreenWidgetClass;

private:
	void PushGameHUD();

	void BindCharacterDeath(APawn* InPawn);

	UFUNCTION()
	void HandleCharacterDeath(AWxCharacterBase* DeadCharacter);
};

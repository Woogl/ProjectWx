// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "WxPlayerController.generated.h"

class AWxPlayerCharacter;
class UAbilitySystemComponent;
class UWxActivatableWidget;
class UWxControllerInputConfig;

/**
 * 플레이어 컨트롤러.
 * 플레이어 단위 입력(UI 토글)을 소유. 입력 구성은 UWxControllerInputConfig DA에서 주입.
 * 게임플레이 입력(이동/시선/어빌리티)은 AWxPlayerCharacter가 소유.
 */
UCLASS()
class WXGAME_API AWxPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void OnRep_Pawn() override;
	virtual void OnRep_PlayerState() override;
	virtual void ReceivedPlayer() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 컨트롤러 입력 설정 (IMC + 메뉴 바인딩) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UWxControllerInputConfig> InputConfig;

private:
	void PushGameHUD(AWxPlayerCharacter* PlayerCharacter);

	void InitializePlayerAbilitySystemViewModel(UAbilitySystemComponent* ASC);
	void DeinitializePlayerAbilitySystemViewModel();

	void InitializeInventoryViewModel();
	void DeinitializeInventoryViewModel();

	void HandleMenuInputTriggered(FGameplayTag LayerTag, TSoftClassPtr<UWxActivatableWidget> WidgetClass);

	UPROPERTY(Transient)
	TObjectPtr<UWxActivatableWidget> GameHUD;
};

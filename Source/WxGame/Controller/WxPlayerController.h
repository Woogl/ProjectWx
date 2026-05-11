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
class UWxInventoryManagerComponent;

/**
 * 플레이어 컨트롤러.
 * 플레이어 단위 입력(UI 토글)을 소유. 입력 구성은 UWxControllerInputConfig DA에서 주입.
 * 게임플레이 입력(이동/시선/어빌리티)은 AWxPlayerCharacter가 소유.
 *
 * 인벤토리(UWxInventoryManagerComponent)를 소유 클라이언트 단위로 관리한다.
 * Pawn 라이프사이클(사망/리스폰)과 디커플링되며, 다른 클라이언트에는 복제되지 않아 네트워크 효율적이다.
 */
UCLASS()
class WXGAME_API AWxPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AWxPlayerController(const FObjectInitializer& ObjectInitializer);

	UWxInventoryManagerComponent* GetInventoryManager() const;

	/** WxCheckpointSubsystem 에 캐시된 마지막 체크포인트 위치로 부활. 권위 측에서만 동작. */
	void Respawn();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void OnRep_Pawn() override;
	virtual void ReceivedPlayer() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 컨트롤러 입력 설정 (IMC + 메뉴 바인딩) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UWxControllerInputConfig> InputConfig;

	/** 소유 클라이언트의 인벤토리. 서버 권한, 소유 연결로만 복제된다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Inventory")
	TObjectPtr<UWxInventoryManagerComponent> InventoryManager;

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

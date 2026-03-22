// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "WxPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UAbilitySystemComponent;
class UWxActivatableWidget;

/** Enhanced Input Action과 Gameplay Tag를 매핑하는 단일 항목 */
USTRUCT(BlueprintType)
struct FWxInputAbilityBinding
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "Input"))
	FGameplayTag InputTag;
};

/**
 * 플레이어 컨트롤러.
 * 입력 관련 데이터(MappingContext, InputAction, InputConfig)를 소유.
 * 현재 빙의 중인 캐릭터의 Health ViewModel을 Global Collection에 등록/관리.
 */
UCLASS()
class WXGAME_API AWxPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UInputMappingContext* GetDefaultMappingContext() const;
	const UInputAction* GetMoveAction() const;
	const UInputAction* GetLookAction() const;
	const TArray<FWxInputAbilityBinding>& GetAbilityInputBindings() const;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_Pawn() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputAction> LookAction;

	/** 어빌리티 입력 바인딩 설정. InputAction → InputTag 매핑 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TArray<FWxInputAbilityBinding> AbilityInputBindings;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|UI")
	TSubclassOf<UWxActivatableWidget> GameHUDClass;

private:
	void InitializePlayerHealthViewModel(UAbilitySystemComponent* ASC);
	void InitializePlayerManaViewModel(UAbilitySystemComponent* ASC);
	void InitializePlayerAbilityViewModels(UAbilitySystemComponent* ASC);
};

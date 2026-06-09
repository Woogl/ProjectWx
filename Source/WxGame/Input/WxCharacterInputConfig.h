// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "WxCharacterInputConfig.generated.h"

class UInputMappingContext;
class UInputAction;

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
 * 플레이어 캐릭터용 입력 설정.
 * 이동/시선/어빌리티 입력 등 게임플레이 레벨 입력 포함.
 */
UCLASS()
class WXGAME_API UWxCharacterInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 캐릭터 IMC. Possess 시 Subsystem에 추가됨 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputAction> LookAction;

	/** 점프 입력. Started → Jump(), Completed → StopJumping() */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputAction> JumpAction;

	/** 어빌리티 입력 바인딩 설정. InputAction → InputTag 매핑 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TArray<FWxInputAbilityBinding> AbilityInputBindings;
};

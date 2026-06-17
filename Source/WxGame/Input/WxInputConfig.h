// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "WxInputConfig.generated.h"

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
 * 플레이어 입력 설정.
 * 이동/시선/점프 등 직접 바인딩 입력과 어빌리티 입력 매핑을 포함한다.
 * 메뉴/UI 입력은 CommonUI 액션(WxHUDLayout)으로 처리하므로 여기 포함하지 않는다.
 */
UCLASS()
class WXGAME_API UWxInputConfig : public UDataAsset
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputAction> JumpAction;

	/** 어빌리티 입력 바인딩 설정. InputAction → InputTag 매핑 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TArray<FWxInputAbilityBinding> AbilityInputBindings;
};

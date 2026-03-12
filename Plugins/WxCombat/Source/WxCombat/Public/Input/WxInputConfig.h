// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "WxInputConfig.generated.h"

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
 * 어빌리티 입력 설정 데이터 에셋.
 *
 * Enhanced Input Action → Gameplay Tag 매핑 목록을 정의.
 * 플레이어 캐릭터에서 이 에셋을 참조하여 입력 시 ASC에 태그 기반 어빌리티 활성화 요청.
 */
UCLASS(BlueprintType, Const)
class WXCOMBAT_API UWxInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FWxInputAbilityBinding> AbilityInputBindings;

	/** 태그에 해당하는 InputAction 반환. 없으면 nullptr */
	const UInputAction* FindInputActionForTag(const FGameplayTag& InputTag) const;

	/** InputAction에 해당하는 태그 반환. 없으면 빈 태그 */
	FGameplayTag FindTagForInputAction(const UInputAction* InputAction) const;
};

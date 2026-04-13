// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Sprint.generated.h"

/**
 * 
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Sprint : public UGameplayEffect
{
	GENERATED_BODY()
	
public:
	UWxEffect_Sprint();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	float SprintSpeedBonus = 0.5f;
};

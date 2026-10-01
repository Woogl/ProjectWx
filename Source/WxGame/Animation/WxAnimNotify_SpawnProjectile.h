// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/WxAnimNotify_AbilityEvent.h"
#include "WxAnimNotify_SpawnProjectile.generated.h"

class AWxProjectileBase;

/**
 * 생성과 권위 판정은 AWxProjectileBase::SpawnProjectile이 맡는다.
 */
UCLASS()
class WXGAME_API UWxAnimNotify_SpawnProjectile : public UWxAnimNotify_AbilityEvent
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual FLinearColor GetEditorColor() override;
#endif

	virtual FString GetNotifyName_Implementation() const override;

	UPROPERTY(EditAnywhere, Category = "Wx")
	TSubclassOf<AWxProjectileBase> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Wx")
	FName SpawnSocketName = TEXT("hand_r");
};

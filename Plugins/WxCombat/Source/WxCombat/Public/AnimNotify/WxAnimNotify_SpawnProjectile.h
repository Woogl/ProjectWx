// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_SpawnProjectile.generated.h"

class AWxProjectileBase;

/**
 * 생성과 권위 판정은 ProjectileSubsystem이 맡는다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_SpawnProjectile : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Wx")
	TSubclassOf<AWxProjectileBase> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Wx")
	FName SpawnSocketName = TEXT("hand_r");
};

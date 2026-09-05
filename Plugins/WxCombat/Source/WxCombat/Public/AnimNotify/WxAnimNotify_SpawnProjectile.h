// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_SpawnProjectile.generated.h"

class AWxProjectileBase;

/**
 * 소켓 위치와 투사체 클래스를 풀어 월드의 ProjectileSubsystem에 생성을 맡긴다. 권위 판정은 서브시스템이 한다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_SpawnProjectile : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Wx|Projectile")
	TSubclassOf<AWxProjectileBase> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Wx|Projectile")
	FName SpawnSocketName = TEXT("hand_r");
};

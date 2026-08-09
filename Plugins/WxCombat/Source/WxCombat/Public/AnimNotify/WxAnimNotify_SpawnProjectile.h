// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_SpawnProjectile.generated.h"

class AWxProjectileBase;

/**
 * 투사체 스폰 AnimNotify.
 * 투사체 클래스·소켓은 여기서 저작하고, 실제 스폰(서버 권위)은 재생 중인 어빌리티에, 대미지는 투사체 클래스에 맡긴다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_SpawnProjectile : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	/** 스폰할 투사체 클래스 */
	UPROPERTY(EditAnywhere, Category = "Wx|Projectile")
	TSubclassOf<AWxProjectileBase> ProjectileClass;

	/** 투사체 스폰 위치 소켓 이름 */
	UPROPERTY(EditAnywhere, Category = "Wx|Projectile")
	FName SpawnSocketName = TEXT("hand_r");
};

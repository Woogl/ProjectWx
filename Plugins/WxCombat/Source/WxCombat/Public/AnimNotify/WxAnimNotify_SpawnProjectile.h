// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_SpawnProjectile.generated.h"

class AWxProjectileBase;

/**
 * 투사체 클래스·스폰 소켓과 함께 GameplayEvent를 발행한다.
 * 자신을 이벤트 페이로드로 전달할 뿐, 투사체 생성은 ProjectileComponent에 맡긴다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_SpawnProjectile : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

	TSubclassOf<AWxProjectileBase> GetProjectileClass() const;
	FName GetSpawnSocketName() const;

protected:
	UPROPERTY(EditAnywhere, Category = "Wx|Projectile")
	TSubclassOf<AWxProjectileBase> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Wx|Projectile")
	FName SpawnSocketName = TEXT("hand_r");
};

// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "WxProjectileBase.generated.h"

class UArrowComponent;
class UGameplayEffect;
class USphereComponent;
class USkeletalMeshComponent;
class UProjectileMovementComponent;

/**
 * 투사체 베이스 클래스.
 *
 * 사용 흐름:
 *  1. AnimNotify에서 SpawnActor → BeginPlay에서 Owner의 ASC로 EffectSpec 목록 생성 (발사 시점 스탯 확정)
 *  2. Pawn/WorldDynamic에 Overlap 시 캐싱된 Spec 목록을 대상 ASC에 적용 후 Destroy
 *
 * 중력 없는 직선 투사체가 기본값. BP에서 ProjectileMovement 설정으로 조정 가능.
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API AWxProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AWxProjectileBase();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Projectile")
	TObjectPtr<UArrowComponent> Arrow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Projectile")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Projectile")
	TObjectPtr<USphereComponent> HitCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	/** 피격 대상에 적용할 GameplayEffect 목록 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Projectile")
	TArray<TSubclassOf<UGameplayEffect>> EffectClasses;

	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void HandleHitCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:
	/** 공격 속성 태그. 스폰 전 외부에서 설정하면 BeginPlay에서 Spec에 반영 */
	FGameplayTagContainer AttackTags;

private:
	TArray<FGameplayEffectSpecHandle> EffectSpecHandles;
};

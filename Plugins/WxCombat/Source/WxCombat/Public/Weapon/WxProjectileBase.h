// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "WxProjectileBase.generated.h"

class USphereComponent;
class USkeletalMeshComponent;
class UProjectileMovementComponent;

/**
 * 투사체 베이스 클래스.
 *
 * 사용 흐름:
 *  1. 어빌리티에서 SpawnActor 후 SetDamageEffectSpecHandle 호출
 *  2. Pawn/WorldDynamic에 Overlap 시 SpecHandle을 대상 ASC에 적용 후 자신을 Destroy
 *
 * 중력 없는 직선 투사체가 기본값. BP에서 ProjectileMovement 설정으로 조정 가능.
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API AWxProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AWxProjectileBase();

	/** 어빌리티에서 발사 직전에 SpecHandle을 주입 */
	void SetDamageEffectSpecHandle(const FGameplayEffectSpecHandle& InHandle);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Projectile")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Projectile")
	TObjectPtr<USphereComponent> HitCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Projectile")
	float InitialSpeed = 1500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Projectile")
	float MaxSpeed = 1500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Projectile")
	float LifeSpanSeconds = 5.f;

	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void HandleHitCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	FGameplayEffectSpecHandle DamageEffectSpecHandle;
};

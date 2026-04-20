// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "WxDamageInfo.h"
#include "WxProjectileBase.generated.h"

class UArrowComponent;
class USphereComponent;
class USkeletalMeshComponent;
class UProjectileMovementComponent;
class UNiagaraComponent;
class UNiagaraSystem;

/**
 * 투사체 베이스 클래스.
 *
 * 사용 흐름:
 *  1. AnimNotify에서 SpawnActorDeferred → InitializeDamageSpec(DamageInfo) → FinishSpawning
 *  2. Pawn/WorldDynamic에 Overlap 시 캐싱된 Spec을 대상 ASC에 적용 후 Destroy
 *
 * 중력 없는 직선 투사체가 기본값. BP에서 ProjectileMovement 설정으로 조정 가능.
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API AWxProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AWxProjectileBase();

	/** DamageInfo 기반 Spec 배열을 생성해 저장한다. SpawnActorDeferred 직후 FinishSpawning 이전에 호출한다. */
	void InitializeDamageSpec(const FWxDamageInfo& InDamageInfo);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Projectile")
	TObjectPtr<UArrowComponent> Arrow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Projectile")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Projectile")
	TObjectPtr<USphereComponent> HitCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Projectile|FX")
	TObjectPtr<UNiagaraComponent> TrailFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Projectile|FX")
	TObjectPtr<UNiagaraSystem> ImpactFX;

	virtual void BeginPlay() override;
	virtual void Destroyed() override;

	UFUNCTION()
	virtual void HandleHitCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	FGameplayEffectContextHandle CachedEffectContext;
	TArray<FGameplayEffectSpecHandle> CachedSpecHandles;
};

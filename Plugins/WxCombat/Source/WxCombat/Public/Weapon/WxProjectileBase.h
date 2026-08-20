// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "WxProjectileBase.generated.h"

class UArrowComponent;
class USphereComponent;
class USkeletalMeshComponent;
class UProjectileMovementComponent;
class UNiagaraComponent;
class UNiagaraSystem;

/**
 * Pawn에 Overlap하거나 월드에 Block하면 이펙트를 재생하고 사라진다.
 *
 * 스폰과 파괴 모두 서버 권위이며, 대미지도 그 서버에서만 적용한다 — 클라에는 예측 키가 없어 어차피 GAS가 적용을 막는다.
 * 반면 ImpactFX는 권위 검사 앞에서 재생하므로, 충돌을 감지한 머신이 각자 즉시 재생한다.
 *
 * 대미지는 이 투사체 클래스(BP 서브클래스)가 DamageDataRow로 직접 저작한다.
 * HitCollision은 루트 컴포넌트이며 "WxProjectile" 콜리전 프로파일(DefaultEngine.ini)을 사용한다.
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API AWxProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AWxProjectileBase();

protected:
	UPROPERTY(EditAnywhere, Category = "Wx|Projectile|Damage", meta = (RowType = "/Script/WxCombat.WxDamageTableRow"))
	FDataTableRowHandle DamageDataRow;

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

	UFUNCTION()
	virtual void HandleHitCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	virtual void HandleHitCollisionHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
	void PlayImpactFX();
};

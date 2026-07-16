// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
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
 *  1. 어빌리티가 SpawnActor로 스폰하면, 투사체가 자신의 대미지 데이터로 BeginPlay에서 Spec을 생성한다.
 *  2. Pawn(캐릭터 메시)에 Overlap 시 캐싱된 Spec을 대상 ASC에 적용 후 Destroy
 *  3. WorldStatic/WorldDynamic에 Block 시 Destroy (Destroyed에서 ImpactFX 스폰)
 *
 * 대미지는 이 투사체 클래스(BP 서브클래스)가 DamageDataRow로 직접 저작한다.
 * HitCollision은 루트 컴포넌트이며 "WxProjectile" 콜리전 프로파일(DefaultEngine.ini)을 사용한다.
 * 중력 없는 직선 투사체가 기본값. BP에서 ProjectileMovement 설정으로 조정 가능.
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API AWxProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AWxProjectileBase();

	virtual void Destroyed() override;

protected:
	/** 이 투사체의 대미지 수치 테이블 Row */
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
	/** DamageDataRow 기반 Spec 배열을 생성해 저장한다. BeginPlay에서 호출한다. */
	void InitializeDamageSpec();

	FGameplayEffectContextHandle CachedEffectContext;
	TArray<FGameplayEffectSpecHandle> CachedSpecHandles;
};

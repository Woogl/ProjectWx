// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "GenericTeamAgentInterface.h"
#include "WxProjectileBase.generated.h"

class UArrowComponent;
class USphereComponent;
class USkeletalMeshComponent;
class UProjectileMovementComponent;
class UNiagaraComponent;
class UNiagaraSystem;

/**
 * Pawn에 Overlap하거나 월드에 Block하면 이펙트를 재생하고 사라진다.
 * 다만 판정이 성립하지 않는 Pawn은 충돌로 치지 않아, 이펙트도 파괴도 없이 그대로 통과한다 — 아군·중립이거나 회피 무적으로 흘려낸 경우다.
 * 되돌릴 수 있는 투사체를 퍼펙트 가드로 막은 히트만은 파괴 대신 쏜 쪽으로 돌아선다.
 *
 * 스폰과 파괴 모두 서버 권위이며, 대미지와 적중 연출도 서버 판정을 따른다.
 * ImpactFX는 충돌을 감지한 머신마다 재생하며, Overlap에서는 무적 대상(회피)이면 생략한다.
 *
 * HitCollision의 "WxProjectile" 콜리전 프로파일은 DefaultEngine.ini에 정의돼 있다.
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API AWxProjectileBase : public AActor, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AWxProjectileBase();

	int32 GetProjectileLevel() const;

	/** 퍼펙트 가드로 막힌 히트에서 쏜 쪽으로 되돌린다. bCanReflect가 false면 아무것도 하지 않는다. */
	void Reflect(APawn& Parrier);

	/** 적중 시 공격자에게 걸 역경직 지속 시간 (초). 0 이하이면 미적용 */
	UPROPERTY(EditAnywhere, Category = "Wx|Projectile|Damage")
	float InstigatorHitStop = 0.f;

	/** 적중 시 피격자에게 걸 역경직 지속 시간 (초). 0 이하이면 미적용 */
	UPROPERTY(EditAnywhere, Category = "Wx|Projectile|Damage")
	float VictimHitStop = 0.1f;

	//~ Begin IGenericTeamAgentInterface
	/** 팀을 따로 들지 않고 Instigator의 것을 그대로 쓴다 — 피격 판정도 같은 출처로 적대 여부를 가린다. */
	virtual FGenericTeamId GetGenericTeamId() const override;
	//~ End IGenericTeamAgentInterface

protected:
	UPROPERTY(EditAnywhere, Category = "Wx|Projectile|Damage", meta = (RowType = "/Script/WxCombat.WxDamageTableRow", WxPreviewRow = "true"))
	FDataTableRowHandle DamageDataRow;

	/** false이면 퍼펙트 가드로 막혀도 되돌아가지 않고 그대로 파괴된다. */
	UPROPERTY(EditAnywhere, Category = "Wx|Projectile")
	bool bCanReflect = true;

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

	/** 되돌림은 Instigator 교체로 복제되므로, 클라는 그 신호에 맞춰 로컬 예측 속도를 복제된 회전으로 다시 세운다. */
	virtual void OnRep_Instigator() override;

	UFUNCTION()
	virtual void HandleHitCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	virtual void HandleHitCollisionHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
	friend class UWxProjectileSubsystem;

	/** 발사 시 확정한다. 반사는 소유자만 바꾸고 레벨은 유지한다. */
	UPROPERTY(VisibleInstanceOnly, Category = "Wx|Projectile|Damage")
	int32 ProjectileLevel = 1;

	void PlayImpactFX();
};

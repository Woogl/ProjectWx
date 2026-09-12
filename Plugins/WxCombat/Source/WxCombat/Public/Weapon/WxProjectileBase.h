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
 * 스폰과 파괴 모두 서버 권위이며, 대미지도 그 서버에서만 적용한다 — 클라에는 예측 키가 없어 어차피 GAS가 적용을 막는다.
 * 반면 ImpactFX는 권위 검사 앞에서 재생하므로, 충돌을 감지한 머신이 각자 즉시 재생한다.
 *
 * HitCollision은 루트 컴포넌트이며 "WxProjectile" 콜리전 프로파일(DefaultEngine.ini)을 사용한다.
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API AWxProjectileBase : public AActor, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AWxProjectileBase();

	//~ Begin IGenericTeamAgentInterface
	/** 팀을 따로 들지 않고 Instigator의 것을 그대로 쓴다 — 피격 판정도 같은 출처로 적대 여부를 가린다. */
	virtual FGenericTeamId GetGenericTeamId() const override;
	//~ End IGenericTeamAgentInterface

protected:
	UPROPERTY(EditAnywhere, Category = "Wx|Projectile|Damage", meta = (RowType = "/Script/WxCombat.WxDamageTableRow", WxPreviewRow = "true"))
	FDataTableRowHandle DamageDataRow;

	/** 적중 시 공격자에게 걸 역경직 지속 시간 (초). 0 이하이면 미적용 */
	UPROPERTY(EditAnywhere, Category = "Wx|Projectile|Damage")
	float InstigatorHitStop = 0.f;

	/** 적중 시 피격자에게 걸 역경직 지속 시간 (초). 0 이하이면 미적용 */
	UPROPERTY(EditAnywhere, Category = "Wx|Projectile|Damage")
	float VictimHitStop = 0.1f;

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
	/** 되돌림이 성립한 히트에서만 부른다. 쏜 쪽은 오버랩 핸들러의 적대 판정이 보장한다. */
	void Reflect(APawn& Parrier);

	void PlayImpactFX();
};

// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxDamageInfo.h"
#include "WxWeaponBase.generated.h"

class UArrowComponent;
class UCapsuleComponent;
class USkeletalMesh;
class USkeletalMeshComponent;

/**
 * 무기 베이스 클래스.
 *
 * 사용 흐름:
 *  1. SpawnActor → Equip(TargetMesh, Socket, MeshAsset)
 *  2. ANS_WeaponAttack이 BeginAttack / EndAttack을 호출
 *  3. 무기가 내부 레퍼런스 카운팅으로 히트 콜리전 활성/비활성 자동 전환
 *
 * GripPoint(SceneComponent)가 루트이며, 캐릭터 소켓에 부착되는 기준점.
 * 메시는 BP에서 GripPoint 하위에 원하는 타입(Static/Skeletal)으로 추가.
 * 히트 판정은 HitCollision(CapsuleComponent) Overlap 기반.
 * 한 스윙에서 동일 액터는 최대 1회만 피격 (HitActorsThisSwing으로 관리).
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API AWxWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWxWeaponBase();

	/** 소유자 액터에 부착된 AWxWeaponBase를 반환. 없으면 nullptr */
	static AWxWeaponBase* FindWeapon(const AActor* Owner);

	/**
	 * 공격 구간을 시작한다.
	 * 히트 목록을 초기화하고 DamageInfo를 설정한 뒤, 첫 호출 시 콜리전을 활성화한다.
	 * 겹치는 ANS가 있으면 레퍼런스 카운트만 증가하고 콜리전은 유지된다.
	 */
	void BeginAttack(const FWxDamageInfo& InDamageInfo);

	/**
	 * 공격 구간을 종료한다.
	 * 모든 활성 구간이 종료되면 콜리전을 비활성화하고 공격 상태를 초기화한다.
	 */
	void EndAttack();

	/**
	 * 무기를 대상 메시의 소켓에 장착한다.
	 * MeshAsset이 지정되면 무기 외형을 해당 메시로 교체, nullptr이면 CDO 기본 메시로 복원한다.
	 * TargetMesh의 Owner가 무기의 Owner로 설정된다.
	 */
	void Equip(USkeletalMeshComponent* TargetMesh, FName SocketName, USkeletalMesh* MeshAsset = nullptr);

	/** 장착 해제. 활성 공격 구간이 있으면 강제 종료 후 메시에서 분리한다. */
	void Unequip();

	/** 역경직 지속 시간 (초). 0 이하이면 역경직 미적용 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Weapon")
	float HitStopDuration = 0.15f;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 손잡이 위치. 캐릭터 소켓에 부착되는 기준점 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Weapon")
	TObjectPtr<USceneComponent> GripPoint;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Wx|Weapon")
	TObjectPtr<UArrowComponent> GripArrow;
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Weapon")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Weapon")
	TObjectPtr<UCapsuleComponent> HitCollision;

	UFUNCTION()
	virtual void HandleHitCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	/** 히트 검증/팀 체크/GE 적용/HitStop을 수행. Overlap 이벤트와 Tick Sweep이 공통으로 호출 */
	void ProcessHit(AActor* OtherActor, const FHitResult& HitResult);

	/** 현재 활성 공격 구간 수. 0이면 콜리전 비활성 상태 */
	int32 ActiveAttackCount = 0;

	/** 현재 활성 공격의 DamageInfo. 히트 시 Damage Spec 생성에 사용 */
	FWxDamageInfo DamageInfo;

	/** 한 스윙 내 이미 피격된 액터 목록 */
	TSet<TObjectPtr<AActor>> HitActorsThisSwing;

	/** 직전 프레임 HitCollision 위치/회전. Tick에서 현재 위치까지 Sweep할 때 시작점으로 사용 */
	FVector PrevCapsuleLocation = FVector::ZeroVector;
	FQuat PrevCapsuleRotation = FQuat::Identity;
};

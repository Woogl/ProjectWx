// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxDamageInfo.h"
#include "WxWeaponBase.generated.h"

class ACharacter;
class UCapsuleComponent;
class USkeletalMesh;
class USkeletalMeshComponent;

/**
 * 무기 베이스 클래스.
 * ANS_WeaponAttack이 BeginAttack/EndAttack을 호출하면, 무기가 내부 레퍼런스 카운팅으로 히트 콜리전을 켜고 끈다.
 *
 * 루트인 GripPoint가 캐릭터 소켓에 부착되는 기준점이고, 메시는 BP에서 그 하위에 원하는 타입으로 추가한다.
 * 히트 판정은 HitCollision Overlap 기반이며, 한 스윙에서 같은 액터는 최대 1회만 피격된다.
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API AWxWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWxWeaponBase();

	/** 소유자 액터에 부착된 AWxWeaponBase를 반환. 없으면 nullptr */
	static AWxWeaponBase* FindWeapon(const AActor* Owner);

	/** 첫 호출에서만 콜리전을 켜고, 겹치는 ANS는 레퍼런스 카운트만 올린다. */
	void BeginAttack(const FWxDamageInfo& InDamageInfo);

	/** 모든 활성 구간이 닫혔을 때만 콜리전을 끈다. */
	void EndAttack();

	/** 사망처럼 ANS의 구간 종료를 기다릴 수 없는 상황에서 판정을 즉시 걷어낸다. */
	void CancelAttack();

	/** 무기 외형 메시를 교체한다. MeshAsset이 nullptr이면 무시한다. */
	void SetVisualMesh(USkeletalMesh* MeshAsset);

	/** 대상 캐릭터의 GetMesh() 소켓에 부착하고 Owner도 그 캐릭터로 설정한다. */
	void AttachToCharacter(ACharacter* OwnerCharacter, FName SocketName);

	/** 활성 공격 구간이 남아 있으면 강제 종료한 뒤 분리한다 */
	void DetachFromCharacter();

	/** C++ 서브오브젝트라 항상 존재한다. BP가 따로 추가한 메시는 포함하지 않는다. */
	USkeletalMeshComponent* GetMesh() const;

	/** 역경직 지속 시간 (초). 0 이하이면 역경직 미적용 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Weapon")
	float HitStopDuration = 0.15f;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 손잡이 위치. 캐릭터 소켓에 부착되는 기준점 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Weapon")
	TObjectPtr<USceneComponent> GripPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Weapon")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Weapon")
	TObjectPtr<UCapsuleComponent> HitCollision;

	UFUNCTION()
	virtual void HandleHitCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	/**
	 * 소유자와 한 스윙 내 중복 히트만 걸러 GE 적용과 HitStop을 수행. Overlap 이벤트와 Tick Sweep이 공통으로 호출.
	 * 팀 판정은 하지 않는다 — 맞은 대상은 아군·중립 여부와 무관하게 피해를 받는다.
	 */
	void ProcessHit(AActor* OtherActor, const FHitResult& HitResult);

	/** 현재 활성 공격 구간 수. 0이면 콜리전 비활성 상태 */
	int32 ActiveAttackCount = 0;

	/** 현재 활성 공격의 DamageInfo. 히트 시 Damage Spec 생성에 사용 */
	UPROPERTY()
	FWxDamageInfo DamageInfo;

	/** 한 스윙 내 이미 피격된 액터 목록 */
	UPROPERTY()
	TSet<TObjectPtr<AActor>> HitActorsThisSwing;

	/** 직전 프레임 HitCollision 위치. Tick에서 현재 위치까지 Sweep할 때 시작점으로 사용 */
	FVector PrevCapsuleLocation = FVector::ZeroVector;
};

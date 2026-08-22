// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxWeaponBase.generated.h"

class ACharacter;
class UCapsuleComponent;
class USkeletalMesh;
class USkeletalMeshComponent;

/**
 * WxAnimNotifyState_WeaponAttack이 BeginAttack/EndAttack을 호출하면, 무기가 내부 레퍼런스 카운팅으로 히트 콜리전을 켜고 끈다.
 *
 * 루트인 GripPoint가 캐릭터 소켓에 부착되는 기준점이고, 외형은 SetVisualMesh로 Mesh 서브오브젝트에 얹는다.
 * 히트 판정은 HitCollision Overlap과 매 틱 캡슐 Sweep을 함께 쓰며, 한 스윙에서 같은 액터는 최대 1회만 피격된다.
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API AWxWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWxWeaponBase();

	static AWxWeaponBase* FindWeapon(const AActor* Owner);

	/** 첫 호출에서만 콜리전을 켜고, 겹치는 ANS는 레퍼런스 카운트만 올린다. */
	void BeginAttack(const FDataTableRowHandle& InDamageInfo);

	/** 모든 활성 구간이 닫혔을 때만 콜리전을 끈다. */
	void EndAttack();

	/** 사망처럼 ANS의 구간 종료를 기다릴 수 없는 상황에서 판정을 즉시 걷어낸다. */
	void CancelAttack();

	/** MeshAsset이 nullptr이면 무시한다. */
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
	 * Overlap 이벤트와 Tick Sweep이 공통으로 호출한다.
	 * 팀 판정은 대미지 ExecCalc가 하므로 아군에게도 GE는 적용된다.
	 */
	void ProcessHit(AActor* OtherActor, const FHitResult& HitResult);

	/** 0이면 콜리전 비활성 상태 */
	int32 ActiveAttackCount = 0;

	UPROPERTY()
	FDataTableRowHandle DamageInfo;

	UPROPERTY()
	TSet<TObjectPtr<AActor>> HitActorsThisSwing;

	FVector PrevCapsuleLocation = FVector::ZeroVector;
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "WxWeaponBase.generated.h"

class UAnimInstance;
class UAnimMontage;
class UArrowComponent;
class UCapsuleComponent;
class USkeletalMeshComponent;
class UGameplayEffect;

/**
 * 무기 베이스 클래스.
 *
 * 사용 흐름:
 *  1. SpawnActor → AttachToCharacter(Character, SocketName)
 *  2. 공격 어빌리티에서 SetCollisionEnabled(true) 호출 → 히트 감지 시작
 *  3. 스윙 종료 시 SetCollisionEnabled(false) 호출
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

	/** 캐릭터 메시의 SocketName에 부착 */
	void AttachToCharacter(ACharacter* Character, FName SocketName);

	void DetachFromCharacter();

	/**
	 * 히트 콜리전 활성/비활성.
	 * bEnabled = true 시 HitActorsThisSwing을 초기화하여 새 스윙 시작.
	 */
	void SetWeaponCollisionEnabled(bool bEnabled);

	/** 피격 대상에게 적용할 GameplayEffect (보통 Instant Damage) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Weapon")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Weapon")
	FGameplayTag WeaponTag;

protected:
	/** 손잡이 위치. 캐릭터 소켓에 부착되는 기준점 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Weapon")
	TObjectPtr<USceneComponent> GripPoint;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Wx|Weapon")
	TObjectPtr<UArrowComponent> GripArrow;
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Weapon")
	TObjectPtr<UCapsuleComponent> HitCollision;

	UFUNCTION()
	virtual void HandleHitCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	void BindMontageEndedCallback(bool bBind);

	UFUNCTION()
	void HandleOwnerMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** 한 스윙 내 이미 피격된 액터 목록 */
	TSet<TObjectPtr<AActor>> HitActorsThisSwing;
};

// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxWeaponBase.generated.h"

class UArrowComponent;
class UCapsuleComponent;
class USkeletalMeshComponent;
class UGameplayEffect;
struct FGameplayTag;

/**
 * 무기 베이스 클래스.
 *
 * 사용 흐름:
 *  1. SpawnActor → AttachToCharacter(Character, SocketName)
 *  2. ANS_WeaponCollision이 캐릭터 ASC에 ANS.WeaponCollision 태그를 부여
 *  3. 무기가 태그 변화를 감지하여 히트 콜리전 활성/비활성 자동 전환
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

	/** 캐릭터 메시의 SocketName에 부착하고 태그 감지 등록 */
	void AttachToCharacter(ACharacter* Character, FName SocketName);

	void DetachFromCharacter();

	/** 피격 대상에게 적용할 GameplayEffect (보통 Instant Damage) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Weapon")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

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
	void SetWeaponCollisionEnabled(bool bEnabled);

	/** 소유 캐릭터의 ANS.WeaponCollision 태그 변화 콜백 */
	void HandleWeaponCollisionTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	/** 한 스윙 내 이미 피격된 액터 목록 */
	TSet<TObjectPtr<AActor>> HitActorsThisSwing;
};

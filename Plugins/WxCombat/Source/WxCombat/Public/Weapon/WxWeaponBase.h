// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxWeaponBase.generated.h"

class ACharacter;
class UShapeComponent;
class USkeletalMesh;
class USkeletalMeshComponent;

/**
 * 히트박스는 BP가 무기에 부착한 ShapeComponent(박스·캡슐·구) 전부이며, 콜리전 구성은 코드가 강제하므로 BP는 형상과 트랜스폼만 저작한다.
 * 히트 판정은 Overlap과 매 틱 형상별 Sweep을 함께 쓰며, 한 스윙에서 같은 액터는 최대 1회만 피격된다.
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API AWxWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWxWeaponBase();

	static AWxWeaponBase* FindWeapon(const AActor* Owner);

	void BeginAttack(const FDataTableRowHandle& InDamageInfo);
	void EndAttack();
	void CancelAttack();

	void SetVisualMesh(USkeletalMesh* MeshAsset);

	void AttachToCharacter(ACharacter* OwnerCharacter, FName SocketName);
	void DetachFromCharacter();

	USkeletalMeshComponent* GetMesh() const;

	/** 역경직 지속 시간 (초). 0 이하이면 역경직 미적용 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Weapon")
	float HitStopDuration = 0.15f;

	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Weapon")
	TObjectPtr<USceneComponent> GripPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Weapon")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UFUNCTION()
	virtual void HandleHitShapeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	void ProcessHit(AActor* OtherActor, const FHitResult& HitResult);

	UPROPERTY()
	TArray<TObjectPtr<UShapeComponent>> HitShapes;

	/** 0이면 콜리전 비활성 상태 */
	int32 ActiveAttackCount = 0;

	UPROPERTY()
	FDataTableRowHandle DamageInfo;

	UPROPERTY()
	TSet<TObjectPtr<AActor>> HitActorsThisSwing;

	/** HitShapes와 평행한 직전 프레임 위치. 틱 Sweep의 시작점 */
	TArray<FVector> PrevShapeLocations;
};

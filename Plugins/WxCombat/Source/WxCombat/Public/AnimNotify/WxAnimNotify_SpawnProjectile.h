// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxDamageInfo.h"
#include "WxAnimNotify_SpawnProjectile.generated.h"

class AWxProjectileBase;

/**
 * 투사체 스폰 AnimNotify.
 *
 * 원거리 공격 몽타주에 배치하면 해당 프레임에서
 * SpawnSocketName 소켓 위치에 투사체를 스폰한다.
 * 서버에서만 스폰한다.
 */
UCLASS(DisplayName = "Wx Spawn Projectile")
class WXCOMBAT_API UWxAnimNotify_SpawnProjectile : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	/** 스폰할 투사체 클래스 */
	UPROPERTY(EditAnywhere, Category = "Wx|Projectile")
	TSubclassOf<AWxProjectileBase> ProjectileClass;

	/** 투사체 스폰 위치 소켓 이름 */
	UPROPERTY(EditAnywhere, Category = "Wx|Projectile")
	FName SpawnSocketName = TEXT("hand_r");

	/** 설정 시 DamageInfo를 테이블에서 읽어 사용한다. 미설정 시 아래 DamageInfo를 직접 사용 */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (RowType = "/Script/WxCombat.WxDamageTableRow"))
	FDataTableRowHandle DamageDataRow;

	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ShowOnlyInnerProperties, EditCondition = "DamageDataRow.DataTable == nullptr"))
	FWxDamageInfo DamageInfo;

private:
	FWxDamageInfo ResolveDamageInfo() const;
};

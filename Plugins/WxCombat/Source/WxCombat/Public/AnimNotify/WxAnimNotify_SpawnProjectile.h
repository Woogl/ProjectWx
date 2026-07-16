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
 * 원거리 공격 몽타주에 배치하면 해당 프레임에서 재생 중인 어빌리티에 스폰을 위임한다(UWxAbilityBase::SpawnProjectile).
 * 스폰할 투사체 클래스·소켓·데미지는 이 노티파이에서 저작하고, 실제 스폰 실행(서버 권위)은 어빌리티가 담당한다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_SpawnProjectile : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif

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

	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ShowOnlyInnerProperties))
	FWxDamageInfo DamageInfo;

private:
	FWxDamageInfo ResolveDamageInfo() const;
};

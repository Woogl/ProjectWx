// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxDamageInfo.h"
#include "WxAnimNotify_AreaAttack.generated.h"

class UTargetingPreset;

/**
 * 범위 공격 AnimNotify.
 *
 * 인스턴트 1회성 AoE 공격용.
 * 해당 프레임에 TargetingPreset 쿼리를 1회 실행해 잡힌 액터들의 ASC에 DamageInfo로부터 만든 Damage Spec을 적용한다.
 * 서버에서만 처리한다.
 *
 * SourceSocketName이 NAME_None이 아니면 해당 소켓 위치를 쿼리의 SourceLocation으로 사용한다.
 * NAME_None이면 Preset이 SourceActor의 액터 위치를 기준으로 동작한다.
 */
// TODO: 게임 로직 이관 필요
UCLASS()
class WXCOMBAT_API UWxAnimNotify_AreaAttack : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif

protected:
	/** 타겟 후보 쿼리에 사용할 TargetingPreset. AOE Selection + Team/LineTrace Filter 조합을 권장 */
	UPROPERTY(EditAnywhere, Category = "Wx|Targeting")
	TObjectPtr<UTargetingPreset> TargetingPreset;

	/** NAME_None 이 아니면 이 소켓 위치를 쿼리의 SourceLocation 으로 사용 */
	UPROPERTY(EditAnywhere, Category = "Wx|Targeting")
	FName SourceSocketName = NAME_None;

	/** 설정 시 DamageInfo를 테이블에서 읽어 사용한다. 미설정 시 아래 DamageInfo를 직접 사용 */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (RowType = "/Script/WxCombat.WxDamageTableRow"))
	FDataTableRowHandle DamageDataRow;

	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ShowOnlyInnerProperties))
	FWxDamageInfo DamageInfo;

private:
	FWxDamageInfo ResolveDamageInfo() const;
};

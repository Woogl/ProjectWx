// Copyright Woogle. All Rights Reserved.

#include "Animation/WxAnimNotifyState_WeaponAttack.h"
#if WITH_EDITOR
#include "Animation/WxAnimNotifySettings.h"
#endif

#if WITH_EDITOR
FLinearColor UWxAnimNotifyState_WeaponAttack::GetEditorColor()
{
	return GetDefault<UWxAnimNotifySettings>()->AttackColor;
}
#endif

UWxAnimNotifyState_WeaponAttack::UWxAnimNotifyState_WeaponAttack()
{
	// 애님 평가 내부에서 동기 실행돼 저프레임·히치에도 히트 구간이 스킵되지 않고, 콤보 전환 시 Begin/End 순서가 애님 시간 기준으로 보장된다.
	bIsNativeBranchingPoint = true;
}

FString UWxAnimNotifyState_WeaponAttack::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Attack: %s"), DamageDataRow.IsNull() ? TEXT("None") : *DamageDataRow.RowName.ToString());
}

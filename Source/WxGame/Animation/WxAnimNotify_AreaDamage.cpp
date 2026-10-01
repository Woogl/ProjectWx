// Copyright Woogle. All Rights Reserved.

#include "Animation/WxAnimNotify_AreaDamage.h"
#include "Targeting/WxTargetingPreview.h"
#if WITH_EDITOR
#include "Animation/WxAnimNotifySettings.h"
#endif

#if WITH_EDITOR
FLinearColor UWxAnimNotify_AreaDamage::GetEditorColor()
{
	return GetDefault<UWxAnimNotifySettings>()->AttackColor;
}
#endif

FString UWxAnimNotify_AreaDamage::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Area: %s"), DamageDataRow.IsNull() ? TEXT("None") : *DamageDataRow.RowName.ToString());
}

#if WITH_EDITOR
void UWxAnimNotify_AreaDamage::DrawInEditor(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* MeshComp, const UAnimSequenceBase* Animation, const FAnimNotifyEvent& NotifyEvent) const
{
	const FLinearColor PreviewColor = GetDefault<UWxAnimNotifySettings>()->AttackColor;
	WxTargetingPreview::DrawDebugTargetingPreset(PDI, MeshComp, NotifyEvent, TargetingPreset, PreviewColor);
}
#endif

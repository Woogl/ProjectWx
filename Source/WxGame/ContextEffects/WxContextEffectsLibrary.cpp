// Copyright Woogle. All Rights Reserved.

#include "ContextEffects/WxContextEffectsLibrary.h"

bool UWxContextEffectsLibrary::ResolveEffect(const FGameplayTag& EffectTag, EPhysicalSurface Surface, FWxContextEffectAssets& OutAssets) const
{
	const FWxContextEffectSurfaceSet* SurfaceSet = Effects.Find(EffectTag);
	if (!SurfaceSet)
	{
		return false;
	}

	if (const FWxContextEffectAssets* Found = SurfaceSet->SurfaceAssets.Find(Surface))
	{
		OutAssets = *Found;
	}
	else
	{
		OutAssets = SurfaceSet->DefaultAssets;
	}

	// 사운드·VFX 가 모두 비어 있으면 재생할 게 없다.
	return OutAssets.Sound != nullptr || OutAssets.VFX != nullptr;
}

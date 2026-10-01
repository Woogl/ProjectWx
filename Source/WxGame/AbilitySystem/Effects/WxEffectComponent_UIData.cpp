// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effects/WxEffectComponent_UIData.h"

FText UWxEffectComponent_UIData::GetTitle() const
{
	return Title;
}

FText UWxEffectComponent_UIData::GetDescription() const
{
	return Description;
}

TSoftObjectPtr<UObject> UWxEffectComponent_UIData::GetIcon() const
{
	return Icon;
}

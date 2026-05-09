// Copyright Woogle. All Rights Reserved.

#include "Widget/WxLazyImage.h"

#include "Engine/Texture2D.h"

void UWxLazyImage::SetLazyTexture(const TSoftObjectPtr<UTexture2D>& LazyTexture)
{
	SetBrushFromLazyTexture(LazyTexture, false);
}

// Copyright Woogle. All Rights Reserved.

#include "Component/WxPromptWidgetComponent.h"

UWxPromptWidgetComponent::UWxPromptWidgetComponent()
{
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawAtDesiredSize(true);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetVisibility(false);
}

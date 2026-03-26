// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Cue/WxDamageFloaterActor.h"
#include "AbilitySystem/WxDamageFloaterInterface.h"
#include "Components/WidgetComponent.h"

AWxDamageFloaterActor::AWxDamageFloaterActor()
{
	PrimaryActorTick.bCanEverTick = false;

	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComponent->SetDrawAtDesiredSize(true);
	WidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = WidgetComponent;

	InitialLifeSpan = 5.f;
}

void AWxDamageFloaterActor::InitDamageInfo(TSubclassOf<UUserWidget> InWidgetClass, float InDamageAmount, bool bInIsCritical)
{
	if (InWidgetClass)
	{
		WidgetComponent->SetWidgetClass(InWidgetClass);
		WidgetComponent->InitWidget();
	}

	UUserWidget* Widget = WidgetComponent->GetUserWidgetObject();
	if (Widget && Widget->GetClass()->ImplementsInterface(UWxDamageFloaterInterface::StaticClass()))
	{
		IWxDamageFloaterInterface::Execute_InitDamageInfo(Widget, InDamageAmount, bInIsCritical);
	}
}

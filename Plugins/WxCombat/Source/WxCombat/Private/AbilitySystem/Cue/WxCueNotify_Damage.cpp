// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Cue/WxCueNotify_Damage.h"
#include "NiagaraFunctionLibrary.h"
#include "WxGameplayTags.h"
#include "Components/WidgetComponent.h"

UWxCueNotify_Damage::UWxCueNotify_Damage()
{
	GameplayCueTag = WxGameplayTags::GameplayCue_Damage;
}

void UWxCueNotify_Damage::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	Super::HandleGameplayCue(MyTarget, EventType, Parameters);

	if (EventType != EGameplayCueEvent::Executed)
	{
		return;
	}

	if (!MyTarget || !FloaterWidgetClass)
	{
		return;
	}

	UWorld* World = MyTarget->GetWorld();
	if (!World)
	{
		return;
	}

	// 데미지 플로터 출력 여부 확인
	if (!Parameters.AggregatedSourceTags.HasTag(WxGameplayTags::Damage_SuppressFloater))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AWxDamageFloaterActor* FloaterActor = World->SpawnActor<AWxDamageFloaterActor>(Parameters.Location, FRotator::ZeroRotator, SpawnParams);
		if (FloaterActor)
		{
			const float Damage = Parameters.RawMagnitude;
			const bool bIsCritical = Parameters.AggregatedSourceTags.HasTag(WxGameplayTags::Damage_Critical);
			FloaterActor->InitDamageInfo(FloaterWidgetClass, Damage, bIsCritical);
		}
	}

	if (HitNiagaraSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, HitNiagaraSystem, Parameters.Location);
	}
}

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
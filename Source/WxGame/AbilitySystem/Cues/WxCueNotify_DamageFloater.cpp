// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Cues/WxCueNotify_DamageFloater.h"
#include "WxGame.h"
#include "WxGameplayTags.h"
#include "Components/WidgetComponent.h"
#include "UI/MVVM/WxViewModel_Damage.h"
#include "View/MVVMView.h"

UWxCueNotify_DamageFloater::UWxCueNotify_DamageFloater()
{
	GameplayCueTag = WxGameplayTags::GameplayCue_DamageFloater;
}

void UWxCueNotify_DamageFloater::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
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

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AWxDamageFloaterActor* FloaterActor = World->SpawnActor<AWxDamageFloaterActor>(MyTarget->GetActorLocation(), FRotator::ZeroRotator, SpawnParams))
	{
		const float Damage = Parameters.RawMagnitude;
		const bool bIsCritical = Parameters.AggregatedSourceTags.HasTag(WxGameplayTags::Damage_Critical);
		FloaterActor->InitDamageInfo(FloaterWidgetClass, Damage, bIsCritical);
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

	// Slate 가 없는 실행(헤드리스)에서는 엔진이 위젯을 만들지 않는 게 정상이라 경고 없이 빠진다.
	UUserWidget* Widget = WidgetComponent->GetUserWidgetObject();
	if (!Widget)
	{
		return;
	}

	UWxViewModel_Damage* ViewModel = NewObject<UWxViewModel_Damage>(Widget);
	ViewModel->Damage = InDamageAmount;
	ViewModel->bIsCritical = bInIsCritical;

	UMVVMView* View = Widget->GetExtension<UMVVMView>();
	if (!View || !View->SetViewModelByClass(ViewModel))
	{
		UE_LOG(LogWxUI, Warning, TEXT("DamageFloater: Damage 뷰모델을 넣지 못했다. 위젯의 MVVM View·Manual 소스를 확인한다. Widget=%s"), *GetNameSafe(Widget->GetClass()));
	}
}
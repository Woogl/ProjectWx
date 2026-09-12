// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Nameplate.h"

#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameplayTagAssetInterface.h"
#include "Kismet/GameplayStatics.h"

void UWxViewModel_Nameplate::Initialize(UWidgetComponent* InNameplate)
{
	Deinitialize();
	Nameplate = InNameplate;
	if (HandleUpdatePresentation(0.f))
	{
		// Collapsed 위젯은 UMG Tick이 멈추므로, 다시 표시할 조건은 위젯 틱 밖에서 갱신한다.
		UpdateHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &ThisClass::HandleUpdatePresentation));
	}
}

void UWxViewModel_Nameplate::Deinitialize()
{
	if (UpdateHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(UpdateHandle);
		UpdateHandle.Reset();
	}
	Nameplate.Reset();
	if (!HasAnyFlags(RF_BeginDestroyed))
	{
		UE_MVVM_SET_PROPERTY_VALUE(bHasViewer, false);
	}
	Super::Deinitialize();
}

bool UWxViewModel_Nameplate::HandleUpdatePresentation(float DeltaTime)
{
	const UWidgetComponent* Component = Nameplate.Get();
	if (!Component || !Component->GetWorld() || Component->GetWorld()->bIsTearingDown)
	{
		UpdateHandle.Reset();
		UE_MVVM_SET_PROPERTY_VALUE(bHasViewer, false);
		return false;
	}

	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(Component, 0);
	if (!PlayerPawn)
	{
		UE_MVVM_SET_PROPERTY_VALUE(bHasViewer, false);
		return true;
	}

	const double NewDistance = FVector::Dist(Component->GetComponentLocation(), PlayerPawn->GetActorLocation());
	FGameplayTagContainer NewTags;
	if (const IGameplayTagAssetInterface* TagOwner = Cast<IGameplayTagAssetInterface>(Component->GetOwner()))
	{
		TagOwner->GetOwnedGameplayTags(NewTags);
	}
	const bool bDistanceChanged = !FMath::IsNearlyEqual(Distance, NewDistance, UE_DOUBLE_KINDA_SMALL_NUMBER);
	const bool bTagsChanged = OwnedTags != NewTags;
	const bool bViewerChanged = !bHasViewer;
	// Immediate 변환도 같은 시점의 입력을 읽도록 값을 먼저 갱신한다.
	if (bDistanceChanged)
	{
		Distance = NewDistance;
	}
	OwnedTags = MoveTemp(NewTags);
	bHasViewer = true;
	if (bDistanceChanged)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Distance);
	}
	if (bTagsChanged)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(OwnedTags);
	}
	if (bViewerChanged)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bHasViewer);
	}
	return true;
}

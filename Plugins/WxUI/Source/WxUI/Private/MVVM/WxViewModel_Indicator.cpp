// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Indicator.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Indicator/WxIndicatorDescriptor.h"
#include "Indicator/WxIndicatorManagerComponent.h"

void UWxViewModel_Indicator::StartObserving(APlayerController* PC, int32 InIndicatorIndex)
{
	if (!PC)
	{
		return;
	}

	ObservedController = PC;
	IndicatorIndex = InIndicatorIndex;

	// 이미 붙어 있으면 기다릴 것 없이 바로 연결한다.
	if (UWxIndicatorManagerComponent* Manager = PC->FindComponentByClass<UWxIndicatorManagerComponent>())
	{
		Initialize(Manager);
		return;
	}

	ManagerReadyHandle = UWxIndicatorManagerComponent::OnAnyManagerReady.AddUObject(this, &UWxViewModel_Indicator::HandleManagerReady);
}

void UWxViewModel_Indicator::Initialize(UWxIndicatorManagerComponent* InManager)
{
	if (!InManager)
	{
		return;
	}

	Deinitialize();

	CachedManager = InManager;

	InManager->OnIndicatorsUpdated.AddUObject(this, &UWxViewModel_Indicator::HandleIndicatorsUpdated);

	// 구독 전에 끝난 broadcast 가 있을 수 있으므로 현재 등록 상태로 시드한다.
	HandleIndicatorsUpdated();
}

void UWxViewModel_Indicator::Deinitialize()
{
	if (UWxIndicatorManagerComponent* Manager = CachedManager.Get())
	{
		Manager->OnIndicatorsUpdated.RemoveAll(this);
	}
	CachedManager.Reset();

	ApplyIndicator(nullptr);

	Super::Deinitialize();
}

void UWxViewModel_Indicator::BeginDestroy()
{
	StopObserving();

	Super::BeginDestroy();
}

void UWxViewModel_Indicator::HandleManagerReady(UWxIndicatorManagerComponent* Manager)
{
	// 신호는 클래스 차원이라 남의 매니저도 온다(PIE 다중 인스턴스 포함). 관찰 중인 컨트롤러의 것만 받는다.
	if (!Manager || Manager->GetOwner() != ObservedController.Get())
	{
		return;
	}

	StopObserving();
	Initialize(Manager);
}

void UWxViewModel_Indicator::HandleIndicatorsUpdated()
{
	const UWxIndicatorDescriptor* Indicator = nullptr;

	if (const UWxIndicatorManagerComponent* Manager = CachedManager.Get())
	{
		const TArray<TObjectPtr<UWxIndicatorDescriptor>>& Registered = Manager->GetIndicators();
		if (Registered.IsValidIndex(IndicatorIndex))
		{
			Indicator = Registered[IndicatorIndex];
		}
	}

	ApplyIndicator(Indicator);
}

void UWxViewModel_Indicator::StopObserving()
{
	if (ManagerReadyHandle.IsValid())
	{
		UWxIndicatorManagerComponent::OnAnyManagerReady.Remove(ManagerReadyHandle);
		ManagerReadyHandle.Reset();
	}
}

void UWxViewModel_Indicator::ApplyIndicator(const UWxIndicatorDescriptor* Indicator)
{
	const bool bNewHasIndicator = Indicator && Indicator->IsProjected();

	// 위치를 먼저 반영하고 표시를 켠다 — 순서가 반대면 켜지는 프레임에 이전 좌표가 보인다.
	if (bNewHasIndicator)
	{
		if (ScreenPosition != Indicator->GetScreenPosition())
		{
			ScreenPosition = Indicator->GetScreenPosition();
			UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ScreenPosition);
		}

		if (DistanceMeters != Indicator->GetDistanceMeters())
		{
			DistanceMeters = Indicator->GetDistanceMeters();
			UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(DistanceMeters);
		}

		if (bClamped != Indicator->IsClamped())
		{
			bClamped = Indicator->IsClamped();
			UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bClamped);
		}
	}

	if (bHasIndicator != bNewHasIndicator)
	{
		bHasIndicator = bNewHasIndicator;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bHasIndicator);
	}
}

UObject* UWxViewModelResolver_Indicator::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	if (!PC)
	{
		return nullptr;
	}

	// 매니저가 아직 없을 수 있으므로 Outer 는 PC 로 잡는다. 연결은 VM 이 관찰로 스스로 처리한다.
	UWxViewModel_Indicator* ViewModel = NewObject<UWxViewModel_Indicator>(PC);
	ViewModel->StartObserving(PC, IndicatorIndex);
	return ViewModel;
}

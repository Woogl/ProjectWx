// Copyright Woogle. All Rights Reserved.

#include "Component/WxNameplateComponent.h"
#include "AbilitySystemComponent.h"
#include "View/MVVMView.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "Targeting/WxLockOnComponent.h"
#include "WxGameplayTags.h"
#include "GameFramework/PlayerController.h"

UWxNameplateComponent::UWxNameplateComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawAtDesiredSize(true);

	// 기본 숨김: 적의 존재를 미리 노출하지 않는다. 인식/락온 시 RefreshVisibility 가 노출한다.
	SetVisibility(false);
}

void UWxNameplateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	RefreshVisibility();

	UUserWidget* NameplateWidget = GetWidget();
	if (!NameplateWidget)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

	const float Distance = FVector::Dist(GetComponentLocation(), CameraLocation);
	const float Scale = FMath::Clamp(ReferenceDistance / FMath::Max(Distance, 1.0f), MinScale, MaxScale);
	NameplateWidget->SetRenderScale(FVector2D(Scale, Scale));
}

void UWxNameplateComponent::InitializeViewModels(UAbilitySystemComponent* InASC)
{
	UUserWidget* NameplateWidget = GetWidget();
	if (!NameplateWidget)
	{
		return;
	}

	UMVVMView* View = NameplateWidget->GetExtension<UMVVMView>();
	if (!View)
	{
		return;
	}

	UWxViewModel_AbilitySystem* AbilitySystemViewModel = NewObject<UWxViewModel_AbilitySystem>(InASC);
	AbilitySystemViewModel->Initialize(InASC);
	View->SetViewModelByClass(AbilitySystemViewModel);

	CachedASC = InASC;
}

void UWxNameplateComponent::RefreshVisibility()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		return;
	}

	const bool bDead = ASC->HasMatchingGameplayTag(WxGameplayTags::State_Dead);
	const bool bRecognized = ASC->HasMatchingGameplayTag(WxGameplayTags::State_Recognized);

	// SetVisibility 는 값이 동일하면 내부에서 no-op 처리되므로 매 틱 호출해도 부담이 없다.
	SetVisibility(!bDead && (bRecognized || IsLockedOnByLocalPlayer()));
}

bool UWxNameplateComponent::IsLockedOnByLocalPlayer() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		return false;
	}

	const UWxLockOnComponent* LockOnComponent = UWxLockOnComponent::FindComponent(PC->GetPawn());
	return LockOnComponent && LockOnComponent->GetLockOnTarget() == GetOwner();
}

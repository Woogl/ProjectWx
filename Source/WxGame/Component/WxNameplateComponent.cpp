// Copyright Woogle. All Rights Reserved.

#include "Component/WxNameplateComponent.h"
#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "View/MVVMView.h"
#include "Character/WxCharacterBase.h"
#include "MVVM/WxViewModel_Character.h"
#include "Targeting/WxLockOnManagerComponent.h"
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

	// 캐릭터 Composite VM 하나로 어트리뷰트/이펙트(자식 AbilitySystem VM)와 표시 데이터(이름/초상화/설명)를 함께 노출한다.
	if (const AWxCharacterBase* OwnerCharacter = Cast<AWxCharacterBase>(GetOwner()))
	{
		UWxViewModel_Character* CharacterViewModel = NewObject<UWxViewModel_Character>(this);
		CharacterViewModel->Initialize(InASC, OwnerCharacter->GetCharacterUIData());
		View->SetViewModelByClass(CharacterViewModel);
	}

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
	const bool bRecognized = ASC->HasMatchingGameplayTag(WxGameplayTags::State_InCombat);

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

	const UWxLockOnManagerComponent* LockOnManagerComponent = UWxLockOnManagerComponent::FindComponent(PC->GetPawn());
	if (!LockOnManagerComponent)
	{
		return false;
	}

	// 락온 대상은 컴포넌트(부위) 단위이므로, 소유 액터가 이 네임플레이트의 액터인지로 판정한다.
	const USceneComponent* LockOnTarget = LockOnManagerComponent->GetLockOnTarget();
	return LockOnTarget && LockOnTarget->GetOwner() == GetOwner();
}

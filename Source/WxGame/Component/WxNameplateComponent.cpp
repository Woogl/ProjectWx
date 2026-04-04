// Copyright Woogle. All Rights Reserved.

#include "Component/WxNameplateComponent.h"
#include "AbilitySystemComponent.h"
#include "View/MVVMView.h"
#include "MVVM/WxViewModel_Attribute.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxViewModel_GameplayTag.h"
#include "AbilitySystem/WxCombatAttributeSet.h"

UWxNameplateComponent::UWxNameplateComponent()
{
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawAtDesiredSize(true);
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

	UWxViewModel_Attribute* HealthViewModel = NewObject<UWxViewModel_Attribute>(InASC);
	HealthViewModel->Initialize(InASC, UWxCombatAttributeSet::GetHPAttribute(), UWxCombatAttributeSet::GetMaxHPAttribute());
	View->SetViewModel(TEXT("VM_Health"), HealthViewModel);

	UWxViewModel_Attribute* DazeViewModel = NewObject<UWxViewModel_Attribute>(InASC);
	DazeViewModel->Initialize(InASC, UWxCombatAttributeSet::GetDPAttribute(), UWxCombatAttributeSet::GetMaxDPAttribute());
	View->SetViewModel(TEXT("VM_Daze"), DazeViewModel);

	UWxViewModel_GameplayTag* GameplayTagViewModel = NewObject<UWxViewModel_GameplayTag>(InASC);
	GameplayTagViewModel->Initialize(InASC);
	View->SetViewModel(TEXT("VM_GameplayTag"), GameplayTagViewModel);

	UWxViewModel_AbilitySystem* AbilitySystemViewModel = NewObject<UWxViewModel_AbilitySystem>(InASC);
	AbilitySystemViewModel->Initialize(InASC);
	View->SetViewModel(TEXT("VM_AbilitySystem"), AbilitySystemViewModel);
}

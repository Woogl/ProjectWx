// Copyright Woogle. All Rights Reserved.

#include "Component/WxNameplateComponent.h"
#include "AbilitySystemComponent.h"
#include "View/MVVMView.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "WxGameplayTags.h"

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

	UWxViewModel_AbilitySystem* AbilitySystemViewModel = NewObject<UWxViewModel_AbilitySystem>(InASC);
	AbilitySystemViewModel->Initialize(InASC);
	View->SetViewModelByClass(AbilitySystemViewModel);

	InASC->RegisterGameplayTagEvent(WxGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UWxNameplateComponent::HandleDeadTagChanged);
}

void UWxNameplateComponent::HandleDeadTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	SetVisibility(NewCount == 0);
}

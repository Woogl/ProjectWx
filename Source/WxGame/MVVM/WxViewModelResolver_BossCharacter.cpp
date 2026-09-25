// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_BossCharacter.h"
#include "AbilitySystemComponent.h"
#include "Battle/WxBattleSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Character/WxCharacterBase.h"
#include "Engine/World.h"
#include "MVVM/WxViewModel_Character.h"
#include "MVVM/WxViewModelResolver_AbilitySystem.h"

UObject* UWxViewModelResolver_BossCharacter::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	UWxBattleSubsystem* Battle = UserWidget ? UWorld::GetSubsystem<UWxBattleSubsystem>(UserWidget->GetWorld()) : nullptr;
	if (!Battle)
	{
		return nullptr;
	}

	UWxViewModel_Character* ViewModel = NewObject<UWxViewModel_Character>(const_cast<UUserWidget*>(UserWidget));
	const FWxOnCurrentBossChanged::FDelegate ApplyBoss = FWxOnCurrentBossChanged::FDelegate::CreateWeakLambda(ViewModel, [ViewModel](AWxCharacterBase* CurrentBoss)
	{
		if (CurrentBoss)
		{
			ViewModel->Initialize(UWxViewModelResolver_AbilitySystem::GetOrCreate(CurrentBoss->GetAbilitySystemComponent()),
				CurrentBoss->GetTitle(), CurrentBoss->GetIcon());
		}
		else
		{
			ViewModel->Deinitialize();
		}
	});

	// 위젯보다 먼저 시작된 보스전도 보여 준다.
	ApplyBoss.Execute(Battle->GetCurrentBoss());
	Battle->OnCurrentBossChanged.Add(ApplyBoss);
	return ViewModel;
}

void UWxViewModelResolver_BossCharacter::DestroyInstance(UObject* ViewModel, const UMVVMView* View) const
{
	if (UWxBattleSubsystem* Battle = ViewModel ? UWorld::GetSubsystem<UWxBattleSubsystem>(ViewModel->GetWorld()) : nullptr)
	{
		Battle->OnCurrentBossChanged.RemoveAll(ViewModel);
	}
}

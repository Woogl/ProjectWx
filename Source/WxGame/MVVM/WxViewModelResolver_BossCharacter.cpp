// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_BossCharacter.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/WxEnemyCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxViewModel_Character.h"
#include "WxGameplayTags.h"

UObject* UWxViewModelResolver_BossCharacter::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	UWorld* World = UserWidget ? UserWidget->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}

	UWxViewModel_Character* ViewModel = UWxViewModel_Character::GetOrCreate(World->GetGameState());

	// 리졸버 인스턴스는 위젯 클래스가 공유하므로 위젯마다 한 번씩 여기에 들어온다.
	AWxEnemyCharacter::OnAnyBossEngagementChanged.RemoveAll(this);
	AWxEnemyCharacter::OnAnyBossEngagementChanged.AddUObject(this, &ThisClass::HandleBossEngagementChanged);

	// 위젯이 보스보다 늦게 생기면 통지를 놓치므로 이미 교전 중인 보스를 훑어 시드한다.
	for (TActorIterator<AWxEnemyCharacter> It(World); It; ++It)
	{
		if (It->IsBoss() && It->GetAbilitySystemComponent()->HasMatchingGameplayTag(WxGameplayTags::State_Engaged))
		{
			HandleBossEngagementChanged(*It, true);
			break;
		}
	}

	return ViewModel;
}

void UWxViewModelResolver_BossCharacter::DestroyInstance(UObject* ViewModel, const UMVVMView* View) const
{
	AWxEnemyCharacter::OnAnyBossEngagementChanged.RemoveAll(this);
}

void UWxViewModelResolver_BossCharacter::HandleBossEngagementChanged(AWxEnemyCharacter* BossCharacter, bool bEngaged) const
{
	const UWorld* World = BossCharacter ? BossCharacter->GetWorld() : nullptr;
	UWxViewModel_Character* ViewModel = World ? UWxViewModel_Character::GetOrCreate(World->GetGameState()) : nullptr;
	if (!ViewModel)
	{
		return;
	}

	UAbilitySystemComponent* ASC = BossCharacter->GetAbilitySystemComponent();
	if (bEngaged)
	{
		ViewModel->Initialize(ASC, BossCharacter->GetCharacterName(), BossCharacter->GetPortrait());
	}
	else if (ViewModel->AbilitySystem && ViewModel->AbilitySystem->GetOuter() == ASC)
	{
		// 자식 뷰모델의 Outer 가 곧 그것을 발행한 ASC 다 — 그 사이 다른 보스가 자리를 가져갔으면 남의 표시를 지우지 않는다.
		ViewModel->Deinitialize();
	}
}

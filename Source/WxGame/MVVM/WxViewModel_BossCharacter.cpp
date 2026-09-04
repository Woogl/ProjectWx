// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_BossCharacter.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/WxEnemyCharacter.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "WxGameplayTags.h"

void UWxViewModel_BossCharacter::StartObserving(UWorld* World)
{
	if (!World)
	{
		return;
	}

	ObservedWorld = World;
	BossReadyHandle = AWxEnemyCharacter::OnAnyBossReady.AddUObject(this, &UWxViewModel_BossCharacter::HandleBossReady);

	// 위젯이 보스보다 늦게 생기면 발행을 놓치므로 이미 있는 보스를 훑어 시드한다.
	for (TActorIterator<AWxEnemyCharacter> It(World); It; ++It)
	{
		if (It->IsBoss())
		{
			SetBoss(*It);
			break;
		}
	}
}

void UWxViewModel_BossCharacter::BeginDestroy()
{
	AWxEnemyCharacter::OnAnyBossReady.Remove(BossReadyHandle);
	UnbindBoss();

	Super::BeginDestroy();
}

void UWxViewModel_BossCharacter::HandleBossReady(AWxEnemyCharacter* BossCharacter)
{
	// 클래스 차원 델리게이트라 PIE 에선 서버·클라 월드가 함께 실린다.
	if (BossCharacter && BossCharacter->GetWorld() == ObservedWorld.Get())
	{
		SetBoss(BossCharacter);
	}
}

void UWxViewModel_BossCharacter::HandleBossEndPlay(AWxEnemyCharacter* BossCharacter)
{
	if (BossCharacter == CurrentBossCharacter.Get())
	{
		SetBoss(nullptr);
	}
}

void UWxViewModel_BossCharacter::SetBoss(AWxEnemyCharacter* BossCharacter)
{
	UnbindBoss();

	if (!BossCharacter || !BossCharacter->IsBoss())
	{
		SetBossBattleActive(false);
		Deinitialize();
		return;
	}

	UAbilitySystemComponent* ASC = BossCharacter->GetAbilitySystemComponent();
	if (!ASC)
	{
		SetBossBattleActive(false);
		Deinitialize();
		return;
	}

	CurrentBossCharacter = BossCharacter;
	ObservedAbilitySystem = ASC;
	BossCharacter->OnBossEndPlay.AddUObject(this, &ThisClass::HandleBossEndPlay);
	EngagementTagHandle = ASC->RegisterGameplayTagEvent(WxGameplayTags::State_Engaged, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleEngagementTagChanged);
	Initialize(ASC, BossCharacter->GetCharacterName(), BossCharacter->GetPortrait());
	SetBossBattleActive(ASC->HasMatchingGameplayTag(WxGameplayTags::State_Engaged));
}

void UWxViewModel_BossCharacter::UnbindBoss()
{
	if (AWxEnemyCharacter* BossCharacter = CurrentBossCharacter.Get())
	{
		BossCharacter->OnBossEndPlay.RemoveAll(this);
	}

	if (UAbilitySystemComponent* ASC = ObservedAbilitySystem.Get())
	{
		ASC->RegisterGameplayTagEvent(WxGameplayTags::State_Engaged, EGameplayTagEventType::NewOrRemoved).Remove(EngagementTagHandle);
	}

	EngagementTagHandle.Reset();
	ObservedAbilitySystem.Reset();
	CurrentBossCharacter.Reset();
}

void UWxViewModel_BossCharacter::HandleEngagementTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	SetBossBattleActive(NewCount > 0);
}

void UWxViewModel_BossCharacter::SetBossBattleActive(bool bActive)
{
	UE_MVVM_SET_PROPERTY_VALUE(bBossBattleActive, bActive);
}

UObject* UWxViewModelResolver_BossCharacter::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	if (!PC)
	{
		return nullptr;
	}

	UWxViewModel_BossCharacter* ViewModel = NewObject<UWxViewModel_BossCharacter>(PC);
	ViewModel->StartObserving(PC->GetWorld());
	return ViewModel;
}

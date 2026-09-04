// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_BossCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Character/WxEnemyCharacter.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

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

	if (AWxEnemyCharacter* BossCharacter = CurrentBossCharacter.Get())
	{
		BossCharacter->OnBossEndPlay.RemoveAll(this);
		BossCharacter->OnEngagementChanged.RemoveAll(this);
	}

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
	if (AWxEnemyCharacter* PreviousBossCharacter = CurrentBossCharacter.Get())
	{
		PreviousBossCharacter->OnBossEndPlay.RemoveAll(this);
		PreviousBossCharacter->OnEngagementChanged.RemoveAll(this);
	}

	CurrentBossCharacter = BossCharacter;

	if (!BossCharacter || !BossCharacter->IsBoss())
	{
		CurrentBossCharacter.Reset();
		HandleEngagementChanged(false);
		Deinitialize();
		return;
	}

	BossCharacter->OnBossEndPlay.AddUObject(this, &ThisClass::HandleBossEndPlay);
	BossCharacter->OnEngagementChanged.AddUObject(this, &ThisClass::HandleEngagementChanged);
	Initialize(BossCharacter->GetAbilitySystemComponent(), BossCharacter->GetCharacterName(), BossCharacter->GetPortrait());
	HandleEngagementChanged(BossCharacter->IsEngaged());
}

void UWxViewModel_BossCharacter::HandleEngagementChanged(bool bEngaged)
{
	UE_MVVM_SET_PROPERTY_VALUE(bBossBattleActive, bEngaged);
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

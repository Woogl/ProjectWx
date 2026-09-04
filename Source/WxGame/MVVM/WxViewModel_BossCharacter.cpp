// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_BossCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Character/Component/WxBossComponent.h"
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
	BossReadyHandle = UWxBossComponent::OnAnyBossReady.AddUObject(this, &UWxViewModel_BossCharacter::HandleBossReady);

	// 위젯이 보스보다 늦게 생기면 발행을 놓치므로 이미 있는 보스를 훑어 시드한다.
	for (TActorIterator<AWxEnemyCharacter> It(World); It; ++It)
	{
		if (UWxBossComponent* BossComponent = It->FindComponentByClass<UWxBossComponent>())
		{
			SetBoss(BossComponent);
			break;
		}
	}
}

void UWxViewModel_BossCharacter::BeginDestroy()
{
	UWxBossComponent::OnAnyBossReady.Remove(BossReadyHandle);

	if (UWxBossComponent* BossComponent = CurrentBossComponent.Get())
	{
		BossComponent->OnBossEndPlay.RemoveAll(this);
		BossComponent->OnEngagementChanged.RemoveAll(this);
	}

	Super::BeginDestroy();
}

void UWxViewModel_BossCharacter::HandleBossReady(UWxBossComponent* BossComponent)
{
	// 클래스 차원 델리게이트라 PIE 에선 서버·클라 월드가 함께 실린다.
	if (BossComponent && BossComponent->GetWorld() == ObservedWorld.Get())
	{
		SetBoss(BossComponent);
	}
}

void UWxViewModel_BossCharacter::HandleBossEndPlay(UWxBossComponent* BossComponent)
{
	if (BossComponent == CurrentBossComponent.Get())
	{
		SetBoss(nullptr);
	}
}

void UWxViewModel_BossCharacter::SetBoss(UWxBossComponent* BossComponent)
{
	if (UWxBossComponent* PreviousBossComponent = CurrentBossComponent.Get())
	{
		PreviousBossComponent->OnBossEndPlay.RemoveAll(this);
		PreviousBossComponent->OnEngagementChanged.RemoveAll(this);
	}

	CurrentBossComponent = BossComponent;

	if (!BossComponent)
	{
		HandleEngagementChanged(false);
		Deinitialize();
		return;
	}

	AWxEnemyCharacter* Boss = BossComponent->GetBossCharacter();
	if (!Boss)
	{
		CurrentBossComponent.Reset();
		HandleEngagementChanged(false);
		Deinitialize();
		return;
	}

	BossComponent->OnBossEndPlay.AddUObject(this, &ThisClass::HandleBossEndPlay);
	BossComponent->OnEngagementChanged.AddUObject(this, &ThisClass::HandleEngagementChanged);
	Initialize(Boss->GetAbilitySystemComponent(), Boss->GetCharacterName(), Boss->GetPortrait());
	HandleEngagementChanged(BossComponent->IsEngaged());
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

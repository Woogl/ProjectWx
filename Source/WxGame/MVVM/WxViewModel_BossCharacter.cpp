// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_BossCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Character/Component/WxEnemyComponent.h"
#include "Character/WxCharacterBase.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

void UWxViewModel_BossCharacter::StartObserving(UWorld* World)
{
	if (!World)
	{
		return;
	}

	ObservedWorld = World;
	BossReadyHandle = UWxEnemyComponent::OnAnyBossReady.AddUObject(this, &UWxViewModel_BossCharacter::HandleBossReady);

	// 위젯이 보스보다 늦게 생기면 발행을 놓치므로 이미 있는 보스를 훑어 시드한다.
	for (TActorIterator<AWxCharacterBase> It(World); It; ++It)
	{
		if (UWxEnemyComponent* EnemyComponent = It->FindComponentByClass<UWxEnemyComponent>(); EnemyComponent && EnemyComponent->IsBoss())
		{
			SetBoss(EnemyComponent);
			break;
		}
	}
}

void UWxViewModel_BossCharacter::BeginDestroy()
{
	UWxEnemyComponent::OnAnyBossReady.Remove(BossReadyHandle);

	if (UWxEnemyComponent* EnemyComponent = CurrentBossComponent.Get())
	{
		EnemyComponent->OnBossEndPlay.RemoveAll(this);
		EnemyComponent->OnEngagementChanged.RemoveAll(this);
	}

	Super::BeginDestroy();
}

void UWxViewModel_BossCharacter::HandleBossReady(UWxEnemyComponent* EnemyComponent)
{
	// 클래스 차원 델리게이트라 PIE 에선 서버·클라 월드가 함께 실린다.
	if (EnemyComponent && EnemyComponent->GetWorld() == ObservedWorld.Get())
	{
		SetBoss(EnemyComponent);
	}
}

void UWxViewModel_BossCharacter::HandleBossEndPlay(UWxEnemyComponent* EnemyComponent)
{
	if (EnemyComponent == CurrentBossComponent.Get())
	{
		SetBoss(nullptr);
	}
}

void UWxViewModel_BossCharacter::SetBoss(UWxEnemyComponent* EnemyComponent)
{
	if (UWxEnemyComponent* PreviousEnemyComponent = CurrentBossComponent.Get())
	{
		PreviousEnemyComponent->OnBossEndPlay.RemoveAll(this);
		PreviousEnemyComponent->OnEngagementChanged.RemoveAll(this);
	}

	CurrentBossComponent = EnemyComponent;

	if (!EnemyComponent || !EnemyComponent->IsBoss())
	{
		CurrentBossComponent.Reset();
		HandleEngagementChanged(false);
		Deinitialize();
		return;
	}

	AWxCharacterBase* Boss = EnemyComponent->GetEnemyCharacter();
	if (!Boss)
	{
		CurrentBossComponent.Reset();
		HandleEngagementChanged(false);
		Deinitialize();
		return;
	}

	EnemyComponent->OnBossEndPlay.AddUObject(this, &ThisClass::HandleBossEndPlay);
	EnemyComponent->OnEngagementChanged.AddUObject(this, &ThisClass::HandleEngagementChanged);
	Initialize(Boss->GetAbilitySystemComponent(), Boss->GetCharacterName(), Boss->GetPortrait());
	HandleEngagementChanged(EnemyComponent->IsEngaged());
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

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
		if (It->FindComponentByClass<UWxBossComponent>())
		{
			SetBoss(*It);
			break;
		}
	}
}

void UWxViewModel_BossCharacter::BeginDestroy()
{
	UWxBossComponent::OnAnyBossReady.Remove(BossReadyHandle);

	if (AWxEnemyCharacter* Boss = CurrentBoss.Get())
	{
		Boss->OnEndPlay.RemoveDynamic(this, &UWxViewModel_BossCharacter::HandleBossEndPlay);
		Boss->OnAITargetChanged.RemoveAll(this);
	}

	Super::BeginDestroy();
}

void UWxViewModel_BossCharacter::HandleBossReady(UWxBossComponent* BossComponent)
{
	// 클래스 차원 델리게이트라 PIE 에선 서버·클라 월드가 함께 실린다.
	if (BossComponent && BossComponent->GetWorld() == ObservedWorld.Get())
	{
		SetBoss(BossComponent->GetBossCharacter());
	}
}

void UWxViewModel_BossCharacter::HandleBossEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	if (Actor == CurrentBoss.Get())
	{
		SetBoss(nullptr);
	}
}

void UWxViewModel_BossCharacter::SetBoss(AWxEnemyCharacter* Boss)
{
	if (AWxEnemyCharacter* PreviousBoss = CurrentBoss.Get())
	{
		PreviousBoss->OnEndPlay.RemoveDynamic(this, &UWxViewModel_BossCharacter::HandleBossEndPlay);
		PreviousBoss->OnAITargetChanged.RemoveAll(this);
	}

	CurrentBoss = Boss;

	if (!Boss)
	{
		HandleAITargetChanged(false);
		Deinitialize();
		return;
	}

	Boss->OnEndPlay.AddDynamic(this, &UWxViewModel_BossCharacter::HandleBossEndPlay);
	Boss->OnAITargetChanged.AddUObject(this, &UWxViewModel_BossCharacter::HandleAITargetChanged);
	Initialize(Boss->GetAbilitySystemComponent(), Boss->GetCharacterName(), Boss->GetPortrait());
	HandleAITargetChanged(Boss->HasAITarget());
}

void UWxViewModel_BossCharacter::HandleAITargetChanged(bool bNewHasAITarget)
{
	UE_MVVM_SET_PROPERTY_VALUE(bHasAITarget, bNewHasAITarget);
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

// Copyright Woogle. All Rights Reserved.

#include "Character/WxBossCharacter.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "WxGameplayTags.h"
#include "MVVM/WxGlobalViewModelSubsystem.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void AWxBossCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 보스 체력바는 인식 지속 신호(State.Recognized)를 따른다. TargetActor 는 시야 상실 즉시 비워지므로(보스 등 뒤로 이동 등 일시적 시야 상실 포함) 표시 기준으로 쓰지 않는다. 인식이 유지되는 한 체력바도 유지된다.
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RegisterGameplayTagEvent(WxGameplayTags::State_Recognized, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &AWxBossCharacter::HandleRecognizedTagChanged);
	}
}

void AWxBossCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DeactivateBossAbilitySystemViewModel();

	Super::EndPlay(EndPlayReason);
}

void AWxBossCharacter::HandleRecognizedTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		ActivateBossAbilitySystemViewModel();
	}
	else
	{
		DeactivateBossAbilitySystemViewModel();
	}
}

void AWxBossCharacter::ActivateBossAbilitySystemViewModel()
{
	if (UWxViewModel_AbilitySystem* VM = GetBossAbilitySystemViewModel())
	{
		VM->Initialize(AbilitySystemComponent);
	}
}

void AWxBossCharacter::DeactivateBossAbilitySystemViewModel()
{
	if (UWxViewModel_AbilitySystem* VM = GetBossAbilitySystemViewModel())
	{
		VM->Deinitialize();
	}
}

UWxViewModel_AbilitySystem* AWxBossCharacter::GetBossAbilitySystemViewModel() const
{
	const UWorld* World = GetWorld();
	APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	if (!LocalPlayer)
	{
		return nullptr;
	}

	UWxGlobalViewModelSubsystem* Subsystem = LocalPlayer->GetSubsystem<UWxGlobalViewModelSubsystem>();
	return Subsystem ? Subsystem->GetBossAbilitySystemViewModel() : nullptr;
}

// Copyright Woogle. All Rights Reserved.

#include "Cheat/WxCheatManager.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/Effect/WxEffect_Kill.h"
#include "EngineUtils.h"
#include "WxCombatLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "WxGame.h"
#include "WxGameplayTags.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"

void UWxCheatManager::WxKillPlayer()
{
	const APlayerController* PC = GetOuterAPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
	if (!AbilitySystem)
	{
		return;
	}

	// HP 를 직접 0 으로 쓰면 클램프만 되고 사망이 발동하지 않는다 — 사망 처리는 IncomingDamage 경로에서만 Event.Death 를 송출한다.
	// MakeEffectContext 가 자기 자신을 Instigator 로 실어 주므로 AI 보고까지 유효한 가해자를 갖는다.
	const FGameplayEffectSpecHandle SpecHandle = AbilitySystem->MakeOutgoingSpec(UWxEffect_Kill::StaticClass(), 1.f, AbilitySystem->MakeEffectContext());
	if (SpecHandle.IsValid())
	{
		AbilitySystem->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void UWxCheatManager::WxDamagePlayer(float Amount)
{
	// 0 이하는 IncomingDamage 소비 단계의 Damage > 0 조건에 걸려 GE 만 헛돈다.
	if (Amount <= 0.f)
	{
		return;
	}

	const APlayerController* PC = GetOuterAPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
	if (!AbilitySystem)
	{
		return;
	}
	
	UWxCombatLibrary::ApplyAttributeChange(AbilitySystem, UWxCombatAttributeSet::GetIncomingDamageAttribute(), Amount);
}

void UWxCheatManager::WxKillEnemies(float RadiusMeters)
{
	const APlayerController* PC = GetOuterAPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
	UWorld* World = Pawn ? Pawn->GetWorld() : nullptr;
	if (!AbilitySystem || !World)
	{
		return;
	}

	const float RadiusSquared = FMath::Square(RadiusMeters * 100.f);
	const FVector Origin = Pawn->GetActorLocation();
	int32 KillCount = 0;

	// 오버랩 질의 대신 액터 순회로 찾는다 — 콜리전 설정에 따라 대상이 새면 "구역을 비운다"는 목적 자체가 깨진다.
	// 죽일 수 있는 대상은 결국 ASC 를 가진 액터뿐이라, 클래스로 좁히지 않고 ASC 유무를 필터로 쓴다.
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (FVector::DistSquared(Origin, Actor->GetActorLocation()) > RadiusSquared)
		{
			continue;
		}

		UAbilitySystemComponent* TargetAbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
		if (!TargetAbilitySystem || TargetAbilitySystem == AbilitySystem)
		{
			continue;
		}

		// 이미 죽은 대상은 HP 가 0 이라 어차피 아무 일도 없지만, 헛도는 GE 를 막고 처치 수를 정확히 세기 위해 거른다.
		if (TargetAbilitySystem->HasMatchingGameplayTag(WxGameplayTags::Ability_Death))
		{
			continue;
		}

		// 가해자를 플레이어로 남기기 위해 대상 ASC 에 self-apply 하지 않고 플레이어 ASC 에서 대상으로 적용한다.
		// 즉사 GE 의 HP 캡처는 Target 소스라 대상별로 각자의 HP 가 잡힌다.
		const FGameplayEffectSpecHandle SpecHandle = AbilitySystem->MakeOutgoingSpec(UWxEffect_Kill::StaticClass(), 1.f, AbilitySystem->MakeEffectContext());
		if (SpecHandle.IsValid())
		{
			AbilitySystem->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetAbilitySystem);
			++KillCount;
		}
	}

	// 대상이 없었던 것과 치트가 안 먹은 것을 구분할 수 있게 남긴다.
	UE_LOG(LogWxGame, Log, TEXT("WxKillEnemies: 반경 %.0fm 안에서 %d 개 대상을 처치했다."), RadiusMeters, KillCount);
}

void UWxCheatManager::WxToggleAbility(FString AbilityTagName)
{
	const APlayerController* PC = GetOuterAPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
	if (!AbilitySystem)
	{
		return;
	}

	const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(FName(*AbilityTagName), false);
	if (!AbilityTag.IsValid())
	{
		UE_LOG(LogWxGame, Warning, TEXT("WxToggleAbility: '%s' 는 등록되지 않은 태그다."), *AbilityTagName);
		return;
	}

	// 순회 중에 제거하면 스펙 배열이 흔들리므로 찾기만 하고 빠져나온다.
	FGameplayAbilitySpecHandle FoundHandle;
	TSubclassOf<UGameplayAbility> FoundClass;
	for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasTag(AbilityTag))
		{
			FoundHandle = Spec.Handle;
			FoundClass = Spec.Ability->GetClass();
			break;
		}
	}

	if (FoundHandle.IsValid())
	{
		ClearedAbilities.Add(AbilityTag, FoundClass);
		AbilitySystem->ClearAbility(FoundHandle);
		UE_LOG(LogWxGame, Log, TEXT("WxToggleAbility: %s 를 걷었다."), *GetNameSafe(FoundClass));
		return;
	}

	const TSubclassOf<UGameplayAbility> ClearedClass = ClearedAbilities.FindRef(AbilityTag);
	if (!ClearedClass)
	{
		UE_LOG(LogWxGame, Warning, TEXT("WxToggleAbility: %s 를 단 어빌리티도, 이 치트가 걷어 둔 것도 없다."), *AbilityTagName);
		return;
	}

	FGameplayAbilitySpec Spec(ClearedClass, 1);
	AbilitySystem->GiveAbility(Spec);
	ClearedAbilities.Remove(AbilityTag);

	UE_LOG(LogWxGame, Log, TEXT("WxToggleAbility: %s 를 되돌렸다."), *GetNameSafe(ClearedClass));
}

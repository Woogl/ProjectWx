// Copyright Woogle. All Rights Reserved.

#include "Cheat/WxCheatManager.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/Effect/WxEffect_AddAttribute.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "WxGame.h"
#include "WxGameplayTags.h"

void UWxCheatManager::WxTeleport(float X, float Y, float Z)
{
	const APlayerController* PC = GetOuterAPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}

	const FVector TargetLocation(X, Y, Z);
	if (!Pawn->TeleportTo(TargetLocation, Pawn->GetActorRotation(), false, true))
	{
		UE_LOG(LogWxGame, Warning, TEXT("WxTeleport: 텔레포트 실패 (%s)"), *TargetLocation.ToString());
		return;
	}

	UE_LOG(LogWxGame, Log, TEXT("WxTeleport: %s"), *Pawn->GetActorLocation().ToString());
}

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
	// 컨텍스트를 비워 두면 Apply 가 자기 자신을 Instigator 로 실으므로 AI 보고까지 유효한 가해자를 갖는다.
	const float CurrentHP = AbilitySystem->GetNumericAttribute(UWxCombatAttributeSet::GetHPAttribute());
	UWxEffect_AddIncomingDamage::Apply(AbilitySystem, CurrentHP);
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
	
	UWxEffect_AddIncomingDamage::Apply(AbilitySystem, Amount);
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

		// 가해자를 플레이어로 남기려면 플레이어 ASC 에서 만든 컨텍스트를 실어야 한다.
		const float TargetHP = TargetAbilitySystem->GetNumericAttribute(UWxCombatAttributeSet::GetHPAttribute());
		UWxEffect_AddIncomingDamage::Apply(TargetAbilitySystem, TargetHP, AbilitySystem->MakeEffectContext());

		FString VictimNameStr = Actor->GetActorNameOrLabel();
		UE_LOG(LogWxGame, Log, TEXT("WxKillEnemies: %s 처치"), *VictimNameStr);
	}
}

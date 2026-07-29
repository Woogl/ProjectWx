// Copyright Woogle. All Rights Reserved.

#include "Cheat/WxCheatManager.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/Effect/WxEffect_Kill.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UWxCheatManager::WxKillPlayer()
{
	const APlayerController* PC = GetOuterAPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
	if (!AbilitySystem)
	{
		return;
	}

	// HP 를 직접 0 으로 쓰면 클램프만 되고 사망이 발동하지 않는다 — 사망 처리는 IncomingDamage 경로에서만 State.Dead 를 붙인다.
	// 그래서 현재 HP 를 그대로 대미지로 넣는 즉사 GE 를 태워 사망 어빌리티·사망 화면까지 실제 경로를 그대로 밟게 한다.
	// MakeEffectContext 가 자기 자신을 Instigator 로 실어 주므로 AI 보고까지 유효한 가해자를 갖는다.
	const FGameplayEffectSpecHandle SpecHandle = AbilitySystem->MakeOutgoingSpec(UWxEffect_Kill::StaticClass(), 1.f, AbilitySystem->MakeEffectContext());
	if (SpecHandle.IsValid())
	{
		AbilitySystem->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

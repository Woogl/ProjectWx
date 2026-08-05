// Copyright Woogle. All Rights Reserved.

#include "Cheat/WxCheatManager.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/Effect/WxEffect_Damage.h"
#include "AbilitySystem/Effect/WxEffect_Kill.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "WxGame.h"
#include "WxGameplayTags.h"

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

void UWxCheatManager::WxDamagePlayer(float Amount)
{
	// 0 이하는 Raw 대미지 모드 조건을 벗어나 Coeff 없는 ATK·DEF 공식으로 빠지므로 GE 만 헛돈다.
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

	// 표준 대미지 GE 를 SetByCaller.RawDamage 모드로 태운다. 이 모드는 ATK·DEF·계수·치명타를 우회해
	// 입력한 수치를 그대로 최종 대미지로 쓰므로, 치트가 요구한 만큼이 스탯·난수와 무관하게 들어간다.
	// 즉사 치트와 달리 ExecCalc 를 그대로 타므로 무적·가드·퍼펙트 가드 판정도 함께 검증된다.
	const FGameplayEffectSpecHandle SpecHandle = AbilitySystem->MakeOutgoingSpec(UWxEffect_Damage::StaticClass(), 1.f, AbilitySystem->MakeEffectContext());
	if (SpecHandle.IsValid())
	{
		FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
		Spec->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_RawDamage, Amount);

		// 피격 리액션 태그가 없으면 비가드 상태에서 HitReact 이벤트가 발송되지 않아 HP 만 깎인다.
		Spec->AddDynamicAssetTag(WxGameplayTags::Event_HitReact_Normal);

		AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec);
	}
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

	// 콘솔에서 미터로 입력받아 언리얼 단위로 환산한다.
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
		if (TargetAbilitySystem->HasMatchingGameplayTag(WxGameplayTags::State_Dead))
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

	// 화면 밖에서 벌어지는 일이라 결과가 보이지 않는다. 대상이 없었던 것과 치트가 안 먹은 것을 구분할 수 있게 남긴다.
	UE_LOG(LogWxGame, Log, TEXT("WxKillEnemies: 반경 %.0fm 안에서 %d 개 대상을 처치했다."), RadiusMeters, KillCount);
}

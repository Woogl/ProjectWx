// Copyright Woogle. All Rights Reserved.

#include "WxCombatLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GameplayAbility.h"
#include "GenericTeamAgentInterface.h"
#include "Damage/WxDamageEffectContext.h"
#include "Damage/WxDamageTableRow.h"
#include "Weapon/WxProjectileBase.h"

bool UWxCombatLibrary::IsHostile(const AActor* Source, const AActor* Target)
{
	const IGenericTeamAgentInterface* SourceTeamAgent = Cast<IGenericTeamAgentInterface>(Source);
	if (!SourceTeamAgent)
	{
		return false;
	}
	
	const IGenericTeamAgentInterface* TargetTeamAgent = Cast<IGenericTeamAgentInterface>(Target);
	if (!TargetTeamAgent)
	{
		return false;
	}
	
	return SourceTeamAgent->GetTeamAttitudeTowards(*Target) == ETeamAttitude::Hostile;
}

bool UWxCombatLibrary::ApplyDamage(AActor* Causer, const AActor* Target, const FDataTableRowHandle& DamageTableRow, const FHitResult& HitResult)
{
	if (!Causer || !Target)
	{
		return false;
	}

	AActor* Instigator = Causer;
	UAbilitySystemComponent* Source = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Instigator);
	if (!Source)
	{
		Instigator = Causer->GetOwner();
		Source = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Instigator);
	}
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!Source || !TargetASC)
	{
		return false;
	}
	// 적대는 GE 요건이 아니라 여기서 거른다. 무적 Immunity가 GE 요건보다 먼저 돌아 아군 공격에도 회피 통지가 나간다.
	if (!Source->IsOwnerActorAuthoritative() || !IsHostile(Source->GetAvatarActor(), TargetASC->GetAvatarActor()))
	{
		return false;
	}

	FGameplayEffectContextHandle Context(new FWxDamageEffectContext(*Source->MakeEffectContext().Get()));
	Context.AddInstigator(Instigator, Causer);
	const UGameplayAbility* SourceAbility = nullptr;
	float DamageLevel = 1.f;
	if (const AWxProjectileBase* Projectile = Cast<AWxProjectileBase>(Causer))
	{
		// 반사 후 출처는 현재 Owner를 따르되 발사 시 레벨은 유지한다.
		DamageLevel = Projectile->GetProjectileLevel();
	}
	else
	{
		SourceAbility = Source->GetAnimatingAbility();
		DamageLevel = SourceAbility ? SourceAbility->GetAbilityLevel() : 1.f;
	}
	Context.SetAbility(SourceAbility);
	Context.AddHitResult(HitResult);

	const FWxDamageTableRow* DamageRow = DamageTableRow.GetRow<FWxDamageTableRow>(ANSI_TO_TCHAR(__FUNCTION__));
	if (!DamageRow)
	{
		return false;
	}

	const FGameplayEffectSpecHandle DamageSpec = DamageRow->MakeDamageSpec(Source, Context, DamageLevel);
	if (!DamageSpec.IsValid())
	{
		return false;
	}

	// 무적이면 Invincible의 Immunity가 여기서 막는다. 피해와 Cue는 서버 판정을 따르며, 과거 활성화 키를 실으면 예측본 잔류나 소유 클라의 Cue 생략이 발생한다.
	return Source->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), TargetASC, FPredictionKey()).WasSuccessfullyApplied();
}

void UWxCombatLibrary::ApplyEffect(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, const UGameplayAbility* SourceAbility)
{
	if (!TargetASC || !EffectClass)
	{
		return;
	}

	const UGameplayEffect* CDO = EffectClass->GetDefaultObject<UGameplayEffect>();
	// 어빌리티 없이 걸리는 GE 도 GetAbilityLevel 의 기본 반환과 같은 레벨 1 로 만든다.
	const float Level = SourceAbility ? SourceAbility->GetAbilityLevel() : 1.f;
	FGameplayEffectSpec Spec(CDO, TargetASC->MakeEffectContext(), Level);
	// 발동의 활성화 키를 직접 실으면 창 밖의 권위 적용에도 키가 남아 소유 클라가 이 GE의 Cue를 건너뛴다.
	TargetASC->ApplyGameplayEffectSpecToSelf(Spec, TargetASC->GetPredictionKeyForNewAction());
}

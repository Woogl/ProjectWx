// Copyright Woogle. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/Effect/WxEffect_Damage.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "WxGameplayTags.h"

namespace
{
struct FWxDamageResponseObserver
{
	TArray<FGameplayEventData> Events;
	void HandleEvent(const FGameplayEventData* Payload);
};

void FWxDamageResponseObserver::HandleEvent(const FGameplayEventData* Payload)
{
	Events.Add(*Payload);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxDamageResponseTest, "Wx.Combat.DamageResponse.Execution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxDamageResponseTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	AActor* Owner = World->SpawnActor<AActor>();
	UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Owner);
	ASC->RegisterComponent();
	ASC->InitAbilityActorInfo(Owner, Owner);
	UWxCombatAttributeSet* Attributes = NewObject<UWxCombatAttributeSet>(Owner);
	ASC->AddAttributeSetSubobject(Attributes);
	Attributes->SetMaxHP(100.f);
	Attributes->SetHP(10.f);
	Attributes->SetMaxGP(50.f);

	FWxDamageResponseObserver Observer;
	const FGameplayTag Tags[] = { WxGameplayTags::Event_Groggy, WxGameplayTags::Event_Death,
		WxGameplayTags::Event_Hit, WxGameplayTags::Event_PerfectGuard };
	for (const FGameplayTag& Tag : Tags)
	{
		ASC->GenericGameplayEventCallbacks.FindOrAdd(Tag).AddRaw(&Observer, &FWxDamageResponseObserver::HandleEvent);
	}

	// 계산식과 독립적으로 실제 GE 실행 훅과 메타 속성 소비 순서를 검증한다.
	UWxEffect_Damage* Effect = NewObject<UWxEffect_Damage>();
	Effect->Executions.Reset();
	Effect->GameplayCues.Reset();
	FGameplayModifierInfo GPModifier;
	GPModifier.Attribute = UWxCombatAttributeSet::GetGPAttribute();
	GPModifier.ModifierOp = EGameplayModOp::Additive;
	GPModifier.ModifierMagnitude = FScalableFloat(50.f);
	Effect->Modifiers.Add(GPModifier);
	FGameplayModifierInfo DamageModifier;
	DamageModifier.Attribute = UWxCombatAttributeSet::GetIncomingDamageAttribute();
	DamageModifier.ModifierOp = EGameplayModOp::Additive;
	DamageModifier.ModifierMagnitude = FScalableFloat(50.f);
	Effect->Modifiers.Add(DamageModifier);
	FGameplayEffectSpec DamageSpec(Effect, ASC->MakeEffectContext(), 1.f);
	DamageSpec.AddDynamicAssetTag(WxGameplayTags::Damage_Attack);
	ASC->ApplyGameplayEffectSpecToSelf(DamageSpec);
	TestEqual(TEXT("HP clamps after overkill"), Attributes->GetHP(), 0.f);
	TestEqual(TEXT("Damage meta consumed"), Attributes->GetIncomingDamage(), 0.f);
	if (TestEqual(TEXT("Exactly one groggy, death and hit event"), Observer.Events.Num(), 3))
	{
		TestTrue(TEXT("Groggy precedes death"), Observer.Events[0].EventTag == WxGameplayTags::Event_Groggy);
		TestTrue(TEXT("Death precedes hit"), Observer.Events[1].EventTag == WxGameplayTags::Event_Death);
		TestTrue(TEXT("Hit after attribute processing"), Observer.Events[2].EventTag == WxGameplayTags::Event_Hit);
		TestEqual(TEXT("Overkill reports damage rather than HP delta"), Observer.Events[2].EventMagnitude, 50.f);
		TestTrue(TEXT("Target preserved"), Observer.Events[2].Target.Get() == Owner);
	}

	Observer.Events.Reset();
	Attributes->SetHP(100.f);
	Effect->Modifiers.RemoveAt(0);
	FGameplayEffectSpec UntaggedSpec(Effect, ASC->MakeEffectContext(), 1.f);
	ASC->ApplyGameplayEffectSpecToSelf(UntaggedSpec);
	TestEqual(TEXT("Untagged damage still changes HP"), Attributes->GetHP(), 50.f);
	TestEqual(TEXT("Untagged damage has no hit event"), Observer.Events.Num(), 0);

	Effect->Modifiers[0].Attribute = UWxCombatAttributeSet::GetIncomingReflectAttribute();
	Effect->Modifiers[0].ModifierMagnitude = FScalableFloat(0.f);
	FGameplayEffectSpec ReflectSpec(Effect, ASC->MakeEffectContext(), 1.f);
	ReflectSpec.AddDynamicAssetTag(WxGameplayTags::Damage_CanGuard);
	ASC->ApplyGameplayEffectSpecToSelf(ReflectSpec);
	TestEqual(TEXT("Perfect guard leaves HP intact"), Attributes->GetHP(), 50.f);
	TestEqual(TEXT("Reflect meta consumed"), Attributes->GetIncomingReflect(), 0.f);
	if (TestEqual(TEXT("Zero reflect still emits one event"), Observer.Events.Num(), 1))
	{
		TestTrue(TEXT("Perfect guard event"), Observer.Events[0].EventTag == WxGameplayTags::Event_PerfectGuard);
		TestEqual(TEXT("Zero reflect magnitude"), Observer.Events[0].EventMagnitude, 0.f);
		TestTrue(TEXT("Attack tags preserved"), Observer.Events[0].InstigatorTags.HasTag(WxGameplayTags::Damage_CanGuard));
	}

	AActor* Attacker = World->SpawnActor<AActor>();
	UAbilitySystemComponent* SourceASC = NewObject<UAbilitySystemComponent>(Attacker);
	SourceASC->RegisterComponent();
	SourceASC->InitAbilityActorInfo(Attacker, Attacker);
	UWxCombatAttributeSet* SourceAttributes = NewObject<UWxCombatAttributeSet>(Attacker);
	SourceASC->AddAttributeSetSubobject(SourceAttributes);
	SourceAttributes->SetMaxGP(50.f);
	FWxDamageResponseObserver SourceObserver;
	SourceASC->GenericGameplayEventCallbacks.FindOrAdd(WxGameplayTags::Event_Hit_Parry)
		.AddRaw(&SourceObserver, &FWxDamageResponseObserver::HandleEvent);
	SourceASC->GenericGameplayEventCallbacks.FindOrAdd(WxGameplayTags::Event_Groggy)
		.AddRaw(&SourceObserver, &FWxDamageResponseObserver::HandleEvent);

	struct FWxReflectCase
	{
		const TCHAR* Name;
		float Amount;
		bool bCanParry;
		bool bGroggy;
		float ExpectedGP;
		int32 ExpectedSourceEvents;
	};
	const FWxReflectCase Cases[] =
	{
		{TEXT("Parry reaches groggy"), 20.f, true, false, 50.f, 2},
		{TEXT("Groggy GP is not replenished"), 20.f, true, true, 40.f, 1},
		{TEXT("Non-parry guard does not reflect"), 20.f, false, false, 40.f, 0},
		{TEXT("Zero reflect still parries"), 0.f, true, false, 40.f, 1}
	};
	for (const FWxReflectCase& Case : Cases)
	{
		Observer.Events.Reset();
		SourceObserver.Events.Reset();
		SourceAttributes->SetGP(40.f);
		if (Case.bGroggy)
		{
			SourceASC->AddLooseGameplayTag(WxGameplayTags::Ability_Groggy);
		}
		Effect->Modifiers[0].ModifierMagnitude = FScalableFloat(Case.Amount);
		FGameplayEffectSpec ParrySpec(Effect, SourceASC->MakeEffectContext(), 1.f);
		TestTrue(TEXT("Source ASC available through effect context"), ParrySpec.GetContext().GetInstigatorAbilitySystemComponent() == SourceASC);
		ParrySpec.AddDynamicAssetTag(WxGameplayTags::Damage_CanGuard);
		if (Case.bCanParry)
		{
			ParrySpec.AddDynamicAssetTag(WxGameplayTags::Damage_CanParry);
		}
		ASC->ApplyGameplayEffectSpecToSelf(ParrySpec);
		TestEqual(Case.Name, SourceAttributes->GetGP(), Case.ExpectedGP);
		TestEqual(Case.Name, SourceObserver.Events.Num(), Case.ExpectedSourceEvents);
		TestEqual(TEXT("GuardReact event remains exactly once without active Guard ability"), Observer.Events.Num(), 1);
		TestEqual(TEXT("Perfect guard never damages defender HP"), Attributes->GetHP(), 50.f);
		TestEqual(TEXT("Nonzero reflect meta also resets"), Attributes->GetIncomingReflect(), 0.f);
		if (Case.bCanParry && !SourceObserver.Events.IsEmpty())
		{
			const FGameplayEventData& Parry = SourceObserver.Events.Last();
			TestTrue(TEXT("Parry follows reflected GP processing"), Parry.EventTag == WxGameplayTags::Event_Hit_Parry);
			TestTrue(TEXT("Parry instigator is defender avatar"), Parry.Instigator.Get() == Owner);
			TestTrue(TEXT("Parry target is attacker"), Parry.Target.Get() == Attacker);
		}
		if (Case.ExpectedSourceEvents == 2 && SourceObserver.Events.Num() == 2)
		{
			TestTrue(TEXT("Reflected GP triggers groggy first"), SourceObserver.Events[0].EventTag == WxGameplayTags::Event_Groggy);
			TestTrue(TEXT("Reflected GP context attributes groggy to defender"), SourceObserver.Events[0].Instigator.Get() == Owner);
		}
		if (Case.bGroggy)
		{
			SourceASC->RemoveLooseGameplayTag(WxGameplayTags::Ability_Groggy);
		}
	}
	SourceASC->GenericGameplayEventCallbacks.FindChecked(WxGameplayTags::Event_Hit_Parry).RemoveAll(&SourceObserver);
	SourceASC->GenericGameplayEventCallbacks.FindChecked(WxGameplayTags::Event_Groggy).RemoveAll(&SourceObserver);

	for (const FGameplayTag& Tag : Tags)
	{
		ASC->GenericGameplayEventCallbacks.FindChecked(Tag).RemoveAll(&Observer);
	}
	World->DestroyWorld(false);
	return true;
}

#endif


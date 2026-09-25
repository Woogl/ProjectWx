// Copyright Woogle. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/Effect/WxEffectComponent_UIData.h"
#include "AbilitySystemComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "MVVM/WxViewModel_Ability.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxViewModel_Character.h"
#include "MVVM/WxViewModel_Effect.h"
#include "MVVM/WxViewModelResolver_Ability.h"
#include "MVVM/WxViewModelResolver_AbilitySystem.h"
#include "UObject/UnrealType.h"
#include "WxGameplayTags.h"

namespace
{
	struct FWxPresentationTestWorld
	{
		FWxPresentationTestWorld();
		~FWxPresentationTestWorld();

		UWorld* World;
		UAbilitySystemComponent* ASC;
	};

	FWxPresentationTestWorld::FWxPresentationTestWorld()
	{
		const UWorld::InitializationValues Values = UWorld::InitializationValues()
			.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
		World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
		AActor* Owner = World->SpawnActor<AActor>();
		ASC = NewObject<UAbilitySystemComponent>(Owner);
		ASC->RegisterComponent();
		ASC->InitAbilityActorInfo(Owner, Owner);
		ASC->AddAttributeSetSubobject(NewObject<UWxCombatAttributeSet>(Owner));
	}

	FWxPresentationTestWorld::~FWxPresentationTestWorld()
	{
		World->DestroyWorld(false);
	}

	void SetAbilityPresentation(UWxAbilityBase& Ability, const TCHAR* Title, int32 Charges, float Cooldown)
	{
		FindFProperty<FTextProperty>(Ability.GetClass(), TEXT("Title"))->SetPropertyValue_InContainer(&Ability, FText::FromString(Title));
		FindFProperty<FIntProperty>(Ability.GetClass(), TEXT("MaxRecharges"))->SetPropertyValue_InContainer(&Ability, Charges);
		FindFProperty<FFloatProperty>(Ability.GetClass(), TEXT("CooldownTime"))->SetPropertyValue_InContainer(&Ability, Cooldown);
		FindFProperty<FSoftObjectProperty>(Ability.GetClass(), TEXT("Icon"))->SetPropertyValue_InContainer(&Ability, FSoftObjectPtr());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxAbilityPresentationTest, "Wx.UI.Presentation.AbilityRebind", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxAbilityPresentationTest::RunTest(const FString& Parameters)
{
	UClass* AbilityClass = LoadClass<UWxAbilityBase>(nullptr, TEXT("/Game/Character/Template/Shared/Abilities/Dodge/GA_Shared_Dodge.GA_Shared_Dodge_C"));
	if (!TestNotNull(TEXT("Dodge asset"), AbilityClass))
	{
		return false;
	}

	FWxPresentationTestWorld Fixture;
	const FGameplayTagContainer SlotTags(WxGameplayTags::Ability_Dodge);
	const FGameplayAbilitySpecHandle FirstHandle = Fixture.ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
	UWxAbilityBase* FirstAbility = CastChecked<UWxAbilityBase>(Fixture.ASC->FindAbilitySpecFromHandle(FirstHandle)->GetPrimaryInstance());
	SetAbilityPresentation(*FirstAbility, TEXT("First"), 3, 3.f);
	UWxViewModel_Ability* ViewModel = UWxViewModelResolver_Ability::GetOrCreate(Fixture.ASC, SlotTags);
	TestEqual(TEXT("Initial presentation precedes charge calculation"), ViewModel->GetCurrentCharges(), 3);
	TestEqual(TEXT("Initial title"), ViewModel->GetTitle().ToString(), FString(TEXT("First")));
	TestTrue(TEXT("Shared slot is retained"), ViewModel == UWxViewModelResolver_Ability::GetOrCreate(Fixture.ASC, SlotTags));

	UGameplayEffect* Cooldown = NewObject<UGameplayEffect>();
	Cooldown->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	Cooldown->DurationMagnitude = FScalableFloat(8.f);
	FGameplayEffectSpec CooldownSpec(Cooldown, Fixture.ASC->MakeEffectContext(), 1.f);
	CooldownSpec.DynamicGrantedTags.AppendTags(*FirstAbility->GetCooldownTags());
	const FActiveGameplayEffectHandle CooldownHandle = Fixture.ASC->ApplyGameplayEffectSpecToSelf(CooldownSpec);
	TestTrue(TEXT("Cooldown applied"), CooldownHandle.IsValid());
	// 슬롯은 GE 추가 콜백이 아닌 다음 월드 타이머 틱에서 활성 목록을 읽는다.
	Fixture.World->GetTimerManager().Tick(1.f / 60.f);
	TestEqual(TEXT("One consumed charge"), ViewModel->GetCurrentCharges(), 2);
	TestEqual(TEXT("Period comes from ability, not queued GE duration"), ViewModel->GetCooldownDuration(), 3.f);

	Fixture.ASC->ClearAbility(FirstHandle);
	ViewModel->RefreshBoundAbility();
	TestTrue(TEXT("Removed ability clears title"), ViewModel->GetTitle().IsEmpty());
	TestEqual(TEXT("Removed ability clears charges"), ViewModel->GetCurrentCharges(), 0);
	TestEqual(TEXT("Removed ability clears period"), ViewModel->GetCooldownDuration(), 0.f);
	Fixture.ASC->RemoveActiveGameplayEffect(CooldownHandle);

	const FGameplayAbilitySpecHandle SecondHandle = Fixture.ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
	UWxAbilityBase* SecondAbility = CastChecked<UWxAbilityBase>(Fixture.ASC->FindAbilitySpecFromHandle(SecondHandle)->GetPrimaryInstance());
	SetAbilityPresentation(*SecondAbility, TEXT("Second"), 1, 0.f);
	ViewModel->RefreshBoundAbility();
	TestEqual(TEXT("Rebound title"), ViewModel->GetTitle().ToString(), FString(TEXT("Second")));
	TestEqual(TEXT("Rebound charge limit"), ViewModel->GetMaxRecharges(), 1);
	TestEqual(TEXT("Rebound available charge"), ViewModel->GetCurrentCharges(), 1);
	ViewModel->Deinitialize();
	UWxViewModel_AbilitySystem::GetOrCreate(Fixture.ASC)->Deinitialize();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxEffectPresentationTest, "Wx.UI.Presentation.EffectLateConnection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxEffectPresentationTest::RunTest(const FString& Parameters)
{
	FWxPresentationTestWorld Fixture;
	UWxViewModel_AbilitySystem* ViewModel = UWxViewModel_AbilitySystem::GetOrCreate(Fixture.ASC);
	TestEqual(TEXT("Unconfigured effect list"), ViewModel->GetActiveEffectViewModels().Num(), 0);

	UGameplayEffect* Effect = NewObject<UGameplayEffect>();
	Effect->DurationPolicy = EGameplayEffectDurationType::Infinite;
	UWxEffectComponent_UIData& Data = Effect->AddComponent<UWxEffectComponent_UIData>();
	UTexture2D* Icon = NewObject<UTexture2D>();
	FindFProperty<FTextProperty>(Data.GetClass(), TEXT("Title"))->SetPropertyValue_InContainer(&Data, FText::FromString(TEXT("Buff")));
	FindFProperty<FSoftObjectProperty>(Data.GetClass(), TEXT("Icon"))->SetPropertyValue_InContainer(&Data, FSoftObjectPtr(Icon));
	const FActiveGameplayEffectHandle Handle = Fixture.ASC->ApplyGameplayEffectToSelf(Effect, 1.f, Fixture.ASC->MakeEffectContext());
	TestTrue(TEXT("Effect applied before connection"), Handle.IsValid());
	TestTrue(TEXT("Resolver retains shared VM"), ViewModel == UWxViewModelResolver_AbilitySystem::GetOrCreate(Fixture.ASC));
	if (TestEqual(TEXT("Late connection populates current effect"), ViewModel->GetActiveEffectViewModels().Num(), 1))
	{
		UWxViewModel_Effect* EffectVM = ViewModel->GetActiveEffectViewModels()[0];
		TestEqual(TEXT("Effect title"), EffectVM->GetTitle().ToString(), FString(TEXT("Buff")));
		TestTrue(TEXT("Effect icon"), EffectVM->GetIcon() == Icon);
		UWxViewModelResolver_AbilitySystem::GetOrCreate(Fixture.ASC);
		TestTrue(TEXT("Reconnection preserves child"), ViewModel->GetActiveEffectViewModels()[0] == EffectVM);
		Fixture.ASC->RemoveActiveGameplayEffect(Handle);
		TestEqual(TEXT("Removal updates list"), ViewModel->GetActiveEffectViewModels().Num(), 0);
		TestFalse(TEXT("Removal detaches child"), EffectVM->GetBoundHandle().IsValid());
		TestNull(TEXT("Removal clears icon"), EffectVM->GetIcon());
	}

	UGameplayEffect* HiddenEffect = NewObject<UGameplayEffect>();
	HiddenEffect->DurationPolicy = EGameplayEffectDurationType::Infinite;
	HiddenEffect->AddComponent<UWxEffectComponent_UIData>();
	Fixture.ASC->ApplyGameplayEffectToSelf(HiddenEffect, 1.f, Fixture.ASC->MakeEffectContext());
	TestEqual(TEXT("Iconless effect stays hidden"), ViewModel->GetActiveEffectViewModels().Num(), 0);
	ViewModel->Deinitialize();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxCharacterPresentationTest, "Wx.UI.Presentation.CharacterSharing", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxCharacterPresentationTest::RunTest(const FString& Parameters)
{
	FWxPresentationTestWorld Fixture;
	UWxViewModel_AbilitySystem* AbilitySystem = UWxViewModelResolver_AbilitySystem::GetOrCreate(Fixture.ASC);
	UTexture2D* Portrait = NewObject<UTexture2D>();
	UWxViewModel_Character* ViewModel = UWxViewModel_Character::GetOrCreate(AbilitySystem, FText::FromString(TEXT("Character")), Portrait);
	TestTrue(TEXT("Character shares by ability system"), ViewModel == UWxViewModel_Character::GetOrCreate(AbilitySystem, FText::FromString(TEXT("Ignored")), nullptr));
	TestEqual(TEXT("Shared lookup preserves name"), ViewModel->CharacterName.ToString(), FString(TEXT("Character")));
	TestTrue(TEXT("Shared lookup preserves portrait"), ViewModel->Portrait == Portrait);
	ViewModel->Initialize(AbilitySystem, ViewModel->CharacterName, Portrait);
	TestEqual(TEXT("Reinitialize accepts its own display value"), ViewModel->CharacterName.ToString(), FString(TEXT("Character")));
	ViewModel->Deinitialize();
	TestNull(TEXT("Character releases shared child"), ViewModel->AbilitySystem.Get());
	TestTrue(TEXT("Character clears name"), ViewModel->CharacterName.IsEmpty());
	TestNull(TEXT("Character clears portrait"), ViewModel->Portrait.Get());
	TestTrue(TEXT("Character teardown leaves AS VM alive"), AbilitySystem == UWxViewModelResolver_AbilitySystem::GetOrCreate(Fixture.ASC));
	AbilitySystem->Deinitialize();
	return true;
}

#endif

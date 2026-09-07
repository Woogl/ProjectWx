// Copyright Woogle. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "MVVM/WxViewModel_Ability.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxMVVMConversionLibrary.h"
#include "WxUIData.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxAbilityWidgetTest, "Wx.MVVM.Ability.SeparateWidgets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxAbilityWidgetTest::RunTest(const FString& Parameters)
{
	UClass* GroundClass = LoadClass<UGameplayAbility>(nullptr, TEXT("/Game/AbilitySystem/Ability/GA_Attack_Light.GA_Attack_Light_C"));
	UClass* AirClass = LoadClass<UGameplayAbility>(nullptr, TEXT("/Game/AbilitySystem/Ability/GA_Attack_Air.GA_Attack_Air_C"));
	if (!TestNotNull(TEXT("Ground ability"), GroundClass) || !TestNotNull(TEXT("Air ability"), AirClass))
	{
		return false;
	}
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	AActor* Owner = World->SpawnActor<AActor>();
	UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Owner);
	ASC->RegisterComponent();
	ASC->InitAbilityActorInfo(Owner, Owner);
	ASC->AddAttributeSetSubobject(NewObject<UWxCombatAttributeSet>(Owner));
	const FGameplayAbilitySpecHandle GroundHandle = ASC->GiveAbility(FGameplayAbilitySpec(GroundClass));
	const FGameplayAbilitySpecHandle AirHandle = ASC->GiveAbility(FGameplayAbilitySpec(AirClass));
	const FGameplayTag InAir = FGameplayTag::RequestGameplayTag(TEXT("Movement.InAir"));
	const FGameplayTagContainer GroundTags(FGameplayTag::RequestGameplayTag(TEXT("Ability.Attack.Light")));
	const FGameplayTagContainer AirTags(FGameplayTag::RequestGameplayTag(TEXT("Ability.Attack.Air")));
	UObject* GroundIcon = CastChecked<IWxUIData>(GroundClass->GetDefaultObject())->GetIcon().LoadSynchronous();
	UObject* AirIcon = CastChecked<IWxUIData>(AirClass->GetDefaultObject())->GetIcon().LoadSynchronous();
	UWxViewModel_AbilitySystem* SystemVM = UWxViewModel_AbilitySystem::GetOrCreate(ASC);
	UWxViewModel_Ability* GroundVM = SystemVM->GetOrCreateAbilityViewModel(GroundTags);
	UWxViewModel_Ability* AirVM = SystemVM->GetOrCreateAbilityViewModel(AirTags);
	TestTrue(TEXT("Two independent ability view models"), GroundVM != AirVM);
	TestTrue(TEXT("Same ability shares view model"), GroundVM == SystemVM->GetOrCreateAbilityViewModel(GroundTags));
	TestTrue(TEXT("Ground icon bound"), GroundIcon && GroundVM->GetIcon() == GroundIcon);
	TestTrue(TEXT("Air icon bound even while grounded"), AirIcon && AirVM->GetIcon() == AirIcon);

	for (const int32 InAirCount : { 0, 1, 0 })
	{
		ASC->SetLooseGameplayTagCount(InAir, InAirCount);
		FGameplayTagContainer OwnedTags;
		ASC->GetOwnedGameplayTags(OwnedTags);
		const ESlateVisibility GroundVisibility = UWxMVVMConversionLibrary::Conv_GameplayTagToSlateVisibility(OwnedTags, InAir, ESlateVisibility::Collapsed, ESlateVisibility::SelfHitTestInvisible);
		const ESlateVisibility AirVisibility = UWxMVVMConversionLibrary::Conv_GameplayTagToSlateVisibility(OwnedTags, InAir);
		TestEqual(TEXT("Ground visibility follows movement"), GroundVisibility, InAirCount ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
		TestEqual(TEXT("Air visibility follows movement"), AirVisibility, InAirCount ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		GroundVM->RefreshBoundAbility();
		AirVM->RefreshBoundAbility();
		TestTrue(TEXT("State never replaces grounded ability"), GroundVM->GetIcon() == GroundIcon);
		TestTrue(TEXT("State never replaces air ability"), AirVM->GetIcon() == AirIcon);
	}
	ASC->ClearAbility(AirHandle);
	AirVM->RefreshBoundAbility();
	TestNull(TEXT("Revoked ability clears its own display"), AirVM->GetIcon());
	TestTrue(TEXT("Other ability unaffected"), GroundVM->GetIcon() == GroundIcon);
	GroundVM->Deinitialize();
	AirVM->Deinitialize();
	SystemVM->Deinitialize();
	ASC->ClearAbility(GroundHandle);
	World->DestroyWorld(false);
	return true;
}

#endif

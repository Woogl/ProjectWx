// Copyright Woogle. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/WxEnemyCharacter.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxViewModel_BossDisplay.h"
#include "MVVM/WxViewModel_Character.h"
#include "MVVM/WxViewModelResolver_BossCharacter.h"
#include "UObject/StrongObjectPtr.h"
#include "WxGameplayTags.h"

namespace WxBossDisplayTests
{
	struct FWxTestWorld
	{
		FWxTestWorld();
		~FWxTestWorld();
		UWorld* World;
	};

	FWxTestWorld::FWxTestWorld()
	{
		const UWorld::InitializationValues Values = UWorld::InitializationValues()
			.AllowAudioPlayback(false).CreatePhysicsScene(true).RequiresHitProxies(false)
			.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
		World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	}

	FWxTestWorld::~FWxTestWorld()
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	}

	AWxEnemyCharacter* SpawnBoss(UWorld* World, UClass* BossClass)
	{
		FActorSpawnParameters Parameters;
		// 표시 수명 테스트는 전투 AI나 BP Construction Script 실행을 필요로 하지 않는다.
		Parameters.bDeferConstruction = true;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World->SpawnActor<AWxEnemyCharacter>(BossClass, FTransform::Identity, Parameters);
	}

	void PublishEngagement(AWxEnemyCharacter* Boss, bool bEngaged)
	{
		Boss->GetAbilitySystemComponent()->SetLooseGameplayTagCount(WxGameplayTags::State_Engaged, bEngaged ? 1 : 0);
		AWxEnemyCharacter::OnAnyBossEngagementChanged.Broadcast(Boss, bEngaged);
	}

	UObject* GetDisplayedSource(const UWxViewModel_BossDisplay* Display)
	{
		return Display->Character->AbilitySystem ? Display->Character->AbilitySystem->GetOuter() : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxBossDisplayLifecycleTest, "Wx.MVVM.BossDisplay.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxBossDisplayLifecycleTest::RunTest(const FString& Parameters)
{
	using namespace WxBossDisplayTests;

	UClass* BossClass = LoadClass<AWxEnemyCharacter>(nullptr, TEXT("/Game/Character/Boss/BP_Boss.BP_Boss_C"));
	UClass* WidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/Widget/WBP_Nameplate_Boss.WBP_Nameplate_Boss_C"));
	if (!TestNotNull(TEXT("Boss fixture"), BossClass) || !TestNotNull(TEXT("Boss widget fixture"), WidgetClass))
	{
		return false;
	}

	FWxTestWorld Local;
	FWxTestWorld Other;
	AWxEnemyCharacter* BossA = SpawnBoss(Local.World, BossClass);
	AWxEnemyCharacter* BossB = SpawnBoss(Local.World, BossClass);
	AWxEnemyCharacter* ForeignBoss = SpawnBoss(Other.World, BossClass);
	if (!TestNotNull(TEXT("Boss A"), BossA) || !TestNotNull(TEXT("Boss B"), BossB) || !TestNotNull(TEXT("Other world boss"), ForeignBoss))
	{
		return false;
	}
	TestTrue(TEXT("Fixture is a boss for late-widget discovery"), BossA->IsBoss());

	TStrongObjectPtr<UWxViewModelResolver_BossCharacter> Resolver(NewObject<UWxViewModelResolver_BossCharacter>());
	TStrongObjectPtr<UUserWidget> WidgetA(NewObject<UUserWidget>(Local.World, WidgetClass));
	TStrongObjectPtr<UUserWidget> WidgetB(NewObject<UUserWidget>(Local.World, WidgetClass));
	TStrongObjectPtr<UWxViewModel_BossDisplay> DisplayA(Cast<UWxViewModel_BossDisplay>(Resolver->CreateInstance(UWxViewModel_BossDisplay::StaticClass(), WidgetA.Get(), nullptr)));
	TStrongObjectPtr<UWxViewModel_BossDisplay> DisplayB(Cast<UWxViewModel_BossDisplay>(Resolver->CreateInstance(UWxViewModel_BossDisplay::StaticClass(), WidgetB.Get(), nullptr)));
	if (!TestNotNull(TEXT("Display A"), DisplayA.Get()) || !TestNotNull(TEXT("Display B"), DisplayB.Get()))
	{
		return false;
	}

	TestTrue(TEXT("Each widget owns its display state"), DisplayA.Get() != DisplayB.Get() && DisplayA->Character != DisplayB->Character);
	TestNull(TEXT("No boss starts empty"), GetDisplayedSource(DisplayB.Get()));
	PublishEngagement(ForeignBoss, true);
	TestNull(TEXT("Other world is ignored"), GetDisplayedSource(DisplayB.Get()));
	PublishEngagement(BossA, true);
	TestEqual(TEXT("First widget observes A"), GetDisplayedSource(DisplayA.Get()), static_cast<UObject*>(BossA->GetAbilitySystemComponent()));
	TestEqual(TEXT("Second widget observes A"), GetDisplayedSource(DisplayB.Get()), static_cast<UObject*>(BossA->GetAbilitySystemComponent()));
	TestTrue(TEXT("ASC data remains shared"), DisplayA->Character->AbilitySystem == DisplayB->Character->AbilitySystem);

	PublishEngagement(BossB, true);
	PublishEngagement(BossB, true);
	TestEqual(TEXT("A stays selected when B joins twice"), GetDisplayedSource(DisplayB.Get()), static_cast<UObject*>(BossA->GetAbilitySystemComponent()));
	PublishEngagement(BossB, false);
	TestEqual(TEXT("Non-selected B leaving preserves A"), GetDisplayedSource(DisplayB.Get()), static_cast<UObject*>(BossA->GetAbilitySystemComponent()));
	PublishEngagement(BossB, true);
	Resolver->DestroyInstance(DisplayA.Get(), nullptr);
	TestFalse(TEXT("Released display unsubscribes immediately"), AWxEnemyCharacter::OnAnyBossEngagementChanged.IsBoundToObject(DisplayA.Get()));
	TestTrue(TEXT("Remaining display keeps its subscription"), AWxEnemyCharacter::OnAnyBossEngagementChanged.IsBoundToObject(DisplayB.Get()));
	PublishEngagement(BossA, false);
	TestEqual(TEXT("Remaining widget switches to B"), GetDisplayedSource(DisplayB.Get()), static_cast<UObject*>(BossB->GetAbilitySystemComponent()));
	TestNull(TEXT("Released widget does not reconnect"), GetDisplayedSource(DisplayA.Get()));
	PublishEngagement(BossB, false);
	TestNull(TEXT("Last boss leaving clears display"), GetDisplayedSource(DisplayB.Get()));

	PublishEngagement(BossA, true);
	PublishEngagement(BossB, true);
	TStrongObjectPtr<UWxViewModel_BossDisplay> LateDisplay(Cast<UWxViewModel_BossDisplay>(Resolver->CreateInstance(UWxViewModel_BossDisplay::StaticClass(), WidgetA.Get(), nullptr)));
	if (!TestNotNull(TEXT("Late display"), LateDisplay.Get()))
	{
		Resolver->DestroyInstance(DisplayB.Get(), nullptr);
		return false;
	}
	UObject* LateSource = GetDisplayedSource(LateDisplay.Get());
	TestNotNull(TEXT("Late widget seeds an engaged boss"), LateSource);
	AWxEnemyCharacter* FirstBoss = LateSource == BossA->GetAbilitySystemComponent() ? BossA : BossB;
	AWxEnemyCharacter* RemainingBoss = FirstBoss == BossA ? BossB : BossA;
	PublishEngagement(FirstBoss, false);
	TestEqual(TEXT("Late widget seeds every boss, not just the first"), GetDisplayedSource(LateDisplay.Get()), static_cast<UObject*>(RemainingBoss->GetAbilitySystemComponent()));
	// 실제 EndPlay의 발행 경로도 검증한다. 테스트 월드는 BeginPlay 전이므로 직접 호출한다.
	RemainingBoss->EndPlay(EEndPlayReason::RemovedFromWorld);
	TestNull(TEXT("Actor EndPlay clears late display"), GetDisplayedSource(LateDisplay.Get()));
	TestNull(TEXT("Actor EndPlay clears remaining widget"), GetDisplayedSource(DisplayB.Get()));

	Resolver->DestroyInstance(LateDisplay.Get(), nullptr);
	Resolver->DestroyInstance(DisplayB.Get(), nullptr);
	Resolver->DestroyInstance(DisplayB.Get(), nullptr);
	TestFalse(TEXT("Repeated release leaves no subscription"), AWxEnemyCharacter::OnAnyBossEngagementChanged.IsBoundToObject(DisplayB.Get()));
	TestNull(TEXT("Missing widget is rejected"), Resolver->CreateInstance(UWxViewModel_BossDisplay::StaticClass(), nullptr, nullptr));
	TestNull(TEXT("Unrelated model type is rejected"), Resolver->CreateInstance(UWxViewModel_Character::StaticClass(), WidgetB.Get(), nullptr));
	return true;
}

#endif

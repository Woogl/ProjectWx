// Copyright Woogle. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/UserWidget.h"
#include "Character/WxEnemyCharacter.h"
#include "Component/WxNameplateComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "MVVM/WxViewModel_Character.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "View/MVVMView.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxNameplateViewModelTest, "Wx.UI.Nameplate.ManualViewModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxNameplateViewModelTest::RunTest(const FString& Parameters)
{
	UClass* WidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/Widget/WBP_Nameplate_Enemy.WBP_Nameplate_Enemy_C"));
	UClass* EnemyClass = LoadClass<AWxEnemyCharacter>(nullptr, TEXT("/Game/Character/Sandbag/BP_Sandbag.BP_Sandbag_C"));
	if (!TestNotNull(TEXT("Enemy widget loads without resolver"), WidgetClass)
		|| !TestNotNull(TEXT("Concrete enemy class loads"), EnemyClass))
	{
		return false;
	}

	const UWorld::InitializationValues Initialization = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false)
		.RequiresHitProxies(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Initialization);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);

	// 캐릭터의 전투·AI BeginPlay 없이 실제 ASC와 AttributeSet, Nameplate를 사용한다.
	AWxEnemyCharacter* Owner = World->SpawnActorDeferred<AWxEnemyCharacter>(EnemyClass, FTransform::Identity);
	if (!TestNotNull(TEXT("Enemy spawned"), Owner))
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		return false;
	}
	UWxNameplateComponent* Component = Owner->FindComponentByClass<UWxNameplateComponent>();
	if (!TestNotNull(TEXT("Enemy has nameplate"), Component))
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		return false;
	}
	Component->SetWidgetClass(WidgetClass);
	Component->InitWidget();
	UUserWidget* FirstWidget = Component->GetWidget();
	if (TestNotNull(TEXT("Initial widget created"), FirstWidget))
	{
		UMVVMView* FirstView = FirstWidget->GetExtension<UMVVMView>();
		UWxViewModel_Character* Shared = UWxViewModel_Character::GetOrCreate(Owner->GetAbilitySystemComponent(), Owner);
		if (TestNotNull(TEXT("Actual widget has MVVM View"), FirstView))
		{
			TestEqual(TEXT("Initial widget receives shared VM"), FirstView->GetViewModel(TEXT("WxViewModel_Character")).GetObject(), static_cast<UObject*>(Shared));
			Component->InitWidget();
			TestEqual(TEXT("Repeated InitWidget preserves widget"), Component->GetWidget(), FirstWidget);

			UUserWidget* Replacement = CreateWidget<UUserWidget>(World, WidgetClass);
			Component->SetWidget(Replacement);
			UMVVMView* ReplacementView = Replacement->GetExtension<UMVVMView>();
			TestEqual(TEXT("Replacement receives same shared VM"), ReplacementView->GetViewModel(TEXT("WxViewModel_Character")).GetObject(), static_cast<UObject*>(Shared));
			TSharedPtr<SWidget> SlateWidget = Replacement->TakeWidget();
			SlateWidget.Reset();
			SlateWidget = Replacement->TakeWidget();
			TestEqual(TEXT("View reconstruction preserves manual source"), ReplacementView->GetViewModel(TEXT("WxViewModel_Character")).GetObject(), static_cast<UObject*>(Shared));
			SlateWidget.Reset();

			Component->SetWidget(nullptr);
			TestNull(TEXT("Widget cleared"), Component->GetWidget());
			TestNotNull(TEXT("Clearing widget does not deinitialize shared VM"), Shared->AbilitySystem.Get());
			TestEqual(TEXT("Other view retains shared VM"), FirstView->GetViewModel(TEXT("WxViewModel_Character")).GetObject(), static_cast<UObject*>(Shared));
			Component->InitWidget();
			UMVVMView* RecreatedView = Component->GetWidget()->GetExtension<UMVVMView>();
			TestEqual(TEXT("Recreated widget receives shared VM"), RecreatedView->GetViewModel(TEXT("WxViewModel_Character")).GetObject(), static_cast<UObject*>(Shared));
		}
	}

	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return !HasAnyErrors();
}

#endif

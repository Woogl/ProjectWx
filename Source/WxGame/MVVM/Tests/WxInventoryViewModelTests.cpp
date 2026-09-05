// Copyright Woogle. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/WxInventoryComponent.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"
#include "Items/WxItemInstance.h"
#include "Misc/AutomationTest.h"
#include "MVVM/WxViewModel_Inventory.h"
#include "MVVM/WxViewModel_Item.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

namespace WxInventoryViewModelTests
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

	UWxInventoryComponent* CreateInventory(APlayerController* PC)
	{
		UWxInventoryComponent* Inventory = NewObject<UWxInventoryComponent>(PC);
		Inventory->RegisterComponent();
		return Inventory;
	}

	struct FWxNotifications
	{
		void HandleChanged(UObject* Object, UE::FieldNotification::FFieldId Field);
		void HandleReady(UWxInventoryComponent* Inventory);
		int32 Count = 0;
		int32 ReadyCount = 0;
	};

	void FWxNotifications::HandleChanged(UObject* Object, UE::FieldNotification::FFieldId Field)
	{
		++Count;
	}

	void FWxNotifications::HandleReady(UWxInventoryComponent* Inventory)
	{
		++ReadyCount;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxInventoryViewModelLifecycleTest, "Wx.MVVM.Inventory.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxInventoryViewModelLifecycleTest::RunTest(const FString& Parameters)
{
	using namespace WxInventoryViewModelTests;
	FWxTestWorld Local;
	APlayerController* PC = Local.World->SpawnActor<APlayerController>();
	APlayerController* OtherPC = Local.World->SpawnActor<APlayerController>();
	TStrongObjectPtr<UWxItemDefinition> Def(NewObject<UWxItemDefinition>());
	Def->DisplayName = FText::FromString(TEXT("Test potion"));
	Def->Fragments.Add(NewObject<UWxItemFragment_Stackable>(Def.Get()));
	TStrongObjectPtr<UObject> DefaultIcon(NewObject<UTexture2D>());
	TStrongObjectPtr<UObject> ChargedIcon(NewObject<UTexture2D>());
	Def->Icon = DefaultIcon.Get();
	UWxItemFragment_Charges* Charges = NewObject<UWxItemFragment_Charges>(Def.Get());
	Charges->ChargeIcons.SetNum(Charges->MaxCharges + 1);
	Charges->ChargeIcons[Charges->MaxCharges] = ChargedIcon.Get();
	Def->Fragments.Add(Charges);
	TStrongObjectPtr<UWxViewModel_Item> Fixed(NewObject<UWxViewModel_Item>());
	TStrongObjectPtr<UWxViewModel_Item> Second(NewObject<UWxViewModel_Item>());
	TStrongObjectPtr<UWxViewModel_Inventory> List(NewObject<UWxViewModel_Inventory>());
	Fixed->StartObserving(PC, Def.Get());
	List->StartObserving(PC);
	TestFalse(TEXT("Missing source is unavailable"), Fixed->bIsInventoryAvailable);
	TestEqual(TEXT("Static name before inventory"), Fixed->DisplayName.ToString(), Def->DisplayName.ToString());
	TestEqual(TEXT("Default icon before inventory"), Fixed->Icon.Get(), DefaultIcon.Get());
	TestFalse(TEXT("Unavailable item cannot be used"), Fixed->RequestUseConsumable());
	UWxInventoryComponent* Other = CreateInventory(OtherPC);
	Other->BeginPlay();
	Other->AddItemDefinition(Def.Get(), 9);
	TestFalse(TEXT("Other owner ignored"), Fixed->bIsInventoryAvailable);
	UWxInventoryComponent* Inventory = CreateInventory(PC);
	Second->StartObserving(PC, Def.Get());
	TestFalse(TEXT("Initial lookup rejects inventory before BeginPlay"), Second->bIsInventoryAvailable);
	Inventory->BeginPlay();
	TestTrue(TEXT("Late inventory connects fixed item"), Fixed->bIsInventoryAvailable);
	TestTrue(TEXT("Late inventory connects list"), List->bIsInventoryAvailable);
	TestEqual(TEXT("Available empty is zero"), Fixed->TotalCount, 0);
	Inventory->AddItemDefinition(Def.Get(), 3);
	TestEqual(TEXT("Late data updates count"), Fixed->TotalCount, 3);
	TestEqual(TEXT("Late data updates charges"), Fixed->CurrentCharges, Charges->MaxCharges);
	TestEqual(TEXT("Charge icon replaces default"), Fixed->Icon.Get(), ChargedIcon.Get());
	TestEqual(TEXT("Late data creates slot"), List->AllItems.Num(), 1);
	Second->StartObserving(PC, Def.Get());
	TestEqual(TEXT("Source before VM uses snapshot"), Second->TotalCount, 3);
	Second->StartObserving(PC, Def.Get());
	UWxViewModel_Item* Toast = List->LastAcquiredItem;
	Inventory->NotifyContentsChangedFromReplication();
	TestEqual(TEXT("Snapshot does not fabricate acquisition"), List->LastAcquiredItem.Get(), Toast);
	FWxNotifications Notifications;
	const FDelegateHandle Handle = Fixed->AddFieldValueChangedDelegate(UWxViewModel_Item::FFieldNotificationClassDescriptor::TotalCount,
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateRaw(&Notifications, &FWxNotifications::HandleChanged));
	Inventory->EndPlay(EEndPlayReason::RemovedFromWorld);
	Second->StartObserving(PC, Def.Get());
	TestFalse(TEXT("Initial lookup rejects ended inventory still owned"), Second->bIsInventoryAvailable);
	TestFalse(TEXT("EndPlay disconnects"), Fixed->bIsInventoryAvailable);
	TestEqual(TEXT("Removed source clears count"), Fixed->TotalCount, 0);
	TestEqual(TEXT("Removed source clears charges"), Fixed->CurrentCharges, 0);
	TestEqual(TEXT("Removed source restores default icon"), Fixed->Icon.Get(), DefaultIcon.Get());
	TestTrue(TEXT("Clear notifies bindings"), Notifications.Count > 0);
	Fixed->RemoveFieldValueChangedDelegate(UWxViewModel_Item::FFieldNotificationClassDescriptor::TotalCount, Handle);
	TestEqual(TEXT("Definition survives removal"), Fixed->DisplayName.ToString(), Def->DisplayName.ToString());
	TestEqual(TEXT("List clears on removal"), List->AllItems.Num(), 0);
	UWxInventoryComponent* Replacement = CreateInventory(PC);
	Replacement->AddItemDefinition(Def.Get(), 5);
	Replacement->BeginPlay();
	TestEqual(TEXT("Replacement discovered past old component"), Fixed->TotalCount, 5);
	TestEqual(TEXT("Independent observer reconnects"), Second->TotalCount, 5);
	TestEqual(TEXT("List reconnects"), List->AllItems.Num(), 1);
	TestNull(TEXT("Reconnect snapshot has no acquisition"), List->LastAcquiredItem.Get());
	TStrongObjectPtr<UWxViewModelResolver_Item> Resolver(NewObject<UWxViewModelResolver_Item>());
	Resolver->DestroyInstance(Fixed.Get(), nullptr);
	Replacement->AddItemDefinition(Def.Get(), 2);
	TestEqual(TEXT("Released view stays disconnected"), Fixed->TotalCount, 0);
	TestEqual(TEXT("Another view retains its subscription"), Second->TotalCount, 7);
	const FDelegateHandle ReadyHandle = UWxInventoryComponent::OnAnyInventoryReady.AddRaw(&Notifications, &FWxNotifications::HandleReady);
	Replacement->UnregisterComponent();
	TestTrue(TEXT("Registration changes do not end inventory lifetime"), Second->bIsInventoryAvailable);
	Replacement->RegisterComponent();
	TestEqual(TEXT("Re-register reconnects snapshot"), Second->TotalCount, 7);
	TestEqual(TEXT("Re-register does not repeat initial grant signal"), Notifications.ReadyCount, 0);
	UWxInventoryComponent::OnAnyInventoryReady.Remove(ReadyHandle);
	List->Deinitialize();
	Second->Deinitialize();
	Replacement->DestroyComponent();
	Inventory->DestroyComponent();
	Other->DestroyComponent();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxInventoryViewModelDelayedDefinitionTest, "Wx.MVVM.Inventory.DelayedDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxInventoryViewModelDelayedDefinitionTest::RunTest(const FString& Parameters)
{
	using namespace WxInventoryViewModelTests;
	FWxTestWorld Local;
	APlayerController* PC = Local.World->SpawnActor<APlayerController>();
	UWxInventoryComponent* Inventory = CreateInventory(PC);
	Inventory->BeginPlay();
	TStrongObjectPtr<UWxItemDefinition> Def(NewObject<UWxItemDefinition>());
	UWxItemInstance* Instance = Inventory->AddItemDefinition(Def.Get(), 1);
	FObjectPropertyBase* DefinitionProperty = FindFProperty<FObjectPropertyBase>(UWxItemInstance::StaticClass(), TEXT("ItemDef"));
	if (!TestNotNull(TEXT("Replicated definition property"), DefinitionProperty))
	{
		return false;
	}
	// 네트워크에서 슬롯 참조가 정의보다 먼저 도착한 상태를 재현한다.
	DefinitionProperty->SetObjectPropertyValue_InContainer(Instance, nullptr);
	TStrongObjectPtr<UWxViewModel_Item> Fixed(NewObject<UWxViewModel_Item>());
	TStrongObjectPtr<UWxViewModel_Inventory> List(NewObject<UWxViewModel_Inventory>());
	Fixed->StartObserving(PC, Def.Get());
	List->StartObserving(PC);
	TestEqual(TEXT("Unresolved definition omits slot"), List->AllItems.Num(), 0);
	DefinitionProperty->SetObjectPropertyValue_InContainer(Instance, Def.Get());
	Instance->ProcessEvent(Instance->FindFunctionChecked(TEXT("HandleItemDefReplicated")), nullptr);
	TestEqual(TEXT("Definition replication updates fixed item without count delta"), Fixed->TotalCount, 1);
	TestEqual(TEXT("Definition replication creates slot"), List->AllItems.Num(), 1);
	TestNull(TEXT("Definition arrival is not a new acquisition"), List->LastAcquiredItem.Get());
	Inventory->NotifyContentsChangedFromReplication();
	TestEqual(TEXT("Repeated receive preserves slot"), List->AllItems.Num(), 1);
	Fixed->Deinitialize();
	List->Deinitialize();
	Inventory->DestroyComponent();
	return true;
}

#endif

// Copyright Woogle. All Rights Reserved.

#include "Inventory/WxGameFeatureAction_AddInventoryItems.h"

#include "Inventory/WxInventoryComponent.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogWxAddInventoryItems, Log, All);

void UWxGameFeatureAction_AddInventoryItems::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	const FGameFeatureStateChangeContext ChangeContext(Context);
	ReadyHandles.Add(ChangeContext, UWxInventoryComponent::OnAnyInventoryReady.AddUObject(this, &ThisClass::HandleInventoryReady, ChangeContext));

	// 활성 전에 BeginPlay 를 지난 인벤토리는 도착 신호가 이미 지나갔으므로 지금 훑는다. 아직 BeginPlay 전이면 신호 쪽이 받는다.
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		UWorld* World = WorldContext.World();
		if (!World || !World->IsGameWorld() || !Context.ShouldApplyToWorldContext(WorldContext))
		{
			continue;
		}

		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			UWxInventoryComponent* Inventory = It->Get() ? It->Get()->FindComponentByClass<UWxInventoryComponent>() : nullptr;
			if (Inventory && Inventory->HasBegunPlay() && Inventory->GetOwner()->HasAuthority())
			{
				Inventory->GrantItems(Items);
				UE_LOG(LogWxAddInventoryItems, Log, TEXT("'%s' 인벤토리에 시작 아이템 %d항목 지급(활성 시점)."), *GetNameSafe(Inventory->GetOwner()), Items.Num());
			}
		}
	}
}

void UWxGameFeatureAction_AddInventoryItems::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	FDelegateHandle Handle;
	if (ReadyHandles.RemoveAndCopyValue(Context, Handle))
	{
		UWxInventoryComponent::OnAnyInventoryReady.Remove(Handle);
	}
}

void UWxGameFeatureAction_AddInventoryItems::HandleInventoryReady(UWxInventoryComponent* Inventory, FGameFeatureStateChangeContext ChangeContext)
{
	const FWorldContext* WorldContext = GEngine->GetWorldContextFromWorld(Inventory->GetWorld());
	if (!WorldContext || !ChangeContext.ShouldApplyToWorldContext(*WorldContext))
	{
		return;
	}

	if (Inventory->GetOwner()->HasAuthority())
	{
		Inventory->GrantItems(Items);
		UE_LOG(LogWxAddInventoryItems, Log, TEXT("'%s' 인벤토리에 시작 아이템 %d항목 지급(도착 신호)."), *GetNameSafe(Inventory->GetOwner()), Items.Num());
	}
}

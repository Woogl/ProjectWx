// Copyright Woogle. All Rights Reserved.

#include "Inventory/WxStateTreeTask_RefillItemCharges.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/WxInventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"

FWxStateTreeTask_RefillItemCharges::FWxStateTreeTask_RefillItemCharges()
{
	bShouldCallTick = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_RefillItemCharges::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const bool bInitialEntry = !Transition.SourceStateID.IsValid();
	if (bInitialEntry)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner || !Owner->HasAuthority())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 대상 선택은 보상 직접 지급 경로와 같은 전제다.
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Owner, 0);
	UWxInventoryComponent* Inventory = PlayerController ? PlayerController->FindComponentByClass<UWxInventoryComponent>() : nullptr;
	if (!Inventory)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	for (UWxItemInstance* Item : Inventory->GetAllItems())
	{
		Inventory->RefillItemCharges(Item);
	}

	return EStateTreeRunStatus::Succeeded;
}

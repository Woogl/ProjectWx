// Copyright Woogle. All Rights Reserved.

#include "Inventory/WxInventoryStateTreeNodes.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"
#include "WxGameplayTags.h"

FWxStateTreeTask_RefillItemCharges::FWxStateTreeTask_RefillItemCharges()
{
	// 진입 시 1회 리필만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_RefillItemCharges::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 초기 진입(StateTree 시작/복원/레이트조인) 또는 세이브 복원(호스트가 상태 태그와 함께 보내는 StateTree.Restore 마커)이면 리필을 재실행하지 않고 곧바로 완료한다(발동 순간에만 리필).
	const bool bInitialEntry = !Transition.SourceStateID.IsValid() || Context.HasEventToProcess(WxGameplayTags::StateTree_Restore);
	if (bInitialEntry)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 인벤토리 쓰기는 서버 권위 사건이라 클라 진입은 노옵(클라는 복제로 충전량을 추종).
	AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner || !Owner->HasAuthority())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 리필 대상은 로컬 플레이어(0번 컨트롤러)의 인벤토리다 — 보상 직접 지급 경로와 같은 전제.
	UWxInventoryManagerComponent* Inventory = UWxInventoryManagerComponent::FindInventory(UGameplayStatics::GetPlayerController(Owner, 0));
	if (!Inventory)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 충전형이 아닌 아이템은 RefillItemCharges 내부에서 무시된다.
	for (UWxItemInstance* Item : Inventory->GetAllItems())
	{
		Inventory->RefillItemCharges(Item);
	}

	// 리필은 즉시 끝나므로 곧바로 완료한다.
	return EStateTreeRunStatus::Succeeded;
}

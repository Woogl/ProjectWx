// Copyright Woogle. All Rights Reserved.

#include "Framework/WxGameMode.h"

#include "Framework/WxExperienceDefinition.h"
#include "Framework/WxGameState.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "WxGame.h"

void AWxGameMode::InitGameState()
{
	Super::InitGameState();

	// 여기는 모든 플레이어 로그인(SpawnPlayActor)보다 앞서므로, 서버 사이드 적용이 로그인 전에 끝나
	// PostLogin 전제(인벤토리·스폰 컴포넌트가 이미 부착돼 있음)가 유지된다.
	AWxGameState* WxGameState = Cast<AWxGameState>(GameState);
	if (!WxGameState)
	{
		UE_LOG(LogWxGame, Warning, TEXT("InitGameState: GameState 가 AWxGameState 가 아님. Experience 를 적용할 수 없음."));
		return;
	}

	if (!Experience)
	{
		UE_LOG(LogWxGame, Warning, TEXT("InitGameState: Experience 미설정. 프레임워크 컴포넌트가 주입되지 않음."));
		return;
	}

	WxGameState->SetCurrentExperience(Experience);
}

void AWxGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (DefaultInventoryItems.IsEmpty())
	{
		return;
	}

	// 인벤토리 주입은 컨트롤러의 PreInitializeComponents 에서 동기로 끝나므로 여기선 이미 붙어 있다.
	// 복제 등록 이전이어도 ReadyForReplication 이 기존 엔트리를 back-fill 하므로 지급이 누락되지 않는다.
	if (UWxInventoryManagerComponent* Inventory = UWxInventoryManagerComponent::FindInventory(NewPlayer))
	{
		Inventory->GrantItems(DefaultInventoryItems);
	}
}

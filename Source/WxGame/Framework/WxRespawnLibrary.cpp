// Copyright Woogle. All Rights Reserved.

#include "Framework/WxRespawnLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "Camera/PlayerCameraManager.h"
#include "CommonActivatableWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "System/WxCheckpointSubsystem.h"
#include "System/WxSpawnerLibrary.h"
#include "WxGameplayTags.h"
#include "WxGame.h"

bool UWxRespawnLibrary::RequestRespawn(UCommonActivatableWidget* DeathScreen)
{
	if (!IsValid(DeathScreen) || !DeathScreen->IsActivated())
	{
		return false;
	}
	APlayerController* Controller = DeathScreen->GetOwningPlayer();
	if (!IsValid(Controller) || !Controller->IsLocalController())
	{
		return false;
	}
	UWorld* World = Controller->GetWorld();
	AGameModeBase* GameMode = World ? World->GetAuthGameMode() : nullptr;
	APawn* DeadPawn = Controller->GetPawn();
	const UAbilitySystemComponent* DeadASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(DeadPawn);
	if (!GameMode || !World->IsNetMode(NM_Standalone) || !IsValid(DeadPawn)
		|| !DeadASC || !DeadASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death)
		|| !GameMode->GetDefaultPawnClassForController(Controller))
	{
		return false;
	}
	const UGameInstance* GameInstance = World->GetGameInstance();
	const UWxCheckpointSubsystem* Checkpoint = GameInstance ? GameInstance->GetSubsystem<UWxCheckpointSubsystem>() : nullptr;
	FTransform RespawnTransform;
	const bool bHasCheckpoint = Checkpoint && Checkpoint->TryGetCheckpoint(World, RespawnTransform);
	const bool bHadCollision = DeadPawn->GetActorEnableCollision();
	DeadPawn->SetActorEnableCollision(false);
	// 엔진 RestartPlayer는 빙의 중인 Pawn이 있으면 재사용한다.
	Controller->UnPossess();
	if (bHasCheckpoint)
	{
		GameMode->RestartPlayerAtTransform(Controller, RespawnTransform);
	}
	else
	{
		GameMode->RestartPlayer(Controller);
	}
	APawn* NewPawn = Controller->GetPawn();
	if (!IsValid(NewPawn))
	{
		// 생성에 실패하면 사망 화면에서 다시 시도할 수 있도록 기존 Pawn을 유지한다.
		DeadPawn->SetActorEnableCollision(bHadCollision);
		Controller->Possess(DeadPawn);
		UE_LOG(LogWxGame, Error, TEXT("RequestRespawn: 플레이어 생성 실패."));
		return false;
	}
	DeadPawn->Destroy();
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(NewPawn))
	{
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetHPAttribute(), ASC->GetNumericAttribute(UWxCombatAttributeSet::GetMaxHPAttribute()));
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMPAttribute(), ASC->GetNumericAttribute(UWxCombatAttributeSet::GetMaxMPAttribute()));
	}
	// 다음 월드 틱 전에 새 시점의 지형 스트리밍을 완료해 낙하를 방지한다.
	Controller->SetViewTarget(NewPawn);
	if (Controller->PlayerCameraManager)
	{
		Controller->PlayerCameraManager->UpdateCamera(0.0f);
	}
	World->BlockTillLevelStreamingCompleted();
	UWxSpawnerLibrary::TryRespawnAll(Controller);
	DeathScreen->DeactivateWidget();
	return true;
}

// Copyright Woogle. All Rights Reserved.

#include "Framework/WxPlayerSpawningComponent.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "WxGame.h"
#include "WxPersistenceGameSubsystem.h"

UWxPlayerSpawningComponent::UWxPlayerSpawningComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

AActor* UWxPlayerSpawningComponent::ChoosePlayerStart(AController* Player)
{
	// 1. 저장된 PlayerStartTag 가 있으면 그 PlayerStart(체크포인트 등)로 부활한다.
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UWxPersistenceGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UWxPersistenceGameSubsystem>())
			{
				if (APlayerStart* SavedStart = FindPlayerStartByTag(SaveSubsystem->GetPlayerStartTag()))
				{
					return SavedStart;
				}
			}
		}
	}

	// 2. 세이브 없음/미발견: 최초 접속용 기본 태그 "Default" 의 PlayerStart 를 입구로 쓴다.
	if (APlayerStart* DefaultStart = FindPlayerStartByTag(TEXT("Default")))
	{
		return DefaultStart;
	}

	// 3. "Default" PlayerStart 가 없으면 설정 오류 — 에러 로그 후 nullptr(GameMode 가 엔진 기본 선택으로 폴백).
	UE_LOG(LogWxGame, Error, TEXT("PlayerStartTag 'Default' 를 가진 PlayerStart 가 없음."));
	return nullptr;
}

bool UWxPlayerSpawningComponent::TryGetSavedPawnSpawn(FTransform& OutPawnTransform, FRotator& OutControlRotation) const
{
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UWxPersistenceGameSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxPersistenceGameSubsystem>() : nullptr;
	const UWxPersistenceSaveGame* SaveGame = SaveSubsystem ? SaveSubsystem->GetSaveGame() : nullptr;
	if (!SaveGame || !SaveGame->TravelData.bHasPawnTransform)
	{
		return false;
	}

	// 저장 맵과 현재 맵이 다르면(다른 맵에서 PIE 시작 등) 좌표가 무의미하므로 적용하지 않는다.
	const FName SavedMap = SaveGame->TravelData.Map.GetAssetPath().GetPackageName();
	if (SavedMap != UWxPersistenceGameSubsystem::GetStableMapPackageName(World))
	{
		return false;
	}

	OutPawnTransform = SaveGame->TravelData.PawnTransform;
	OutControlRotation = SaveGame->TravelData.ControlRotation;
	return true;
}

APlayerStart* UWxPlayerSpawningComponent::FindPlayerStartByTag(FName Tag) const
{
	if (Tag.IsNone())
	{
		return nullptr;
	}

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* Start = *It;
		if (Start && Start->PlayerStartTag == Tag)
		{
			return Start;
		}
	}

	return nullptr;
}

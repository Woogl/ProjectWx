// Copyright Woogle. All Rights Reserved.

#include "Framework/WxPlayerSpawningComponent.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "WxGame.h"
#include "WxSaveGameSubsystem.h"

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
			if (UWxSaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UWxSaveGameSubsystem>())
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
	UE_LOG(LogWxGame, Error, TEXT("PlayerSpawning: 최초 접속용 PlayerStartTag 'Default' 를 가진 PlayerStart 가 없음. 레벨 입구 PlayerStart 에 'Default' 태그를 부여하라. 엔진 기본 선택으로 폴백."));
	return nullptr;
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

// Copyright Woogle. All Rights Reserved.

#include "Framework/WxPlayerSpawningComponent.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/PlayerStartPIE.h"
#include "GameFramework/PlayerStart.h"
#include "WorldObject/WxCheckPoint.h"
#include "WxGame.h"
#include "WxPersistenceGameSubsystem.h"

UWxPlayerSpawningComponent::UWxPlayerSpawningComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

AActor* UWxPlayerSpawningComponent::ChoosePlayerStart(AController* Player)
{
	// 0. PIE "여기서 플레이"(APlayerStartPIE)가 있으면 최우선 사용한다.
	// 저장 태그가 이를 덮으면 개발 중 원하는 위치로 못 뜨므로 앞에 둔다.
	for (TActorIterator<APlayerStartPIE> It(GetWorld()); It; ++It)
	{
		return *It;
	}

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

	// 2. 세이브 없음/미발견(최초 접속): bIsDefaultStart 이 켜진 첫 체크포인트를 시작지점으로 쓴다.
	//    같은 순회에서 체크포인트 존재 여부(bAnyCheckPoint)도 함께 판정해 아래 폴백 분기의 로그 정책을 정한다.
	bool bAnyCheckPoint = false;
	for (TActorIterator<AWxCheckPoint> It(GetWorld()); It; ++It)
	{
		bAnyCheckPoint = true;
		if (It->IsDefaultStart())
		{
			return *It;
		}
	}

	// 3. 플래그된 체크포인트가 없음.
	if (bAnyCheckPoint)
	{
		// 체크포인트는 있는데 아무도 최초 시작지점으로 지정되지 않음 — 기획 실수.
		// 에러 로그 후 엔진 기본 선택으로 폴백한다.
		UE_LOG(LogWxGame, Error, TEXT("bIsDefaultStart 이 켜진 체크포인트가 없음 — 최초 시작지점을 결정할 수 없다. 체크포인트 하나에 지정하라."));
	}
	// 체크포인트가 아예 없는 맵(메뉴·테스트 짐 등 부활 흐름 밖)은 정상 케이스이므로 무로그로 엔진 기본 선택에 맡긴다.
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

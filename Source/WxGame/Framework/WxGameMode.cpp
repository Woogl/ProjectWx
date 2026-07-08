// Copyright Woogle. All Rights Reserved.

#include "Framework/WxGameMode.h"

#include "Components/ControllerComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Components/GameStateComponent.h"
#include "Components/PawnComponent.h"
#include "Components/PlayerStateComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/PlayerStartPIE.h"
#include "EngineUtils.h"
#include "Framework/WxGameState.h"
#include "Framework/WxPlayerSpawningComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "WxGame.h"
#include "WxPersistenceGameSubsystem.h"
#include "WxPersistenceSaveGame.h"
#include "WxPersistenceWorldSubsystem.h"

void AWxGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// 에셋에 설정된 프레임워크 컴포넌트 주입 목록을 ModularGameplay 컴포넌트 매니저에 요청 등록한다.
	// receiver 액터(GameState 등)가 spawn 후 receiver 로 등록되면 자동으로 해당 컴포넌트를 부착받는다.
	UGameInstance* GameInstance = GetGameInstance();
	UGameFrameworkComponentManager* Manager = GameInstance ? GameInstance->GetSubsystem<UGameFrameworkComponentManager>() : nullptr;
	if (!Manager)
	{
		return;
	}

	for (const TSubclassOf<UGameFrameworkComponent>& ComponentClass : FrameworkComponents)
	{
		if (!ComponentClass)
		{
			continue;
		}

		// 컴포넌트 베이스 클래스로 부착 대상 프레임워크 액터를 자동 추론한다.
		UClass* ReceiverClass = nullptr;
		if (ComponentClass->IsChildOf(UGameStateComponent::StaticClass()))
		{
			ReceiverClass = AGameStateBase::StaticClass();
		}
		else if (ComponentClass->IsChildOf(UPawnComponent::StaticClass()))
		{
			ReceiverClass = APawn::StaticClass();
		}
		else if (ComponentClass->IsChildOf(UControllerComponent::StaticClass()))
		{
			ReceiverClass = AController::StaticClass();
		}
		else if (ComponentClass->IsChildOf(UPlayerStateComponent::StaticClass()))
		{
			ReceiverClass = APlayerState::StaticClass();
		}

		if (!ReceiverClass)
		{
			UE_LOG(LogWxGame, Warning, TEXT("InitGame: '%s' 의 부착 대상을 추론할 수 없음(GameState/Pawn/Controller/PlayerState 컴포넌트 아님). 건너뜀."), *GetNameSafe(ComponentClass.Get()));
			continue;
		}

		ComponentRequestHandles.Add(Manager->AddComponentRequest(TSoftClassPtr<AActor>(ReceiverClass), ComponentClass));
	}
}

AActor* AWxGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// 스폰 지점 선택은 GameState 의 UWxPlayerSpawningComponent 에 위임한다(Lyra 패턴). GameMode 는 위임 + 엔진 기본 폴백만 담당.
	AWxGameState* WxGameState = Cast<AWxGameState>(GameState);
	UWxPlayerSpawningComponent* PlayerSpawning = WxGameState ? WxGameState->FindComponentByClass<UWxPlayerSpawningComponent>() : nullptr;
	if (PlayerSpawning)
	{
		// 저장 태그·최초 시작지점(bIsDefaultStart)을 못 찾으면 컴포넌트가 nullptr 을 반환하므로 엔진 기본(랜덤)으로 폴백한다.
		if (AActor* Start = PlayerSpawning->ChoosePlayerStart(Player))
		{
			return Start;
		}
	}
	else
	{
		UE_LOG(LogWxGame, Warning, TEXT("ChoosePlayerStart: UWxPlayerSpawningComponent 부재(WxGameState 미사용?) — 엔진 기본 선택으로 폴백."));
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

APawn* AWxGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	// 명시 저장된 플레이어 위치가 있으면 그 트랜스폼으로 스폰해 "저장 지점 복원"을 구현한다. 없으면(오토세이브/사망/신규/PIE 여기서플레이) 인자 그대로 = PlayerStart.
	FTransform SavedTransform;
	const FTransform& FinalTransform = TryGetSavedPlayerTransform(SavedTransform) ? SavedTransform : SpawnTransform;
	return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, FinalTransform);
}

void AWxGameMode::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	Super::FinishRestartPlayer(NewPlayer, StartRotation);

	// 저장된 스탯이 있으면 새 폰의 어트리뷰트를 복원한다. Super 의 Possess 이후라 ASC 초기화(GiveAbilitySet)가 끝나 있어 기본값 위에 안전하게 덮어쓴다(스탠드얼론 authority 전제).
	UGameInstance* GameInstance = GetGameInstance();
	UWxPersistenceGameSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxPersistenceGameSubsystem>() : nullptr;
	const UWxPersistenceSaveGame* SaveGame = SaveSubsystem ? SaveSubsystem->GetSaveGame() : nullptr;
	if (SaveGame && SaveGame->bHasPlayerStats && NewPlayer)
	{
		if (APawn* Pawn = NewPlayer->GetPawn())
		{
			UWxPersistenceWorldSubsystem::ApplyPlayerStats(Pawn, SaveGame->PlayerStats);
		}
	}

	// 저장 위치로 복원된 경우, 로드 직후 카메라(컨트롤 로테이션)를 캐릭터가 바라보는 방향(저장된 폰 회전 Yaw)으로 맞춘다. 시선은 별도 저장 없이 복원된 폰 회전에서 파생한다.
	FTransform SavedTransform;
	if (NewPlayer && TryGetSavedPlayerTransform(SavedTransform))
	{
		NewPlayer->SetControlRotation(FRotator(0.0f, SavedTransform.GetRotation().Rotator().Yaw, 0.0f));
	}
}

bool AWxGameMode::TryGetSavedPlayerTransform(FTransform& OutTransform) const
{
	// PIE "여기서 플레이"(APlayerStartPIE)가 있으면 저장 위치로 덮지 않는다 — 개발 중 지정 위치 우선(ChoosePlayerStart 의 최우선 규칙과 일치).
	for (TActorIterator<APlayerStartPIE> It(GetWorld()); It; ++It)
	{
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UWxPersistenceGameSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxPersistenceGameSubsystem>() : nullptr;
	const UWxPersistenceSaveGame* SaveGame = SaveSubsystem ? SaveSubsystem->GetSaveGame() : nullptr;
	if (!SaveGame)
	{
		return false;
	}

	// 위치 유효성은 sentinel(Identity)로 판정한다(별도 플래그 없음). 좌표는 맵 종속이라 저장 맵이 현재 월드와 일치할 때만 유효하다(정상 로드-트래블은 같은 맵으로 오므로 통과, 크로스맵 오적용만 차단 — ReportTravelFromSaveFileComplete 와 동일 비교).
	const FTransform& SavedTransform = SaveGame->TravelData.PlayerTransform;
	const FName SavedMap = SaveGame->TravelData.Map.IsNull() ? NAME_None : SaveGame->TravelData.Map.GetAssetPath().GetPackageName();
	const FName CurrentMap = UWxPersistenceGameSubsystem::GetStableMapPackageName(GetWorld());
	if (SavedTransform.Equals(FTransform::Identity) || SavedMap != CurrentMap)
	{
		return false;
	}

	OutTransform = SavedTransform;
	return true;
}

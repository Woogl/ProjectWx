// Copyright Woogle. All Rights Reserved.

#include "Framework/WxGameMode.h"

#include "Components/ControllerComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Components/GameStateComponent.h"
#include "Components/PawnComponent.h"
#include "Components/PlayerStateComponent.h"
#include "Engine/GameInstance.h"
#include "Framework/WxGameState.h"
#include "Framework/WxPlayerSpawningComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "WxGame.h"

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

	for (const TSubclassOf<UGameFrameworkComponent>& ComponentClass : InjectedFrameworkComponents)
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
		// 저장 태그/"Default" 태그를 못 찾으면 컴포넌트가 에러 로그 후 nullptr 을 반환하므로 엔진 기본(랜덤)으로 폴백한다.
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

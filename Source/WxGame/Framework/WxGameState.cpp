// Copyright Woogle. All Rights Reserved.

#include "Framework/WxGameState.h"

#include "Components/ControllerComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Components/GameStateComponent.h"
#include "Components/PawnComponent.h"
#include "Components/PlayerStateComponent.h"
#include "Engine/GameInstance.h"
#include "Framework/WxExperienceDefinition.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "WxGame.h"

void AWxGameState::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	// ModularGameplay 컴포넌트 수신 opt-in. 활성 주입 요청의 컴포넌트가 여기에 자동 부착된다.
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AWxGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);

	// GC 를 기다리지 않고 주입 요청을 결정적으로 해제한다. 마지막 요청이 풀리면 매니저가 주입 컴포넌트를 정리한다.
	ComponentRequestHandles.Reset();

	Super::EndPlay(EndPlayReason);
}

void AWxGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxGameState, CurrentExperience);
}

void AWxGameState::SetCurrentExperience(const UWxExperienceDefinition* Experience)
{
	if (!HasAuthority() || !Experience || CurrentExperience)
	{
		return;
	}

	CurrentExperience = Experience;
	ApplyExperience();
}

void AWxGameState::OnRep_CurrentExperience()
{
	ApplyExperience();
}

void AWxGameState::ApplyExperience()
{
	// async 패키지 로딩 설정에선 참조가 unmapped(null)로 먼저 도착할 수 있다. 매핑이 끝나면 RepNotify 가 다시 불려 회복된다.
	if (!CurrentExperience)
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UGameFrameworkComponentManager* Manager = GameInstance ? GameInstance->GetSubsystem<UGameFrameworkComponentManager>() : nullptr;
	if (!Manager)
	{
		return;
	}

	// 사이드 판정은 엔진 GameFeatureAction_AddComponents 와 동일 규칙. 리슨 호스트·스탠드얼론은 양쪽을 모두 만족한다.
	const ENetMode NetMode = GetNetMode();
	const bool bIsServer = NetMode != NM_Client;
	const bool bIsClient = NetMode != NM_DedicatedServer;

	for (const FWxFrameworkComponentEntry& Entry : CurrentExperience->FrameworkComponents)
	{
		if (!Entry.ComponentClass)
		{
			continue;
		}

		const bool bShouldApply = (bIsServer && Entry.bServerComponent) || (bIsClient && Entry.bClientComponent);
		if (!bShouldApply)
		{
			continue;
		}

		// 컴포넌트 베이스 클래스로 부착 대상 프레임워크 액터를 자동 추론한다.
		UClass* ReceiverClass = nullptr;
		if (Entry.ComponentClass->IsChildOf(UGameStateComponent::StaticClass()))
		{
			ReceiverClass = AGameStateBase::StaticClass();
		}
		else if (Entry.ComponentClass->IsChildOf(UPawnComponent::StaticClass()))
		{
			// 주의: 클라 OnRep 적용은 라이브 월드 도중이라 소급 스캔이 초기화 끝난 모든 폰(AI 포함)에 닿는다.
			// 폰 컴포넌트를 엔트리에 넣으려면 그 폰 클래스가 receiver 로 등록돼 있어야 한다.
			ReceiverClass = APawn::StaticClass();
		}
		else if (Entry.ComponentClass->IsChildOf(UControllerComponent::StaticClass()))
		{
			ReceiverClass = AController::StaticClass();
		}
		else if (Entry.ComponentClass->IsChildOf(UPlayerStateComponent::StaticClass()))
		{
			ReceiverClass = APlayerState::StaticClass();
		}

		if (!ReceiverClass)
		{
			UE_LOG(LogWxGame, Warning, TEXT("ApplyExperience: '%s' 의 부착 대상을 추론할 수 없음(GameState/Pawn/Controller/PlayerState 컴포넌트 아님). 건너뜀."), *GetNameSafe(Entry.ComponentClass.Get()));
			continue;
		}

		ComponentRequestHandles.Add(Manager->AddComponentRequest(TSoftClassPtr<AActor>(ReceiverClass), Entry.ComponentClass));
	}
}

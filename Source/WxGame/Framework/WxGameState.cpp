// Copyright Woogle. All Rights Reserved.

#include "Framework/WxGameState.h"

#include "Components/ControllerComponent.h"
#include "Components/GameFrameworkComponent.h"
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

	const bool bIsNetClient = GetNetMode() == NM_Client;

	for (const TSubclassOf<UGameFrameworkComponent>& ComponentClass : CurrentExperience->FrameworkComponents)
	{
		if (!ComponentClass)
		{
			continue;
		}

		// 복제 컴포넌트는 authority 액터에만 생성된다(엔진 UGameFrameworkComponentManager::CreateComponentOnInstance 의 규칙).
		// 클라에는 서버 사본이 복제로 도착하므로 아무것도 만들지 못할 요청을 남기지 않는다.
		// 비복제 컴포넌트는 양쪽에 요청하고, 사이드 제한은 컴포넌트가 자기 role·로컬 여부로 스스로 한다.
		if (bIsNetClient && ComponentClass.GetDefaultObject()->GetIsReplicated())
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
			// 주의: 클라 OnRep 적용은 라이브 월드 도중이라 소급 스캔이 초기화 끝난 모든 폰(AI 포함)에 닿는다.
			// 폰 컴포넌트를 목록에 넣으려면 그 폰 클래스가 receiver 로 등록돼 있어야 한다.
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
			UE_LOG(LogWxGame, Warning, TEXT("ApplyExperience: '%s' 의 부착 대상을 추론할 수 없음(GameState/Pawn/Controller/PlayerState 컴포넌트 아님). 건너뜀."), *GetNameSafe(ComponentClass.Get()));
			continue;
		}

		ComponentRequestHandles.Add(Manager->AddComponentRequest(TSoftClassPtr<AActor>(ReceiverClass), ComponentClass));
	}
}

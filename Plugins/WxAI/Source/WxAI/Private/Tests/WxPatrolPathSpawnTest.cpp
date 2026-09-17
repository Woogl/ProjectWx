// Copyright Woogle. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AIController.h"
#include "Engine/Engine.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "WxAIBehaviorComponent.h"
#include "WxPatrolComponent.h"

// 정찰 경로 인계는 컴포넌트 초기화가 AI 자동 빙의(Owner 덮어쓰기)보다 먼저라는 엔진 순서에 기댄다. 깨지면 정찰이 조용히 꺼지므로 실제 스폰·빙의로 지킨다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxPatrolPathSpawnTest, "Wx.AI.Patrol.SpawnOwnerPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxPatrolPathSpawnTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(true);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());

	AActor* PathOwner = World->SpawnActor<AActor>();
	UWxPatrolComponent* Patrol = NewObject<UWxPatrolComponent>(PathOwner);
	Patrol->RegisterComponent();

	auto SpawnAIPawn = [World](AActor* SpawnOwner)
	{
		APawn* Pawn = World->SpawnActorDeferred<APawn>(APawn::StaticClass(), FTransform::Identity, SpawnOwner);
		Pawn->AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
		Pawn->AIControllerClass = AAIController::StaticClass();
		NewObject<UWxAIBehaviorComponent>(Pawn)->RegisterComponent();
		Pawn->FinishSpawning(FTransform::Identity);
		return Pawn;
	};

	const APawn* SpawnedPawn = SpawnAIPawn(PathOwner);
	TestNotNull(TEXT("AI 자동 빙의"), SpawnedPawn->GetController());
	TestEqual(TEXT("빙의가 Owner 를 컨트롤러로 덮음"), SpawnedPawn->GetOwner(), static_cast<AActor*>(SpawnedPawn->GetController()));
	TestEqual(TEXT("빙의 후에도 스폰 주체의 경로 유지"), UWxPatrolComponent::FindPatrolComponent(SpawnedPawn), Patrol);

	const APawn* OwnerlessPawn = SpawnAIPawn(nullptr);
	TestNotNull(TEXT("Owner 없는 폰도 AI 자동 빙의"), OwnerlessPawn->GetController());
	TestNull(TEXT("스폰 주체가 없으면 경로 없음"), UWxPatrolComponent::FindPatrolComponent(OwnerlessPawn));

	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

#endif

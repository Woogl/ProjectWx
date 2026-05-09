// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxSpawnConsole.h"

#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "Net/UnrealNetwork.h"
#include "Spawnable/WxSpawner.h"

AWxSpawnConsole::AWxSpawnConsole()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ConsoleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ConsoleMesh"));
	ConsoleMesh->SetupAttachment(SceneRoot);

	ConsoleInteraction = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("ConsoleInteraction"));
	ConsoleInteraction->SetupAttachment(ConsoleMesh);
}

void AWxSpawnConsole::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxSpawnConsole, bTriggered);
}

void AWxSpawnConsole::BeginPlay()
{
	Super::BeginPlay();

	ConsoleInteraction->OnInteracted.AddDynamic(this, &AWxSpawnConsole::HandleConsoleInteracted);
}

void AWxSpawnConsole::HandleConsoleInteracted(AActor* InteractingActor)
{
	// 스폰 + 상태 전이는 권위 측에서만. 클라이언트 프롬프트 동기화는 OnRep_bTriggered 에서 처리.
	if (!HasAuthority())
	{
		return;
	}

	if (bTriggered)
	{
		return;
	}

	bTriggered = true;
	ConsoleInteraction->SetInteractionEnabled(false);

	// 스트리밍 아웃된 Spawner 는 강제 로드하지 않고 스킵. 디자이너가 콘솔과 같은 영역에 배치되도록 보장해야 함.
	for (const TSoftObjectPtr<AWxSpawner>& SoftSpawner : TargetSpawners)
	{
		if (AWxSpawner* Spawner = SoftSpawner.Get())
		{
			Spawner->Respawn();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AWxSpawnConsole '%s': TargetSpawner is null or not loaded."), *GetName());
		}
	}
}

void AWxSpawnConsole::OnRep_bTriggered()
{
	if (bTriggered)
	{
		ConsoleInteraction->SetInteractionEnabled(false);
	}
}

// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxAlarmConsole.h"

#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"

AWxAlarmConsole::AWxAlarmConsole()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Console = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Console"));
	Console->SetupAttachment(SceneRoot);

	ConsoleInteraction = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("ConsoleInteraction"));
	ConsoleInteraction->SetupAttachment(Console);
}

void AWxAlarmConsole::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxAlarmConsole, bTriggered);
}

void AWxAlarmConsole::BeginPlay()
{
	Super::BeginPlay();

	ConsoleInteraction->OnInteracted.AddDynamic(this, &AWxAlarmConsole::HandleConsoleInteracted);
}

void AWxAlarmConsole::HandleConsoleInteracted(AActor* InteractingActor)
{
	// OnInteracted 는 서버+모든 클라이언트에서 fire. 권위 측에서 상태 전이/비활성을 수행하고,
	// FX/사운드는 모든 피어가 로컬 재생한다. 클라이언트 측 비활성은 OnRep_bTriggered 에서 처리.
	if (HasAuthority())
	{
		if (bTriggered)
		{
			return;
		}
		bTriggered = true;
		ConsoleInteraction->SetInteractionEnabled(false);
	}

	PlayAlarmFx();
}

void AWxAlarmConsole::OnRep_bTriggered()
{
	// 라이브 발동 시 클라이언트 프롬프트 즉시 숨김 + 레이트조인 시 콘솔 비활성 동기화.
	if (bTriggered)
	{
		ConsoleInteraction->SetInteractionEnabled(false);
	}
}

void AWxAlarmConsole::PlayAlarmFx()
{
	if (AlarmNiagaraSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(AlarmNiagaraSystem, Console, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
	}

	if (AlarmSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, AlarmSound, GetActorLocation());
	}
}

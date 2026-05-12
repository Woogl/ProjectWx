// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxAlarmConsole.h"

#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

AWxAlarmConsole::AWxAlarmConsole()
{
	Console = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Console"));
	Console->SetupAttachment(SceneRoot);

	ConsoleInteraction = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("ConsoleInteraction"));
	ConsoleInteraction->SetupAttachment(Console);
}

void AWxAlarmConsole::BeginPlay()
{
	Super::BeginPlay();

	ConsoleInteraction->OnInteracted.AddDynamic(this, &AWxAlarmConsole::HandleInteracted);

	// Level Streaming Persistence 가 BeginPlay 직전에 bTriggered 를 직접 set 했을 수 있으므로 동기화.
	ApplyState();
}

void AWxAlarmConsole::ApplyState()
{
	if (bTriggered)
	{
		ConsoleInteraction->SetInteractionEnabled(false);
	}
}

void AWxAlarmConsole::HandleInteracted(AActor* InstigatorActor)
{
	// 권위 측은 1회성 발동 처리 (MarkTriggered 가 ApplyState 를 디스패치 → 인터랙션 비활성).
	// FX 는 모든 피어가 로컬 재생. 발동 후엔 인터랙션이 비활성화되어 추가 OnInteracted 가 발화하지 않으므로
	// 레이트조인 클라는 OnRep 으로 비활성만 동기화하고 FX 는 재생하지 않는다.
	if (HasAuthority() && !bTriggered)
	{
		MarkTriggered();
	}

	PlayAlarmFx();
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

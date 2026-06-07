// Copyright Woogle. All Rights Reserved.

#include "Time/WxTimeDilationComponent.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

UWxTimeDilationComponent::UWxTimeDilationComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UWxTimeDilationComponent::SetGlobalTimeDilationAuthoritative(const UObject* WorldContextObject, float NewDilation)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return;
	}

	AGameStateBase* GameState = World->GetGameState();
	if (!GameState)
	{
		return;
	}

	if (UWxTimeDilationComponent* Comp = GameState->FindComponentByClass<UWxTimeDilationComponent>())
	{
		Comp->SetGlobalTimeDilationAuthoritative(NewDilation);
	}
}

void UWxTimeDilationComponent::SetGlobalTimeDilationAuthoritative(float NewDilation)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (FMath::IsNearlyEqual(ReplicatedTimeDilation, NewDilation))
	{
		return;
	}

	ReplicatedTimeDilation = NewDilation;

	// 서버에서는 RepNotify가 자동 호출되지 않으므로 직접 호출해 적용한다.
	ApplyTimeDilation(NewDilation);
}

void UWxTimeDilationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWxTimeDilationComponent, ReplicatedTimeDilation);
}

void UWxTimeDilationComponent::BeginPlay()
{
	Super::BeginPlay();

	// Late join 클라이언트가 진행 중인 슬로우 상태로 합류해도 즉시 동기화되도록 적용.
	if (!FMath::IsNearlyEqual(ReplicatedTimeDilation, 1.f))
	{
		ApplyTimeDilation(ReplicatedTimeDilation);
	}
}

void UWxTimeDilationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 슬로우 상태로 World가 종료되면 다음 World에 잔존 영향이 가지 않도록 복원.
	ApplyTimeDilation(1.f);

	Super::EndPlay(EndPlayReason);
}

void UWxTimeDilationComponent::OnRep_ReplicatedTimeDilation()
{
	ApplyTimeDilation(ReplicatedTimeDilation);
}

void UWxTimeDilationComponent::ApplyTimeDilation(float Dilation)
{
	UGameplayStatics::SetGlobalTimeDilation(this, Dilation);
}

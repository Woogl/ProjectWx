// Copyright Woogle. All Rights Reserved.

#include "Time/WxTimeDilationComponent.h"

#include "WxCombatModule.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

UWxTimeDilationComponent::UWxTimeDilationComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UWxTimeDilationComponent::SetGlobalTimeDilationAuthoritative(const UObject* Requester, float NewDilation)
{
	if (UWxTimeDilationComponent* Comp = FindComponent(Requester))
	{
		Comp->SetDilationFrom(Requester, NewDilation);
	}
}

void UWxTimeDilationComponent::ClearGlobalTimeDilationAuthoritative(const UObject* Requester)
{
	if (UWxTimeDilationComponent* Comp = FindComponent(Requester))
	{
		Comp->ClearDilationFrom(Requester);
	}
}

void UWxTimeDilationComponent::SetDilationFrom(const UObject* Requester, float NewDilation)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	// 소유자 없는 딜레이션은 아무도 걷어낼 수 없으므로 받지 않는다.
	if (!Requester)
	{
		UE_LOG(LogWxCombat, Warning, TEXT("SetGlobalTimeDilationAuthoritative: Requester가 없어 무시한다."));
		return;
	}

	// 값이 같아도 소유권은 새 요청자에게 넘어가야 하므로 조기 반환하지 않는다.
	DilationOwner = Requester;
	ReplicatedTimeDilation = NewDilation;

	// 서버에서는 RepNotify가 자동 호출되지 않으므로 직접 호출해 적용한다.
	ApplyTimeDilation(NewDilation);
}

void UWxTimeDilationComponent::ClearDilationFrom(const UObject* Requester)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	// 소유자가 살아 있는데 내가 아니면 남의 연출이라 건드리지 않는다.
	// 소유자가 이미 사라졌다면 값만 남은 상태이므로 이 해제로 걷어낸다.
	if (DilationOwner.IsValid() && DilationOwner.Get() != Requester)
	{
		return;
	}

	DilationOwner.Reset();
	ReplicatedTimeDilation = 1.f;

	ApplyTimeDilation(1.f);
}

UWxTimeDilationComponent* UWxTimeDilationComponent::FindComponent(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return nullptr;
	}

	AGameStateBase* GameState = World->GetGameState();
	if (!GameState)
	{
		return nullptr;
	}

	UWxTimeDilationComponent* Comp = GameState->FindComponentByClass<UWxTimeDilationComponent>();
	if (!Comp)
	{
		// 컴포넌트가 없으면 슬로우 연출이 통째로 사라지는데 화면만 봐서는 원인이 안 보인다.
		// GameMode가 고른 Experience의 컴포넌트 주입 설정을 확인해야 한다.
		UE_LOG(LogWxCombat, Warning, TEXT("GameState '%s'에 WxTimeDilationComponent가 없어 TimeDilation 요청이 무시된다."), *GetNameSafe(GameState));
	}

	return Comp;
}

void UWxTimeDilationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWxTimeDilationComponent, ReplicatedTimeDilation);
}

void UWxTimeDilationComponent::BeginPlay()
{
	Super::BeginPlay();

	// Late join 클라이언트가 진행 중인 슬로우 상태로 합류해도 즉시 맞춘다.
	if (!FMath::IsNearlyEqual(ReplicatedTimeDilation, 1.f))
	{
		ApplyTimeDilation(ReplicatedTimeDilation);
	}
}

void UWxTimeDilationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 슬로우 상태로 World가 끝나면 다음 World까지 배율이 따라간다.
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

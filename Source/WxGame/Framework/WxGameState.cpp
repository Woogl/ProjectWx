// Copyright Woogle. All Rights Reserved.

#include "Framework/WxGameState.h"

#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AWxGameState::AWxGameState()
{
}

void AWxGameState::SetGlobalTimeDilationAuthoritative(float NewDilation)
{
	if (!HasAuthority())
	{
		return;
	}

	if (FMath::IsNearlyEqual(ReplicatedTimeDilation, NewDilation))
	{
		return;
	}

	ReplicatedTimeDilation = NewDilation;

	// 서버에서는 RepNotify가 자동 호출되지 않으므로 직접 호출해 적용한다.
	OnRep_ReplicatedTimeDilation();
}

void AWxGameState::OnRep_ReplicatedTimeDilation()
{
	UGameplayStatics::SetGlobalTimeDilation(this, ReplicatedTimeDilation);
}

void AWxGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxGameState, ReplicatedTimeDilation);
}

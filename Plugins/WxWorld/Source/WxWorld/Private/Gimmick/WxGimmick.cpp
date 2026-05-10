// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxGimmick.h"

#include "Net/UnrealNetwork.h"

AWxGimmick::AWxGimmick()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
}

void AWxGimmick::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxGimmick, bTriggered);
}

void AWxGimmick::MarkTriggered()
{
	if (!HasAuthority() || bTriggered)
	{
		return;
	}

	bTriggered = true;
	ApplyState();
}

void AWxGimmick::OnRep_bTriggered()
{
	ApplyState();
}

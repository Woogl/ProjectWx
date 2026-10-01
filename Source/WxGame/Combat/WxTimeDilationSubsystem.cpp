// Copyright Woogle. All Rights Reserved.

#include "Combat/WxTimeDilationSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

uint64 UWxTimeDilationSubsystem::AddRequest(float TimeDilation)
{
	if (!GetWorld() || GetWorld()->GetNetMode() == NM_Client || !FMath::IsFinite(TimeDilation))
	{
		return 0;
	}
	if (Requests.IsEmpty())
	{
		OriginalDilation = UGameplayStatics::GetGlobalTimeDilation(this);
	}
	const uint64 Handle = ++NextHandle;
	Requests.Add(Handle, TimeDilation);
	ApplyCurrentRequest();
	return Handle;
}

void UWxTimeDilationSubsystem::RemoveRequest(uint64 Handle)
{
	if (Requests.Remove(Handle) > 0)
	{
		ApplyCurrentRequest();
	}
}

void UWxTimeDilationSubsystem::ApplyCurrentRequest()
{
	uint64 LatestHandle = 0;
	float Dilation = OriginalDilation;
	for (const TPair<uint64, float>& Request : Requests)
	{
		if (Request.Key > LatestHandle)
		{
			LatestHandle = Request.Key;
			Dilation = Request.Value;
		}
	}
	// 배율 제한과 클라이언트 복제는 엔진의 WorldSettings를 따른다.
	UGameplayStatics::SetGlobalTimeDilation(this, Dilation);
}

void UWxTimeDilationSubsystem::Deinitialize()
{
	if (!Requests.IsEmpty())
	{
		Requests.Reset();
		ApplyCurrentRequest();
	}
	Super::Deinitialize();
}

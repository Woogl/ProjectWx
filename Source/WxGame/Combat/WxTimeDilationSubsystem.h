// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WxTimeDilationSubsystem.generated.h"

/** 서버의 전역 배율 요청을 관리한다. 가장 최근의 활성 요청을 적용하고 마지막 해제 시 원래 배율로 복원한다. */
UCLASS()
class WXGAME_API UWxTimeDilationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 실패 시 0. 반환한 핸들은 요청을 끝내는 쪽에서 반드시 해제한다. */
	uint64 AddRequest(float TimeDilation);
	void RemoveRequest(uint64 Handle);
	virtual void Deinitialize() override;

private:
	void ApplyCurrentRequest();

	TMap<uint64, float> Requests;
	uint64 NextHandle = 0;
	float OriginalDilation = 1.f;
};

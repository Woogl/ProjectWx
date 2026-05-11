// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WxCheckpointSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FWxOnLastCheckpointChangedSignature, const FTransform& /*NewTransform*/);

/**
 * 마지막으로 활성화된 체크포인트의 위치를 캐시하는 World Subsystem.
 * 체크포인트 액터는 활성화 시 본 서브시스템에 위치를 보고하고,
 * 부활 시스템은 GetLastCheckpoint() 로 조회한다.
 * 세이브 플러그인은 OnLastCheckpointChanged 델리게이트로 비동기 기록을, 로드 시 SetLastCheckpoint 로 캐시를 채운다.
 *
 * 싱글플레이 기준 단일 캐시. 멀티플레이로 확장 시 PC 별 분리가 필요하다.
 */
UCLASS()
class WXWORLD_API UWxCheckpointSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 마지막 체크포인트 위치 갱신. 체크포인트 활성화 또는 세이브 로드 시 호출. */
	void SetLastCheckpoint(const FTransform& InTransform);

	const FTransform& GetLastCheckpoint() const;

	bool HasValidCheckpoint() const;

	/** 마지막 체크포인트 변경 시 발사. 세이브 플러그인 등이 구독한다. */
	FWxOnLastCheckpointChangedSignature OnLastCheckpointChanged;

private:
	FTransform LastCheckpointTransform;

	bool bHasValidCheckpoint = false;
};

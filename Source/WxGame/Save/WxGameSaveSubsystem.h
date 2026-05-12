// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WxSaveSubsystem.h"
#include "WxGameSaveSubsystem.generated.h"

class UWxSlotData;

/**
 * 본 게임의 SaveSubsystem. WxSave 베이스를 상속해 게임 특화 데이터(체크포인트 등)를 다룬다.
 *
 * 책임:
 *  - 최초 게임 월드 진입 시 1회 AsyncLoadSlot.
 *  - 체크포인트 갱신 API 제공. SlotData 를 즉시 갱신하고 SaveSlot 으로 비동기 저장.
 *  - 부활 위치 조회 API 제공. PlayerController 가 Respawn 시 참조한다.
 */
UCLASS()
class UWxGameSaveSubsystem : public UWxSaveSubsystem
{
	GENERATED_BODY()

public:
	/** 체크포인트 활성화 시 호출. SlotData 를 갱신하고 비동기 저장한다. */
	void SetLastCheckpoint(const FTransform& Transform);

	bool HasValidCheckpoint() const;

	const FTransform& GetLastCheckpoint() const;

protected:
	virtual void OnGameWorldReady(UWorld* World) override;

private:
	bool bRequestedInitialLoad = false;
};

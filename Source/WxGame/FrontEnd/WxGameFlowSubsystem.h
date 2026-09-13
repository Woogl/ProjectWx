// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WxGameFlowSubsystem.generated.h"

class APawn;

/**
 * 프론트엔드에서 고른 폰과 목적지를 들고 맵을 열어, 도착한 GameMode 가 그 폰을 쓰게 한다.
 * 선택은 같은 맵의 재스폰에도 쓰이므로 목적지 월드에 있는 동안 유지되고, 다른 맵이 열리면 버려진다.
 */
UCLASS()
class WXGAME_API UWxGameFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 요청 접수 여부다. 도착 완료가 아니다. */
	bool RequestNewGame(TSoftClassPtr<APawn> PawnClass, TSoftObjectPtr<UWorld> Level);

	/** 목적지를 골라 뒀는데 아직 그 월드가 아니면 이동 중이다. */
	bool IsBusy() const;

	const FText& GetStatusText() const;

	/** 목적지 월드에서만 선택 폰을 돌려준다. */
	UClass* GetSelectedPawnClass(const UWorld* World) const;

private:
	void HandlePostLoadMap(UWorld* World);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& Error);
	bool IsDestinationWorld(const UWorld* World) const;
	bool IsWorldPackage(const UWorld* World, const TSoftObjectPtr<UWorld>& Map) const;

	UPROPERTY(Transient)
	TSoftClassPtr<APawn> PendingPawnClass;

	UPROPERTY(Transient)
	TSoftObjectPtr<UWorld> PendingLevel;

	FText StatusText;
};

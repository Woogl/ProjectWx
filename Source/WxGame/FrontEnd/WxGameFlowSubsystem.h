// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Engine/EngineBaseTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WxGameFlowSubsystem.generated.h"

class UWxExperienceDefinition;
struct FStreamableHandle;
class APawn;
class APlayerController;
class UCharacterMovementComponent;

UENUM()
enum class EWxTravelState : uint8
{
	Idle,
	Preparing,
	Traveling,
	AwaitingReady,
	Recovering,
	Failed
};

/** 실행 중 확정 상태. 디스크 저장과 객체 수명을 결합하지 않는다. */
USTRUCT()
struct FWxRunState
{
	GENERATED_BODY()

	UPROPERTY()
	TSoftClassPtr<APawn> PawnClass;

	UPROPERTY()
	TSoftObjectPtr<UWorld> Level;
};

UCLASS()
class WXGAME_API UWxGameFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool RequestNewGame(TSoftClassPtr<APawn> PawnClass, TSoftObjectPtr<UWorld> Level);
	void CancelPreparation();
	bool IsBusy() const;
	EWxTravelState GetTravelState() const;
	const FText& GetStatusText() const;
	const FWxRunState& GetRunState() const;
	bool IsDestinationWorld(const UWorld* World) const;
	UClass* GetSelectedPawnClass(const UWorld* World) const;
	bool ValidateArrival(const UWorld* World, const UWxExperienceDefinition* Experience);
	void HoldArrivalPawn(APlayerController* Controller);

private:
	bool HandleTick(float DeltaSeconds);
	void HandleAssetsLoaded(FGuid RequestId);
	void HandlePostLoadMap(UWorld* World);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& Error);
	void Fail(const FText& Reason);
	void BeginRecovery();
	void ClearPending();
	void ReleaseArrivalPawn();
	bool IsWorldPackage(const UWorld* World, const TSoftObjectPtr<UWorld>& Map) const;

	UPROPERTY(Transient)
	TSoftClassPtr<APawn> PendingPawnClass;

	UPROPERTY(Transient)
	TSoftObjectPtr<UWorld> PendingLevel;

	UPROPERTY(Transient)
	TSubclassOf<APawn> SelectedPawnClass;

	UPROPERTY(Transient)
	FWxRunState RunState;

	FGuid ActiveRequestId;
	// 도착 실패 시 ClearPending 이후에도 출발 맵으로 돌아갈 수 있어야 한다.
	FName ReturnLevelPackage;
	EWxTravelState State = EWxTravelState::Idle;
	FText StatusText;
	double Deadline = 0.0;
	bool bRecoveryRequired = false;
	bool bMovementTickWasEnabled = false;
	bool bPawnInputWasEnabled = false;
	TWeakObjectPtr<APawn> HeldPawn;
	TWeakObjectPtr<APlayerController> HeldController;
	TWeakObjectPtr<UCharacterMovementComponent> HeldMovement;
	TSharedPtr<FStreamableHandle> AssetHandle;
	FTSTicker::FDelegateHandle TickerHandle;
};

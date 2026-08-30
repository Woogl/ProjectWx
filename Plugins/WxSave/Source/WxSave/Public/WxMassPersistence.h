// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WxSaveGame.h"
#include "WxMassPersistence.generated.h"

DECLARE_DELEGATE(FWxOnMassPreSnapshot);
DECLARE_DELEGATE_OneParam(FWxOnMassSnapshotComplete, TArray<FWxMassEntityConfigGroupSnapshot>);
DECLARE_DELEGATE(FWxOnMassRestoreComplete);

/** Mass 처리 단계와 충돌하지 않는 FrameEnd 경계에서 영속 엔티티를 스냅샷·복원한다. */
UCLASS()
class WXSAVE_API UWxMassPersistence : public UObject
{
	GENERATED_BODY()

public:
	static void SnapshotEntities(
		const UObject* WorldContextObject,
		FWxOnMassPreSnapshot PreSnapshot,
		FWxOnMassSnapshotComplete OnComplete);

	static void RestoreEntities(
		const UObject* WorldContextObject,
		TArray<FWxMassEntityConfigGroupSnapshot> Snapshots,
		FWxOnMassRestoreComplete OnComplete);

private:
	static void DoSnapshotWork(UWorld& World, TArray<FWxMassEntityConfigGroupSnapshot>& OutSnapshots);
	static void DoRestoreWork(UWorld& World, TConstArrayView<FWxMassEntityConfigGroupSnapshot> Snapshots);
};

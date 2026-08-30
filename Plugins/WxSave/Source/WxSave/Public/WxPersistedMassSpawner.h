// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassSpawner.h"
#include "WxPersistedMassSpawner.generated.h"

/** 저장 복원 뒤 원래 MassSpawner가 같은 엔티티를 중복 생성하지 않도록 하는 영속 스포너. */
UCLASS(Blueprintable)
class WXSAVE_API AWxPersistedMassSpawner : public AMassSpawner
{
	GENERATED_BODY()

public:
	AWxPersistedMassSpawner();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual bool ShouldSpawnEntities() const;

	UFUNCTION()
	void HandleSpawningFinished();

	void StampOriginFragmentOnSpawnedEntities();

	/** LSP가 BeginPlay 전에 복원하여 재생성 여부를 결정한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx|Save")
	bool bHasEverSpawned = false;
};

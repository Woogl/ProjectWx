// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Burst.h"
#include "WxCueNotify_GhostTrail.generated.h"

class UPoseableMeshComponent;

UCLASS(Abstract)
class WXCOMBAT_API AWxGhostTrail : public AActor
{
	GENERATED_BODY()

public:	
	AWxGhostTrail();
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Wx")
	TObjectPtr<UPoseableMeshComponent> PoseableMesh;
};

UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API UWxCueNotify_GhostTrail : public UGameplayCueNotify_Burst
{
	GENERATED_BODY()

public:
	UWxCueNotify_GhostTrail();
	
	virtual void HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TSubclassOf<AWxGhostTrail> GhostTrailClass;
	
	// SetLifeSpan(0)은 "수명 없음"이라 0을 넣으면 잔상이 영구히 남는다.
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (ClampMin = "0.01"))
	float LifeSpan = 1.f;
};
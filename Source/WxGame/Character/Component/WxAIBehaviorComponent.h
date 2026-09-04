// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WxAIBehaviorComponent.generated.h"

class UBehaviorTree;

/** AIController가 실행할 행동 자산을 캐릭터 상속과 분리해 제공한다. */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXGAME_API UWxAIBehaviorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	UBehaviorTree* GetBehaviorTree() const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Wx|AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;
};

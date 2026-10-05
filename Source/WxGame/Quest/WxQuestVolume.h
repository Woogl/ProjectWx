// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "WxQuestVolume.generated.h"

class UStateTree;

/**
 * 플레이어 캐릭터가 처음 들어오면 지정 퀘스트를 수주시키는 볼륨.
 */
UCLASS()
class WXGAME_API AWxQuestVolume : public ATriggerBox
{
	GENERATED_BODY()

public:
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Wx|Quest")
	TObjectPtr<UStateTree> QuestAsset;
};

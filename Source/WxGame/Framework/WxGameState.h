// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"

#include "WxGameState.generated.h"

class UWxQuestComponent;

/**
 * 판 전체 상태를 드는 컴포넌트의 거주처다. 퀘스트 컴포넌트를 기본 서브오브젝트로 소유한다.
 */
UCLASS()
class WXGAME_API AWxGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AWxGameState();

private:
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxQuestComponent> QuestComponent;
};

// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"

#include "WxGameState.generated.h"

class UWxQuestComponent;
class UWxSkillCutsceneComponent;

UCLASS()
class WXGAME_API AWxGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AWxGameState();

private:
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxSkillCutsceneComponent> SkillCutsceneComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxQuestComponent> QuestComponent;
};

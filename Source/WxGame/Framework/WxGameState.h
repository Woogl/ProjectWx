// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"

#include "WxGameState.generated.h"

class UWxExperienceManagerComponent;

/**
 * ModularGameplay 컴포넌트 receiver 이자 Experience 매니저 컴포넌트의 거주처다.
 * GameMode(서버 전용)가 고른 Experience 는 매니저가 참조를 복제해 서버·클라 각자 로드·적용한다.
 */
UCLASS()
class WXGAME_API AWxGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AWxGameState();

	//~ Begin AActor
	virtual void PreInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor

	UWxExperienceManagerComponent* GetExperienceManagerComponent() const;

private:
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxExperienceManagerComponent> ExperienceManagerComponent;
};

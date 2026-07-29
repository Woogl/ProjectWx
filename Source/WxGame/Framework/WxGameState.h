// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"

#include "WxGameState.generated.h"

class UWxExperienceManagerComponent;

/**
 * 프로젝트 공용 GameState.
 *
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
	/** Experience 로드·적용의 주체. GameState 서브오브젝트라 서버가 고른 Experience 참조가 전 클라로 복제된다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxExperienceManagerComponent> ExperienceManagerComponent;
};

// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WxExperienceDefinition.generated.h"

class UGameFrameworkComponent;

/**
 * 게임 모드 하나의 프레임워크 구성을 정의하는 데이터 에셋 (Lyra Experience 의 축소 이식).
 * GameMode(서버 전용)가 선택해 GameState 로 넘기면, GameState 가 참조를 복제해 서버·클라가 각자 로컬 등록한다.
 * 어느 사이드에 붙을지는 여기서 지정하지 않는다 — 복제 컴포넌트는 엔진이 authority 로 제한하고, 그 밖의 사이드 제한은 컴포넌트가 스스로 한다.
 */
UCLASS()
class UWxExperienceDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/** ModularGameplay 프레임워크 컴포넌트 목록. 부착 대상은 컴포넌트 베이스(GameState/Pawn/Controller/PlayerState)로 자동 추론된다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TArray<TSubclassOf<UGameFrameworkComponent>> FrameworkComponents;
};

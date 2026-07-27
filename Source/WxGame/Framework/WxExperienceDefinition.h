// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WxExperienceDefinition.generated.h"

class UGameFrameworkComponent;

/**
 * Experience 가 주입하는 프레임워크 컴포넌트 한 항목.
 * 사이드 플래그 명명은 엔진 GameFeatures 의 FGameFeatureComponentEntry 를 따른다.
 */
USTRUCT()
struct FWxFrameworkComponentEntry
{
	GENERATED_BODY()

	FWxFrameworkComponentEntry();

	/** 주입할 컴포넌트. 부착 대상은 컴포넌트 베이스(GameState/Pawn/Controller/PlayerState)로 자동 추론된다. */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameFrameworkComponent> ComponentClass;

	/** 클라이언트에서 주입할지 여부. 복제 컴포넌트는 클라 생성이 막혀 있으므로 서버 주입 + 복제 도착이 정답이다. */
	UPROPERTY(EditDefaultsOnly)
	uint8 bClientComponent : 1;

	/** 서버에서 주입할지 여부. 리슨 호스트는 서버·클라 양쪽 엔트리를 모두 받는다. */
	UPROPERTY(EditDefaultsOnly)
	uint8 bServerComponent : 1;
};

/**
 * 게임 모드 하나의 프레임워크 구성을 정의하는 데이터 에셋 (Lyra Experience 의 축소 이식).
 * GameMode(서버 전용)가 선택해 GameState 로 넘기면, GameState 가 참조를 복제해 서버·클라 각자 자기 사이드 엔트리를 로컬 등록한다.
 */
UCLASS()
class UWxExperienceDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/** ModularGameplay 프레임워크 컴포넌트 목록. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (TitleProperty = "ComponentClass"))
	TArray<FWxFrameworkComponentEntry> FrameworkComponents;
};

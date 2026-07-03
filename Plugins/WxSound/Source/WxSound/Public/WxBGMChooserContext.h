// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "WxBGMChooserContext.generated.h"

class AActor;

/**
 * Chooser 평가에 넘기는 BGM 선택 컨텍스트(Struct Parameter).
 *
 * UWxMusicSubsystem 이 매 재평가 직전에 채워 Chooser 에 전달하며, 테이블의 각 컬럼이 아래 멤버에 바인딩된다.
 * (PlayerStateTags 는 Gameplay Tag 컬럼, BGMTag 는 Gameplay Tag 컬럼.)
 * 에디터에서 컬럼 바인딩 대상이 되려면 멤버가 모두 UPROPERTY 로 노출되어야 한다.
 *
 * 전투/보스 등 게임 상태도 별도 감지 없이 플레이어 ASC 의 owned-tag 로만 다룬다 — 그런 상태를 BGM 에 반영하려면
 * 해당 상태를 플레이어 ASC 에 태그로 부여하면 PlayerStateTags 에 그대로 잡힌다.
 */
USTRUCT(BlueprintType)
struct WXSOUND_API FWxBGMChooserContext
{
	GENERATED_BODY()

	// 로컬 플레이어 ASC 의 owned-tag 스냅샷(State.Dead / State.LockOn / State.Groggy 등).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wx|BGM")
	FGameplayTagContainer PlayerStateTags;

	// BP StartBGM 으로 주입된 BGM 분류 태그(탐험 / 전투 / 보스 / 마을 등). Chooser 가 이 키로 해당 행을 고른다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wx|BGM")
	FGameplayTag BGMTag;

	// 승자(가장 최근 활성) 소스의 소유자 액터. Chooser 의 Object Class 컬럼이 이 액터의 클래스로 행을 필터한다
	// (예: SubClassOf AWxBossCharacter → 보스곡). 활성 소스가 없으면 null.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wx|BGM")
	TObjectPtr<AActor> SourceOwner;
};

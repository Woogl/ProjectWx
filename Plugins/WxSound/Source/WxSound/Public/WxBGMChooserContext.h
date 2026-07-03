// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "WxBGMChooserContext.generated.h"

/**
 * Chooser 평가에 넘기는 BGM 선택 컨텍스트(Struct Parameter).
 *
 * UWxMusicSubsystem 이 매 재평가 직전에 채워 Chooser 에 전달하며, 테이블의 각 컬럼이 아래 멤버에 바인딩된다.
 * (세 멤버 모두 Gameplay Tag 컬럼.) 에디터에서 컬럼 바인딩 대상이 되려면 멤버가 모두 UPROPERTY 로 노출되어야 한다.
 * BGMTags/SourceOwnerTags 는 활성 소스가 여럿이면 그 태그들의 합집합이며, 서브시스템은 승자를 미리 고르지 않는다.
 * 우선순위 해소는 Chooser 테이블의 Row 순서(위→아래, 첫 매치 승)가 담당한다.
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

	// BGM 분류 태그 모음(탐험 / 전투 / 보스 / 마을 등). BP StartBGM 베이스라인 + 활성 소스들의 MusicTag 합집합.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wx|BGM")
	FGameplayTagContainer BGMTags;

	// 활성 소스 소유자(보스 등)들의 owned GameplayTag 합집합. Chooser 가 이 태그로 행을 필터한다
	// (예: Enemy.Boss.Dragon → 드래곤곡). 활성 소스가 없으면 비어 있다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wx|BGM")
	FGameplayTagContainer SourceOwnerTags;
};

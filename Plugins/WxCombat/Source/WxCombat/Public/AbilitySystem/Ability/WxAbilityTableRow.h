// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WxAbilityTableRow.generated.h"

class UTexture2D;

/**
 * 어빌리티 데이터테이블 Row 구조체.
 * 쿨다운, 충전, 코스트 등 어빌리티별 밸런스 수치와 UI 표시 데이터(아이콘)를 관리한다.
 * RowName 예시: GA_Skill_1, GA_Dodge
 */
USTRUCT(BlueprintType)
struct WXCOMBAT_API FWxAbilityTableRow : public FTableRowBase
{
	GENERATED_BODY()

	// ── Cooldown ───────────────────────────────────────────────────────────

	/** 쿨다운 시간(초). 0 이하이면 쿨다운 미적용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldown")
	float CooldownTime = 0.f;

	/** 최대 충전 수. 1이면 단일 쿨다운 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldown")
	int32 MaxRecharges = 1;

	// ── Cost ───────────────────────────────────────────────────────────────

	/** MP 소모량. 0 이하이면 미적용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost")
	float MPCost = 0.f;

	/** UP 소모량. 0 이하이면 미적용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost")
	float UPCost = 0.f;

	// ── Display ────────────────────────────────────────────────────────────

	/** UI 표시 아이콘. 비동기 로드 권장 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
	TSoftObjectPtr<UTexture2D> Icon;
};

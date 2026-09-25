---
title: "AnimNotify 공용 색상 분류와 에디터 설정"
source: ".agents/workflow/tasks/animnotify-categories.md"
type: notes
ingested: 2026-09-26
tags: [wx, animation, editor, foundation]
summary: "17종 노티파이의 6개 색상 분류, WxCore 공용 설정과 에디터 전용 보호 및 사람 확인 범위."
---

# AnimNotify 공용 색상 분류와 에디터 설정

[작업 기록](../../../.agents/workflow/tasks/animnotify-categories.md)의 확정 결정을 수집했다. 승인·상태 정본은 작업 기록이며 이 문서는 재사용할 구현 계약과 검증 범위만 보존한다.

## 확정 계약

- 공용 색상 설정은 WxCore의 UWxAnimNotifySettings에 두어 WxAI·WxInventory가 WxCombat을 참조하지 않고 사용한다.
- Config=Editor, DefaultConfig를 사용하며 프로젝트 설정의 Wx → Wx Anim Notify Settings에서 편집하고 DefaultEditor.ini에 저장한다.
- Attack #E86666: WeaponAttack, AreaDamage, SpawnProjectile, FinisherDamage.
- AbilityFlow #E8BE55: ComboWindow, StartRecovery, FinisherVictim.
- Effect #B58AE6: ApplyGameplayEffect, UseItem.
- Movement #62A9E8: Rush, SnapToTarget.
- Presentation #63C49A: CameraMove, SlowTime, SkillCutscene.
- Misc #929DAA: SpawnMinion, DespawnMinion, ReportNoise.
- HEX는 sRGB 값이다. FromSRGBColor로 선형 색상으로 변환한다. AreaDamage·SnapToTarget 미리보기도 공용 색상을 참조한다.
- 색상 필드와 초기화는 WITH_EDITORONLY_DATA, 17종 GetEditorColor 선언·정의와 설정 include는 WITH_EDITOR로 보호한다.
- WxCombatDeveloperSettings의 기존 색상 3개는 제거했고 피해 계산이 사용하는 DefenseConstant는 유지한다. 타임라인 라벨·Details 속성 분류·노티파이 실행 로직·에셋은 이 변경 대상이 아니다.

## 확인 범위

2026-09-26 정리 시 작업 기록의 AI 4개 항목과 사람 2개 항목이 모두 통과였다. 사람 항목은 이우성이 제출했고 서버가 반영한 결과이며, 이 정리에서 에디터를 재실행하거나 결과를 대신 판정하지 않았다. 코드 리뷰와 에디터 재시작 후 6개 색상·라벨 가독성·DefaultEditor.ini 저장 및 재로드가 사람 확인 범위다. 기존 Editor 빌드 로그는 build_2026-09-26_000757_357_2060.log의 Result Succeeded로 기록되어 있다. 패키지 빌드는 미실행이며 이번 문서 정리로 검증 범위를 확대하지 않는다.

## 설정 코드 확인

이번 정리에서 다음 헤더·cpp의 선언과 기본값을 읽었다. 게임 코드 수정과 코드 버전 대조는 하지 않았다.

### WxAnimNotifySettings.h

[WxAnimNotifySettings.h](../../../Plugins/WxCore/Source/WxCore/Public/WxAnimNotifySettings.h)

```cpp
// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WxAnimNotifySettings.generated.h"

UCLASS(Config = Editor, DefaultConfig, meta = (DisplayName = "Wx Anim Notify Settings"))
class WXCORE_API UWxAnimNotifySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UWxAnimNotifySettings();

#if WITH_EDITORONLY_DATA
	UPROPERTY(Config, EditAnywhere, Category = "Colors", meta = (HideAlphaChannel))
	FLinearColor AttackColor;

	UPROPERTY(Config, EditAnywhere, Category = "Colors", meta = (HideAlphaChannel))
	FLinearColor AbilityFlowColor;

	UPROPERTY(Config, EditAnywhere, Category = "Colors", meta = (HideAlphaChannel))
	FLinearColor EffectColor;

	UPROPERTY(Config, EditAnywhere, Category = "Colors", meta = (HideAlphaChannel))
	FLinearColor MovementColor;

	UPROPERTY(Config, EditAnywhere, Category = "Colors", meta = (HideAlphaChannel))
	FLinearColor PresentationColor;

	UPROPERTY(Config, EditAnywhere, Category = "Colors", meta = (HideAlphaChannel))
	FLinearColor MiscColor;
#endif
};
```

### WxAnimNotifySettings.cpp

[WxAnimNotifySettings.cpp](../../../Plugins/WxCore/Source/WxCore/Private/WxAnimNotifySettings.cpp)

```cpp
// Copyright Woogle. All Rights Reserved.

#include "WxAnimNotifySettings.h"

UWxAnimNotifySettings::UWxAnimNotifySettings()
{
	CategoryName = TEXT("Wx");
#if WITH_EDITORONLY_DATA
	// 팔레트의 HEX는 sRGB 값이므로 선형 색상으로 변환한다.
	AttackColor = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("E86666")));
	AbilityFlowColor = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("E8BE55")));
	EffectColor = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("B58AE6")));
	MovementColor = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("62A9E8")));
	PresentationColor = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("63C49A")));
	MiscColor = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("929DAA")));
#endif
}
```

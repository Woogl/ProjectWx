---
title: "모듈 리뷰의 재등록 수정과 콤보 배열 계약 확인"
source: "MANUAL"
type: notes
ingested: 2026-09-26
tags: [wx, game, combat, static-review]
summary: "ad0db6de0 작업 트리에서 확인한 콤보 몽타주 배열, AbilitySet 재부여, 캐릭터 일회성 처리와 새 게임 선택 검사 범위."
---

# 모듈 리뷰의 재등록 수정과 콤보 배열 계약 확인

2026-09-26 `module-review`의 모듈별 담당 에이전트가 HEAD `ad0db6de0`와 현재 작업 트리의 C++를 정적으로 대조한 관찰이다. 소스 수정·빌드·PIE·네트워크 실행·바이너리 에셋 검증은 수행하지 않았다. 과거 승인과 실행 결과는 [WxGame 작업 기록](../../../.agents/workflow/tasks/module_review_WxGame.md)에 보존하며 이번 실행 검증으로 승격하지 않는다.

## 콤보의 데이터 경계

WxCombat 담당자는 `UWxAbility_Combo`의 `ComboMontages`·`ComboIndex`·`GetMontage`가 공통 데이터를 소유함을 확인했다. Attack·Skill·Pattern의 단계 진행 동작은 각 타입에 남는다. 단계 선택을 하나의 몽타주에서 `1`, `2` 섹션으로 설명한 이전 Wiki는 현재 배열 계약을 설명하지 못한다. 전체 결함 목록은 [WxCombat 리뷰](../../../.agents/workflow/tasks/module_review_WxCombat.md)에 둔다.

확인 파일과 SHA-256:

| 파일 | SHA-256 |
| --- | --- |
| `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Combo.h` | `473403D5C9A56D09746650D3C26A4C581C396F00E3714B6F33EF2F071921F114` |
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Combo.cpp` | `931C0A164D7AC1830B1E59EDB666A1243D34DAC1B447175D284D495FF599641A` |
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp` | `680CD2C1858CAF779ED1E0249F281DBEF27688E125FA4D4654DA6E378E0E38FE` |

## 캐릭터 재등록과 새 게임

코드 근거: `WxCharacterBase.cpp:58`·`:66` 태그 구독 중복 검사, `:197` SPD 구독 검사, `:209` 세트 부여, `:265` 사망 일회성, `WxEnemyCharacter.cpp:135` 처치 일회성. `WxGameFlowSubsystem.cpp:51` 선택 검증 → `:58` 저장 삭제 → `:64` 선택 클래스 저장이며 헤더 `:42`의 UPROPERTY TSubclassOf가 유지한다. ASC `:48`~`:72`와 AbilitySet `:58`~`:72`의 누락 스펙 재부여도 대조했다. AbilitySet은 이미 가진 클래스를 런타임에 조용히 건너뛰며 중복 경고는 `IsDataValid`에만 있다.

| 파일 | SHA-256 |
| --- | --- |
| `Source/WxGame/Character/WxCharacterBase.cpp` | `4EBDFF4F625B657D05EF59767DDC43F3C5C790E97D6E2CBBD914863E583A8706` |
| `Source/WxGame/Character/WxEnemyCharacter.cpp` | `17EBB593F69658A7CA67308D9D24CAE60409DFB228A7C0F0B2CAD21A62B37AA7` |
| `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp` | `357F58E95A2421FE020B8EF9845F52E56F1B6F71F299A6DE917E6D0D55E01822` |
| `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h` | `F31E80318B7449315CB02174D0E4BBD08127AC371FA4498A85BE237480EA88DF` |

WxGame 담당자는 기존 세 지적에 대한 수정 경로를 확인했다. ASC는 실제 스펙이 없는 능력을 재부여하고 최초 속성·효과 초기화와 구분한다. 캐릭터 사망·래그돌 구독은 중복을 막으며 사망 처리·처치 통지는 객체 수명당 한 번으로 제한한다. 새 게임은 체크포인트 삭제 전에 선택 클래스를 로드하고 Pawn 상속·생성 가능 여부를 검사한다.

이는 이전 `39f3629a4` 리뷰의 미수정 설명을 대체하는 현재 코드 관찰이다. 실제 레벨 재표시·보상·새 게임 목적지 진입의 사람 테스트는 기존 기록에서 여전히 대기이며, 정적 리뷰가 이를 통과로 바꾸지 않는다.

## 방향 섹션의 추가 대조

WxCombat 담당자가 다시 확인했다. `WxAbility_Combo.h:19`~`:24`, cpp `:6`~`:9`가 배열 선택을 정의하고 Attack.cpp:26·Skill.cpp:31은 재발동, Pattern.cpp:47~55는 블렌드아웃에서 단계를 진행한다. GetComboStageCount·GetComboStageSection은 WxCombat 소스 검색 0건이다.

`WxAbilityBase.cpp:94`~`:132`는 몸 기준 8방향을 골라 접두사+방향명, 접두사+Forward, NAME_None 순으로 대체한다. `:339`~`:382`는 명시 섹션이 존재하면 그대로 사용하며, 없을 때 접두사+Forward가 있으면 공통 방향 선택을 사용한다. 필요한 원격 서버 인스턴스는 클라이언트 방향 데이터를 기다린다. Dodge의 Backstep·Success와 GuardReact의 GuardHit·GuardKnockback·GuardBreak·PerfectGuard 이름은 유지되며, 정확한 반응 섹션이 없고 반응명+Forward가 있으면 공통 방향 선택이 적용된다. 피격의 HasMontageSection도 정확한 이름 또는 이름+Forward를 허용한다. 이 내용은 C++ 관찰이며 에셋 구성·네트워크 실행을 검증한 결과가 아니다.

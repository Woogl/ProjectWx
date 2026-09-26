---
type: source
title: "결정 노트 - 2026-09-24-module-principles"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "모듈"
  - "아키텍처"
summary: "Wx 플러그인 모듈화 목적을 재사용에서 게임 내부 도메인 경계 강제로 다시 정하고 책임 기반 배치 원칙을 확정한 대화 기록"
source_type: decision-note
source_id: src-f61648c1a135b8f2157a
sha256: c5ddf5b757a5dd4996e60000b51e93895d597bc550a3ab7e46244a765926271c
authority: primary
independence_key: ".wiki/raw/notes/2026-09-24-module-principles.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-24-module-principles.md"
raw_copy: ".raw/captured/c5ddf5b757a5dd4996e60000b51e93895d597bc550a3ab7e46244a765926271c.md"
claim_ids:
  - clm-ea855c6222-c1
  - clm-ea855c6222-c2
  - clm-ea855c6222-c3
  - clm-ea855c6222-c4
key_claims:
  - "사용자는 2026-09-24 Wx 플러그인의 처음 목적이 다른 프로젝트 재사용이었으나 그 목적이 흔들린다고 말했다."
  - "확정된 모듈 배치 원칙은 코드 배치를 참조 모듈 수가 아니라 그 코드를 고치게 만드는 기획 변경이 속한 도메인(책임)으로 정한다."
  - "WxCore에 여러 도메인이 쓰는 상태 없는 헬퍼를 두는 것은 허용으로 반영됐다."
  - "다른 도메인의 베이스 클래스를 상속해야 하는 코드(UWxAbility_UseItem 등)의 배치 원칙은 사용자가 보류했다."
---

# 결정 노트 - 2026-09-24-module-principles

- 원본: `.wiki/raw/notes/2026-09-24-module-principles.md`
- 원자료 사본: `.raw/captured/c5ddf5b757a5dd4996e60000b51e93895d597bc550a3ab7e46244a765926271c.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 옛 LLM Wiki 결정 노트. frontmatter: 제목 "모듈화 목적 재정의와 배치 원칙 정립", 출처 `MANUAL`, 수집일 2026-09-24.
- 2026-09-24 대화 기록이다. Nameplate를 WxGame으로 옮긴 작업(`6c13b2e43`)을 돌아보다 나왔다([[결정 노트 - 2026-09-24-nameplate-manager-wxgame]]).
- 노트의 "확인한 사실"은 `*.Build.cs`·헤더·git 이력을 읽은 **정적 조사(빌드·실행 검증 아님)**이며, 노트 날짜 기준이라 현재 코드와 다를 수 있다.

## 사람의 판단 원문

> 사용자 2026-09-24: "지금 제가 플러그인화를 하는 목적이 좀 흔들리고 있어요. 처음에는 다른 프로젝트에서도 가져다 쓰기 위함이었어요. 그런데 작업하다보니까 과연 다른 프로젝트에서도 이 구조를 가져다쓸지가 의문스러워지긴 하더라구요"

> 사용자 2026-09-24: "그렇다고 해서 플러그인 구조를 벗어나려고 하니, 지금까지 만든게 아깝고, 나름대로 잘 모듈화된 것 같아서 고민이 되요"

> 사용자 2026-09-24: "그럼 저에게 맞는 모듈화에 대한 원칙을 견고하게 세워주세요."

> 사용자 2026-09-24: "이것이 원칙으로서 적절한가요?" / "이것으로 원칙 확정해도 문제 없을까요?"

> 사용자 2026-09-24: "보완할 것 1은 예전부터 고민이 많이 되는 부분입니다. 이 부분은 좀 더 생각해보고 결정하겠습니다. 보완할 것 2는 반영합니다."

## 구현 관찰(정적, 노트 시점)

- 도메인 플러그인 7개(WxAI·WxCombat·WxDialogue·WxInventory·WxQuest·WxUI·WxWorld)는 Wx 모듈 중 WxCore만 의존한다. WxGame은 7개 모두와 WxCore를, WxEditor는 WxCore·WxInventory·WxUI·WxWorld를 의존한다.
- `WxGameplayTags.h`에 네이티브 태그 선언 117개가 있고 상당수가 이 게임 규칙이다(`Master.Doppelganger`, `Device.Elevator.*`, `Effect.PerfectGuard` 등).
- 플러그인 `Content`의 `.uasset`은 WxUI·WxWorld에 1개씩뿐이다. 통합 플러그인 `WxSave`는 2026-09-01(`b3f982b0d`)에 삭제됐다.

## 에이전트 판단(사용자에게 제시)

- Wx 플러그인은 사실상 이 게임 전용이며, 플러그인 구조의 실제 가치는 재사용이 아니라 경계를 컴파일 단계에서 강제하는 것(도메인끼리 모름, 의존 방향 고정, 빌드 격리)이다.
- "언젠가 재사용"을 기준으로 두면 수요 없는 범용성 장치(`LockOnTargetQuery` 델리게이트, 마커 컴포넌트)를 정당화하게 된다. 구조는 유지한다.

## 초안 반증과 확정 결정

에이전트가 초안을 기존 결정·코드에 대 보고 어긋난 점: `IWxSpawnable`이 "도메인 둘 이상 참여" 계약 조건의 반례, "참조 모듈 수로 배치"는 WxGame 클래스 8개를 잘못 걸러냄, "태그 > 조립 > 계약" 우선순위는 `IWxSpawnable` 때 계약 통지를 택한 판단과 충돌, "중간 통합 플러그인 금지"는 보류된 GameFeature 확장 계획과 충돌, WxCore "정의만"은 `FWxLocatorUtils`와 충돌, 베이스 클래스 상속(`UWxAbility_UseItem`·`UWxAbility_Interact`가 `UWxAbilityBase` 상속 때문에 WxGame에 있음)은 통로로 대신할 수 없음.

확정된 원칙:
- 배치는 책임으로 정한다. 시험 질문은 "이 코드를 고치게 만드는 기획 변경은 어느 도메인의 것인가"다.
- 통로는 성격으로 고른다.
- 계약 조건은 "소비자의 책임이 한 도메인 안"으로 바꿨다.
- 기능 단위 플러그인은 조립 계층으로 허용한다.
- WxCore에 여러 도메인이 쓰는 상태 없는 헬퍼를 허용한다(보완 2, 반영).

## 미결정·충돌

- 다른 도메인 베이스 클래스를 상속해야 하는 코드의 배치(보완 1)는 사용자가 보류했다.

## 관련 주제

- [[모듈 구조와 코드 정리]]
- [[어빌리티와 GAS]]

## 핵심 주장

- 사용자는 2026-09-24 Wx 플러그인의 처음 목적이 다른 프로젝트 재사용이었으나 그 목적이 흔들린다고 말했다. ^c1
- 확정된 모듈 배치 원칙은 코드 배치를 참조 모듈 수가 아니라 그 코드를 고치게 만드는 기획 변경이 속한 도메인(책임)으로 정한다. ^c2
- WxCore에 여러 도메인이 쓰는 상태 없는 헬퍼를 두는 것은 허용으로 반영됐다. ^c3
- 다른 도메인의 베이스 클래스를 상속해야 하는 코드(UWxAbility_UseItem 등)의 배치 원칙은 사용자가 보류했다. ^c4

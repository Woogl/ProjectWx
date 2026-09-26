---
type: source
title: "기획서 - WX_BT_기능정리"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "기획서"
  - "적AI"
  - "BehaviorTree"
summary: "WX 자체 BT 기능으로 균등 확률 랜덤 컴포지트 RandomChoice와 능력치 비율 비교 데코레이터 CompareAttributeRatio를 정리하고 작성 규칙을 둔 문서"
source_type: design-doc
source_id: src-016c4f4d42be310c6137
sha256: 5ef1e5f8941b0be03a5211990189016c1ecc851744385b7e84cd0b9c9995c784
authority: primary
independence_key: "Docs/CombatDesign/WX_BT_기능정리.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - "Docs/CombatDesign/WX_BT_기능정리.md"
raw_copy: ".raw/captured/5ef1e5f8941b0be03a5211990189016c1ecc851744385b7e84cd0b9c9995c784.md"
claim_ids:
  - clm-931eb3b32f-c1
  - clm-931eb3b32f-c2
  - clm-931eb3b32f-c3
key_claims:
  - "WX_BT_기능정리 문서는 RandomChoice를 하위 노드 중 하나를 균등 확률로 무작위 선택해 실행하는 BT 컴포지트로 정의하고, AvoidRepeat 옵션으로 직전 노드를 제외할 수 있다고 적는다."
  - "WX_BT_기능정리 문서는 CompareAttributeRatio를 분자 능력치를 분모 능력치로 나눈 비율을 상수와 비교해 하위 노드 실행을 허용하는 BT 데코레이터로 정의한다."
  - "WX_BT_기능정리 문서는 현재 WX 자체 개발 BT 태스크가 없다고 적는다."
---

# 기획서 - WX_BT_기능정리

- 원본: `Docs/CombatDesign/WX_BT_기능정리.md`
- 원자료 사본: `.raw/captured/5ef1e5f8941b0be03a5211990189016c1ecc851744385b7e84cd0b9c9995c784.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

`Docs/CombatDesign/WX_BT_기능정리.md`는 WX가 자체 개발한 Behavior Tree 기능의 목적과 사용법을 기록하는 기능 설명 문서다. 자체 기능 문서를 BT·AM·GA 세 종류로 나누며, 이 문서는 BT를 맡는다. AM 쪽은 [[기획서 - WX_AM_기능정리]]다.

## BT 컴포지트: `RandomChoice` (기능 명세)

- `Selector`와 비슷하게 하위 노드 하나를 골라 실행하지만, 위에서 아래로 차례로 평가하지 않고 **균등한 확률로 무작위 선택**한다.
- 옵션 `AvoidRepeat`: 켜면 직전에 실행한 하위 노드를 다음 선택 대상에서 뺀다.
- 목적: 몬스터 행동 패턴이 늘 같은 순서로 반복되지 않게 하고, 같은 패턴이 연속으로 나오지 않게 제어한다.

## BT 데코레이터: `CompareAttributeRatio` (기능 명세)

- 분자 능력치와 분모 능력치를 지정해 `분자 / 분모` 비율을 계산하고, 설정한 상수와 비교해 조건을 만족하면 연결된 노드 실행을 허용한다.
- 사용 예시: 현재 HP / 최대 HP가 50% 이하일 때 특정 패턴 실행, MP 비율에 따른 스킬 사용, 스태거 게이지 비율에 따른 행동 변경.
- 목적: 절대 수치가 아니라 현재 상태의 비율로 행동을 고르게 해 체력 구간별 패턴 변경, 그로기 직전 판단 같은 분기를 BT에서 처리한다.

## BT 태스크

현재 WX 자체 BT 태스크는 없다고 적혀 있다. 추가되면 기능 개요·기본 동작·세부 옵션·사용 목적 양식으로 정리한다.

## 작성 규칙

새 BT 기능은 기능 개요, 기본 동작(단계별 흐름), 세부 옵션(디테일 패널 설정), 사용 목적, 사용 예시 항목으로 정리한다.

## 미결정·충돌

- `CompareAttributeRatio`의 비교 연산자 종류와 분모가 0일 때의 처리는 적혀 있지 않다.
- `RandomChoice`는 "균등한 확률"만 정의하고 가중치 옵션은 없다. 가중치 선택이 필요한 패턴 설계가 있다면 이 기능으로는 부족할 수 있다.
- 사용 예시의 "스태거 게이지"가 다른 기획서의 그로기/DP 게이지와 같은 것인지 문서만으로는 알 수 없다.

## 관련 주제

- [[적 AI와 몬스터]]
- [[보스 전투]]
- [[기획서 - WX_AM_기능정리]]
- <!--wl-->기획서 - normal_monster_BT

## 핵심 주장

- WX_BT_기능정리 문서는 RandomChoice를 하위 노드 중 하나를 균등 확률로 무작위 선택해 실행하는 BT 컴포지트로 정의하고, AvoidRepeat 옵션으로 직전 노드를 제외할 수 있다고 적는다. ^c1
- WX_BT_기능정리 문서는 CompareAttributeRatio를 분자 능력치를 분모 능력치로 나눈 비율을 상수와 비교해 하위 노드 실행을 허용하는 BT 데코레이터로 정의한다. ^c2
- WX_BT_기능정리 문서는 현재 WX 자체 개발 BT 태스크가 없다고 적는다. ^c3

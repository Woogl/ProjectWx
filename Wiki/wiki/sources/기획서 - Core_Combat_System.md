---
type: source
title: "기획서 - Core_Combat_System"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "기획서"
  - "전투"
  - "자원"
summary: "HP·MP·UP 3대 자원과 일반공격·강공격·가드/패링·회피/극한회피·스킬 4종·궁극기의 6대 핵심 액션을 정의한 초기 공통 전투 기획안"
source_type: design-doc
source_id: src-29fa83e3956cdfe188e5
sha256: 903549ef1c2eb6c61f79473b70780cba03ab51cf01afa6e951362b65652d8ccb
authority: primary
independence_key: "Docs/SystemDesign/Core_Combat_System.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - "Docs/SystemDesign/Core_Combat_System.md"
raw_copy: ".raw/captured/903549ef1c2eb6c61f79473b70780cba03ab51cf01afa6e951362b65652d8ccb.md"
claim_ids:
  - clm-21115045a0-c1
  - clm-21115045a0-c2
  - clm-21115045a0-c3
key_claims:
  - "Core_Combat_System 기획서는 HP·MP·UP를 3대 핵심 자원으로 두고 HP의 자연 회복을 두지 않는다."
  - "Core_Combat_System 기획서는 UP가 MP 소모 스킬이 적에게 적중했을 때만 충전되도록 요구한다."
  - "Core_Combat_System 기획서는 패링(Just Guard) 성공 시 피해 0, MP 대량 수급, UP 소량 수급, 적 경직을 요구한다."
---

# 기획서 - Core_Combat_System

- 원본: `Docs/SystemDesign/Core_Combat_System.md`
- 원자료 사본: `.raw/captured/903549ef1c2eb6c61f79473b70780cba03ab51cf01afa6e951362b65652d8ccb.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

`Docs/SystemDesign/Core_Combat_System.md`는 프로젝트(문서상 이름: 판타지 아포칼립스)의 공통 전투 기조와 시스템 규격을 정의한 기획안이다. 모든 캐릭터와 몬스터 설계의 기반으로 선언되어 있다. 장르는 3인칭 백뷰 고속 액션 RPG, 배경은 마법과 현대 기기가 공존하는 판타지 아포칼립스, 핵심 기조는 총알을 피하고 날아오는 마법을 베어내는 초인적 전사의 감각이다.

## 3대 핵심 자원 (요구사항)

| 자원 | 수급 | 소모 |
|---|---|---|
| HP | 아이템·특정 스킬로만 회복(자연 회복 없음) | 피격 시 감소, 0이면 사망 |
| MP | 평타 적중, 패링 성공, 극한 회피 성공 | 캐릭터 고유 스킬 4종 발동 |
| UP | MP 소모 스킬이 적에게 적중했을 때만 충전 | 최대치에서 전량 소모해 궁극기 발동 |

## 6대 핵심 액션 (요구사항)

- 일반 공격: 가장 빠른 기본 공격이며 MP 수급의 주력 수단.
- 강공격: 느리지만 강하고 넓다. 평타 콤보 중 특정 타이밍 입력 시 특수 파생기(Branching).
- 가드·패링: 일반 가드는 피해 일부 경감, MP/UP 수급 없음. 패링(Just Guard)은 적중 직전 가드 시 발동하며 피해 0 + MP 대량 + UP 소량 + 적 경직.
- 회피·극한 회피: 회피는 짧은 무적 프레임 이동. 극한 회피는 적중 직전 회피 시 슬로우 모션 + MP + UP 소량.
- 스킬: 캐릭터당 고유 4종, MP 소모, 적중할 때마다 UP 충전.
- 궁극기: UP 100%에서만 사용, UP 전량 소모.

## 연출·피드백 (요구사항)

Hit-stop, 투사체 절단·반사(Projectile Counter), 스킬 후딜레이를 회피나 다른 스킬로 캔슬하는 Cancel System을 필수 요소로 든다.

## 전투 루프

평타·패링·회피로 MP 확보 → MP로 4종 스킬 연계해 UP 충전 → UP로 궁극기 발동.

## 미결정·충돌

- UP 수급 조건: 이 문서는 MP 소모 스킬 적중 시에만 충전된다고 적지만, [[기획서 - PC규격서]]는 스킬 발동 혹은 적중 시 충전하며 조건은 캐릭터별 문서에서 정한다고 적는다.
- 가드·저스트 가드 패링, 스킬 4종 구조는 [[기획서 - 전투 방향 변경에  따른 시스템 수정 요구사항]]에서 가드 제거, 공격 적중형 패링, E 단일 키 스킬로 바뀌었다.
- [[기획서 - WA_PC_규격서]]는 MP 대신 스태미나·궁극기 게이지·고유 자원 체계를 쓰므로 3대 자원 구성과 다르다.

## 관련 주제

- [[게임 개요와 전투 방향]]
- [[캐릭터 스탯과 전투 자원]]
- [[플레이어 캐릭터와 조작]]

## 핵심 주장

- Core_Combat_System 기획서는 HP·MP·UP를 3대 핵심 자원으로 두고 HP의 자연 회복을 두지 않는다. ^c1
- Core_Combat_System 기획서는 UP가 MP 소모 스킬이 적에게 적중했을 때만 충전되도록 요구한다. ^c2
- Core_Combat_System 기획서는 패링(Just Guard) 성공 시 피해 0, MP 대량 수급, UP 소량 수급, 적 경직을 요구한다. ^c3

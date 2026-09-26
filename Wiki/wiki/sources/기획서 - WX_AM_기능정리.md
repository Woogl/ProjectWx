---
type: source
title: "기획서 - WX_AM_기능정리"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "기획서"
  - "애니메이션"
  - "GAS"
  - "AnimNotify"
summary: "WX 전용 애님 노티파이 2종(투사체 생성·범위 공격)과 노티파이 시퀀스 5종(콤보 창·무적·퍼펙트 가드·타겟 스냅·무기 공격)의 기능을 정리한 문서"
source_type: design-doc
source_id: src-40ec76dd3189bb392845
sha256: 6e39decd79a407896d145cd14ef97abfd7feaf7cf6f2073376b04e3b5a35c9e7
authority: primary
independence_key: "Docs/CombatDesign/WX_AM_기능정리.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - "Docs/CombatDesign/WX_AM_기능정리.md"
raw_copy: ".raw/captured/6e39decd79a407896d145cd14ef97abfd7feaf7cf6f2073376b04e3b5a35c9e7.md"
claim_ids:
  - clm-fb7c901ba4-c1
  - clm-fb7c901ba4-c2
  - clm-fb7c901ba4-c3
key_claims:
  - "WX_AM_기능정리 문서는 WX 자체 애님 노티파이로 WX_Spawn_Projectile과 WX_Area_Attack 두 가지를, 노티파이 시퀀스로 WX_Combo_Window, WX_Invincible, WX_Perfect_Guard, WX_Snap_To_Target, WX_Weapon_Attack 다섯 가지를 정의한다."
  - "WX_AM_기능정리 문서는 피해를 주는 노티파이가 데미지 계수·피격 판정·히트 리액션을 DamageDataRow(테이블) 또는 DamageInfo(테이블 없는 에셋) 중 하나로 설정할 수 있다고 적는다."
  - "WX_AM_기능정리 문서는 WX_Perfect_Guard가 가드 어빌리티 중 패링 판정 가능 프레임 구간을 지정하고, 그 구간에 막은 공격을 패링으로 처리한다고 적는다."
---

# 기획서 - WX_AM_기능정리

- 원본: `Docs/CombatDesign/WX_AM_기능정리.md`
- 원자료 사본: `.raw/captured/6e39decd79a407896d145cd14ef97abfd7feaf7cf6f2073376b04e3b5a35c9e7.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

`Docs/CombatDesign/WX_AM_기능정리.md`는 언리얼 엔진 순정 기능이 아니라 WX가 자체 개발한 애님 몽타주(AM)용 애님 노티파이(AN)와 애님 노티파이 시퀀스(ANS)의 목적과 사용법을 정리한 기능 설명 문서다. 기획서가 요구하는 기능 명세로 읽는다. 짝 문서로 [[기획서 - WX_BT_기능정리]]가 있으며, 그 문서는 자체 기능 문서를 BT·AM·GA 세 종류로 나눈다고 밝힌다.

## 애님 노티파이 (AN)

| 이름 | 기능 |
|---|---|
| `WX_Spawn_Projectile` | 지정한 프레임에 투사체를 생성한다. 디테일 패널에서 투사체와 투사체 데미지를 설정한다 |
| `WX_Area_Attack` | 지정한 프레임에 타게팅 프리셋에 포함된 모든 범위에 피해를 준다 |

## 애님 노티파이 시퀀스 (ANS)

| 이름 | 기능 |
|---|---|
| `WX_Combo_Window` | GA에 지정된 다음 콤보로 넘어갈 수 있는 유효 프레임 구간을 지정한다. 구간 안에서 해당 GA와 같은 키를 누르면 다음 콤보 AM을 실행한다 |
| `WX_Invincible` | 구간 동안 AM을 실행 중인 캐릭터를 무적으로 만든다. 회피·특수 동작·특정 스킬의 무적 구간에 쓴다 |
| `WX_Perfect_Guard` | 가드 어빌리티 중 패링 판정이 가능한 구간을 지정한다. 이 구간에 적 공격을 막으면 패링으로 처리할 수 있다 |
| `WX_Snap_To_Target` | 구간 동안 캐릭터가 타겟 프리셋 규칙에 따라 고른 대상을 바라보게 해 공격 방향을 보정한다 |
| `WX_Weapon_Attack` | 구간 동안 캐릭터가 든 무기에 피격 판정을 부여한다. 무기 근접 공격 판정에 쓴다 |

## 데미지 설정 방식

피해를 주는 세 기능(`WX_Spawn_Projectile`, `WX_Area_Attack`, `WX_Weapon_Attack`)은 데미지 계수, 피격 판정, 히트 리액션 등을 두 방법 중 하나로 설정한다고 적혀 있다.

- `DamageDataRow`: 데이터 테이블 행을 사용한다.
- `DamageInfo`: 테이블 없이 에셋에서 직접 설정한다.

## 미결정·충돌

- 문서는 각 노티파이의 목적만 적고, 설정 항목의 정확한 이름·기본값, `DamageDataRow`와 `DamageInfo`를 둘 다 채웠을 때 무엇이 우선하는지는 적지 않았다.
- `WX_Combo_Window`는 "해당 GA와 같은 키"로 다음 콤보를 받는다고만 적혀 있어, 선입력 버퍼 여부 같은 입력 처리 세부는 문서만으로 알 수 없다.
- 문서 속 이름은 기능 표기명(`WX_` 접두사)이다. 실제 클래스 이름과 같은지는 이 문서로 확인할 수 없다.

## 관련 주제

- [[어빌리티와 GAS]]
- [[피해 파이프라인]]
- [[플레이어 캐릭터와 조작]]
- [[기획서 - WX_BT_기능정리]]

## 핵심 주장

- WX_AM_기능정리 문서는 WX 자체 애님 노티파이로 WX_Spawn_Projectile과 WX_Area_Attack 두 가지를, 노티파이 시퀀스로 WX_Combo_Window, WX_Invincible, WX_Perfect_Guard, WX_Snap_To_Target, WX_Weapon_Attack 다섯 가지를 정의한다. ^c1
- WX_AM_기능정리 문서는 피해를 주는 노티파이가 데미지 계수·피격 판정·히트 리액션을 DamageDataRow(테이블) 또는 DamageInfo(테이블 없는 에셋) 중 하나로 설정할 수 있다고 적는다. ^c2
- WX_AM_기능정리 문서는 WX_Perfect_Guard가 가드 어빌리티 중 패링 판정 가능 프레임 구간을 지정하고, 그 구간에 막은 공격을 패링으로 처리한다고 적는다. ^c3

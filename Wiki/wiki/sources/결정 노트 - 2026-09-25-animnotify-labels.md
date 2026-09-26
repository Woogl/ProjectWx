---
type: source
title: "결정 노트 - 2026-09-25-animnotify-labels"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "애니메이션"
  - "에디터"
summary: "AnimNotify 17종의 타임라인 라벨을 종류와 대표 값 하나 형식으로 줄이기로 한 사용자 합의와 GetNotifyName 구현 근거를 정적으로 정리한 기록"
source_type: decision-note
source_id: src-9aac608619f197cba225
sha256: 4b5acbf3f00b05230afcaf1ef7042c588cad07a28afa5fafd1d9ea861c8e3083
authority: primary
independence_key: ".wiki/raw/notes/2026-09-25-animnotify-labels.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-25-animnotify-labels.md"
raw_copy: ".raw/captured/4b5acbf3f00b05230afcaf1ef7042c588cad07a28afa5fafd1d9ea861c8e3083.md"
claim_ids:
  - clm-75ff3eef15-c1
  - clm-75ff3eef15-c2
  - clm-75ff3eef15-c3
  - clm-75ff3eef15-c4
key_claims:
  - "2026-09-25 사용자는 AnimNotify 타임라인 라벨을 종류와 대표 값 하나 형식으로 줄이는 안을 승인했다."
  - "AnimNotify 라벨 규칙은 Row·에셋 이름을 자르지 않고 클래스 이름 끝의 _C만 제거하며 미설정 값은 None으로 표시한다."
  - "노트 시점 UWxAnimNotifyState_SlowTime 라벨은 소수점 두 자리 배율, UWxAnimNotify_ReportNoise 라벨은 cm 거리로 표시된다."
  - "이 노트는 코드 정적 확인이며 에디터 화면에서의 라벨 표시는 검증하지 않았다."
---

# 결정 노트 - 2026-09-25-animnotify-labels

- 원본: `.wiki/raw/notes/2026-09-25-animnotify-labels.md`
- 원자료 사본: `.raw/captured/4b5acbf3f00b05230afcaf1ef7042c588cad07a28afa5fafd1d9ea861c8e3083.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 옛 LLM Wiki 결정 노트. frontmatter: 제목 "AnimNotify 타임라인 이름 축약 규칙", 출처 `MANUAL`, 수집일 2026-09-25.
- 작업 기록은 [[작업 - animnotify-labels]]이다(빌드 결과는 그쪽에 따로 있다고 노트가 적음).
- 이 노트는 코드 **정적 조사(빌드·실행 검증 아님)**이며 에디터 화면 검증을 포함하지 않는다. 각 파일의 SHA-256을 기록했고, 노트 날짜 기준이라 현재 코드와 다를 수 있다.

## 확정 결정(사용자 합의, 2026-09-25)

- 사용자가 긴 라벨 제안을 줄이도록 요청하고 `종류: 대표 값 하나` 형식을 승인했다. 노트에는 사용자 발언 원문이 없고 요약만 있다.
- 소켓·프리셋·오프셋·정지 거리·FOV는 라벨에 넣지 않고 Details에서 확인한다.
- Row·에셋 이름을 임의로 자르지 않으며 클래스 이름 끝 `_C`만 제거한다. 미설정은 `None`, 스냅 이동·회전 모두 비활성은 `Off`다.

## 라벨 규칙

- 표시 종류: Attack, Area, Finisher, Victim, Cutscene, Projectile, Summon, Despawn, Effect, Slow, Rush, Snap, Camera, Noise.
- 고정 표식: `Recovery`, `Combo Window`, `Use Item`.
- 구분 값: Rush는 `LockOn`/`Master`/`Minion`, Snap은 `Move`/`Turn`/`Move+Turn`/`Off`, Camera는 `Follow`/`Fixed`. Slow는 소수점 두 자리 배율(`Slow: x%.2f`), Noise는 cm 거리(`Noise: %scm`).

## 구현 관찰(정적)

`GetNotifyName_Implementation`을 재정의한 클래스와 대표 값:

| 클래스 | 라벨 |
|---|---|
| `UWxAnimNotifyState_WeaponAttack` | `Attack: <DamageDataRow 행 이름>` |
| `UWxAnimNotify_AreaDamage` | `Area: <행 이름>` |
| `UWxAnimNotify_FinisherDamage` | `Finisher: <행 이름>` |
| `UWxAnimNotify_FinisherVictim` | `Victim: <VictimMontage 이름>` |
| `UWxAnimNotify_SkillCutscene` | `Cutscene: <Sequence 이름>` |
| `UWxAnimNotify_SpawnProjectile` / `SpawnMinion` / `DespawnMinion` | `Projectile:` / `Summon:` / `Despawn:` + 클래스 이름(`_C` 제거) |
| `UWxAnimNotifyState_ApplyGameplayEffect` | `Effect: <GE 클래스 이름>` |
| `UWxAnimNotifyState_SlowTime` | `Slow: x<TimeDilation>` |
| `UWxAnimNotifyState_Rush` | `Rush: <TargetSource>` |
| `UWxAnimNotifyState_SnapToTarget` | `Snap: <bSnapLocation·bSnapRotation 조합>` |
| `UWxAnimNotifyState_CameraMove` | `Camera: Follow`(bAttachToOwner) / `Camera: Fixed` |
| `UWxAnimNotify_ReportNoise`(WxAI) | `Noise: <HearingDistance>cm` |
| `UWxAnimNotify_StartRecovery` / `UWxAnimNotifyState_ComboWindow` / `UWxAnimNotify_UseItem`(WxInventory) | 고정 문자열 |

## 검증 범위

- 정적 코드 확인뿐이다. 에디터 타임라인 화면 표시는 이 노트에서 검증하지 않았다.

## 관련 주제

- [[에디터 도구]]
- [[어빌리티와 GAS]]

## 핵심 주장

- 2026-09-25 사용자는 AnimNotify 타임라인 라벨을 종류와 대표 값 하나 형식으로 줄이는 안을 승인했다. ^c1
- AnimNotify 라벨 규칙은 Row·에셋 이름을 자르지 않고 클래스 이름 끝의 _C만 제거하며 미설정 값은 None으로 표시한다. ^c2
- 노트 시점 UWxAnimNotifyState_SlowTime 라벨은 소수점 두 자리 배율, UWxAnimNotify_ReportNoise 라벨은 cm 거리로 표시된다. ^c3
- 이 노트는 코드 정적 확인이며 에디터 화면에서의 라벨 표시는 검증하지 않았다. ^c4

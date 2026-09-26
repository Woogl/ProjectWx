---
type: source
title: "결정 노트 - 2026-09-26-refresh-commit-trace"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "Wiki"
  - "최신화"
  - "피해"
summary: "직전 Wiki 최신화 39f3629a4 이후 bf6596012까지 19건 커밋을 정적 대조해 새로 반영할 것은 DefenseConstant 입력 제한 메타 제거 하나였음을 기록"
source_type: decision-note
source_id: src-a7dc3f3aa7dde3110b70
sha256: ceb5c8df0d967c1ed70e6e119136dca6b4b1aef519ae141641bab710464cf8e8
authority: primary
independence_key: ".wiki/raw/notes/2026-09-26-refresh-commit-trace.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-26-refresh-commit-trace.md"
raw_copy: ".raw/captured/ceb5c8df0d967c1ed70e6e119136dca6b4b1aef519ae141641bab710464cf8e8.md"
claim_ids:
  - clm-e898819eeb-c1
  - clm-e898819eeb-c2
  - clm-e898819eeb-c3
  - clm-e898819eeb-c4
key_claims:
  - "커밋 6adb657b9는 UWxCombatDeveloperSettings::DefenseConstant의 ClampMin·UIMin 메타를 제거해 설정 창이 입력값을 제한하지 않게 했다."
  - "CalculateDefenseMultiplier는 FMath::Max(DefenseConstant, UE_SMALL_NUMBER)로 보정하므로 DefenseConstant 메타 제거 후에도 0 이하 값이 방어 계산에 들어가지 않는다."
  - "39f3629a4 이후 bf6596012까지 19건 커밋 중 Wiki에 새로 반영할 지식 변경은 DefenseConstant 메타 제거 하나였다."
  - "2026-09-26 최신화 시점 Export-AbilitySystemLists.ps1 재실행 결과 어빌리티·이펙트·캐릭터 목록은 모두 변하지 않았다."
---

# 결정 노트 - 2026-09-26-refresh-commit-trace

- 원본: `.wiki/raw/notes/2026-09-26-refresh-commit-trace.md`
- 원자료 사본: `.raw/captured/ceb5c8df0d967c1ed70e6e119136dca6b4b1aef519ae141641bab710464cf8e8.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki 조사 노트 `2026-09-26-refresh-commit-trace.md`(제목 "Wiki 최신화: 39f3629a4 이후 커밋 추적", 출처 `MANUAL`, 수집일 2026-09-26).
- 사용자 요청으로 직전 최신화 커밋 `39f3629a4`(2026-09-25 20:36) 이후 HEAD `bf6596012`까지의 커밋 19건(병합 1건 포함)과 현재 작업 트리를 추적했다.
- 성격: 소스를 정적으로 대조한 조사(빌드·실행 검증 아님). 빌드·PIE·에셋 편집기 검증은 하지 않았다.

## 사람의 요청 원문

> 사용자 2026-09-26: "LLM 위키 최신화해주세요"

## 이미 반영된 변경

- `c9e2efec6` UI 표시 데이터 연결의 WxGame 리졸버 분리와 `IWxUIData` 제거: 같은 커밋에서 편찬됨.
- `64fcd9285`(캐릭터 재진입 시 어빌리티 소실·이벤트 중복 수정), `519f929fb`(새 게임 체크포인트 삭제 전 선택 Pawn 검증), `b6f1e9e8c`(어빌리티 방향 선택 공용화와 콤보 몽타주 배열): 모듈 리뷰 계약 노트(`2026-09-26-module-review-contracts`)가 `ad0db6de0` 작업 트리 기준으로 다룸.
- `ac6db7670` Exclusive 어빌리티 차단의 GAS 공통 태그 규칙 전환: 같은 커밋에서 편찬됨.
- `1689d999f`·`4c9f3934e` AnimNotify 역할별 색상 분류와 Editor 설정: AnimNotify 분류 노트로 편찬됨.
- `11047815a`·`33733d1bb`·`81c5e03dc` Workflow 개선·모듈 리뷰·작업 기록: `bf6596012`에서 편찬됨. 작업 트리의 구 워크플로우 잔재 제거와 문서 이미지 기능 제거도 [[결정 노트 - 2026-09-26-workflow-legacy-removal]]·[[결정 노트 - 2026-09-26-workflow-image-removal]]로 편찬돼 있음.

## 반영 대상이 아닌 변경

- `d644364be`: 작업 기록만 변경. `566fb1167`: WxGame UI 표시 자동화 테스트 소스(`WxUIPresentationTests.cpp`) 삭제. Wiki 기사는 이 테스트를 서술하지 않음.
- `7da389b85`(3층 리워크)·`6eb5200de`(적 추가): 레벨 배치 외부 액터·레벨 인스턴스만 변경. `ad0db6de0`은 병합. 새 캐릭터 에셋이 없어 캐릭터 목록도 불변.
- `264a71740`: AGENTS.md 코딩 규칙(엔진 순정 매크로 인라인 예외). 규칙 정본은 AGENTS.md.
- `184a76fec`: 2026-09-26 회의자료. 공유·예정 안건이며 구현 사실이 아님.
- 작업 트리의 `checkpoint-savegame` 기록 변경: 대시보드로 전달한 사람 테스트 결과로 작업 기록만 변경.

## 새로 반영한 변경(구현 관찰, 정적)

- `6adb657b9`: `UWxCombatDeveloperSettings::DefenseConstant`의 `ClampMin`·`UIMin` 메타를 뺐다. 설정 창은 입력값을 제한하지 않는다.
- 피해 계산 `CalculateDefenseMultiplier`(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`)가 `FMath::Max(DefenseConstant, UE_SMALL_NUMBER)`로 보정하므로 0 이하 값이 계산에 들어가지 않는 동작은 그대로다.

## 생성 목록

- `.agents/scripts/Export-AbilitySystemLists.ps1`을 다시 실행했고 어빌리티·이펙트·캐릭터 목록 세 개 모두 unchanged였다(어빌리티 40, 세트 9, 캐릭터 7, 몽타주 54, C++ 이펙트 26, GE_ 에셋 6, 피해 테이블 1).

## 관련 주제

- [[Wiki 운영]]
- [[피해 파이프라인]]

## 핵심 주장

- 커밋 6adb657b9는 UWxCombatDeveloperSettings::DefenseConstant의 ClampMin·UIMin 메타를 제거해 설정 창이 입력값을 제한하지 않게 했다. ^c1
- CalculateDefenseMultiplier는 FMath::Max(DefenseConstant, UE_SMALL_NUMBER)로 보정하므로 DefenseConstant 메타 제거 후에도 0 이하 값이 방어 계산에 들어가지 않는다. ^c2
- 39f3629a4 이후 bf6596012까지 19건 커밋 중 Wiki에 새로 반영할 지식 변경은 DefenseConstant 메타 제거 하나였다. ^c3
- 2026-09-26 최신화 시점 Export-AbilitySystemLists.ps1 재실행 결과 어빌리티·이펙트·캐릭터 목록은 모두 변하지 않았다. ^c4

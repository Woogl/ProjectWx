---
type: source
title: "작업 - animnotify-categories"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "작업-기록"
  - "AnimNotify"
  - "에디터"
  - "설정"
summary: "AnimNotify 17종을 6개 분류 색상으로 묶고 공용 색상 설정을 WxCore의 에디터 전용 설정으로 옮긴 작업 기록으로, 체크리스트 6/6 통과로 완료됐다."
source_type: task-record
source_id: src-a36446b84e1533ed7f08
sha256: c5f8b4308916075f5b4fb089c0cc59a0e23a571012915a6b238fb659a422fb74
authority: primary
independence_key: ".agents/workflow/tasks/animnotify-categories.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".agents/workflow/tasks/animnotify-categories.md"
raw_copy: ".raw/captured/c5f8b4308916075f5b4fb089c0cc59a0e23a571012915a6b238fb659a422fb74.md"
claim_ids:
  - clm-de569da4d0-c1
  - clm-de569da4d0-c2
  - clm-de569da4d0-c3
  - clm-de569da4d0-c4
key_claims:
  - "animnotify-categories 작업은 AnimNotify 17종을 Attack·AbilityFlow·Effect·Movement·Presentation·Misc 6개 분류 색상으로 나눴다."
  - "animnotify-categories 작업은 노티파이 색상 설정을 WxCore의 WxAnimNotifySettings로 두고 Config=Editor·DefaultConfig로 저장하게 했다."
  - "animnotify-categories 작업에서 공용 색상 필드는 WITH_EDITORONLY_DATA로, 17종 GetEditorColor는 WITH_EDITOR로 보호되며 패키지 빌드는 실행하지 않았다."
  - "animnotify-categories 작업의 사람 확인 2항목(코드 리뷰, 타임라인 표시)은 이우성이 2026-09-25에 통과로 기록했다."
---

# 작업 - animnotify-categories

- 원본: `.agents/workflow/tasks/animnotify-categories.md`
- 원자료 사본: `.raw/captured/c5f8b4308916075f5b4fb089c0cc59a0e23a571012915a6b238fb659a422fb74.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

몽타주 타임라인에서 AnimNotify·AnimNotifyState 17종을 성격별 6개 분류 색상으로 구분하고, 색상 설정을 도메인 모듈이 공유할 수 있게 `WxCore`로 옮긴 작업 기록이다. 상태는 완료(체크리스트 6/6 통과·AI 정리 완료)다.

## 사람의 판단 원문

> 사용자 2026-09-25: "네, 적용해주세요."

> 사용자 2026-09-25: "이 노티파이 색상 데이터는 #if 으로 에디터 전용으로 만들어버리는게 나을까요?"

> 사용자 2026-09-26: "네, 그렇게 해주세요. UWxCombatDeveloperSettings의 불필요 데이터는 제거해주세요."

> 사용자 2026-09-26: "제출해주세요."

> 사용자 2026-09-26(대화): "지금 완료로 바꿔주세요."

## 확정 결정

- 분류와 색상(sRGB 값을 선형 색상으로 변환해 쓴다):
  - Attack `#E86666`: WeaponAttack, AreaDamage, SpawnProjectile, FinisherDamage
  - AbilityFlow `#E8BE55`: ComboWindow, StartRecovery, FinisherVictim
  - Effect `#B58AE6`: ApplyGameplayEffect, UseItem
  - Movement `#62A9E8`: Rush, SnapToTarget
  - Presentation `#63C49A`: CameraMove, SlowTime, SkillCutscene
  - Misc `#929DAA`: SpawnMinion, DespawnMinion, ReportNoise
- 색상 설정 정의는 도메인 간 의존 없이 공유하도록 `WxCore`에 둔다. 실행 로직과 Details 속성 분류는 바꾸지 않는다.
- 엔진 `NotifyColor`가 `WITH_EDITORONLY_DATA`로 보호되므로 공용 색상 필드와 초기화도 같은 조건으로 보호하고, 17종 `GetEditorColor` 선언·정의와 설정 include는 `WITH_EDITOR`로 제한한다.
- 색상 설정은 `WxCore`에 유지하되 `Config=Editor`, `DefaultConfig`로 둔다. `UWxCombatDeveloperSettings`의 색상 데이터는 제거된 상태이고, 남은 `DefenseConstant`는 실제 피해 계산에서 쓰므로 유지한다.

## 구현 결과

- 공용 설정: `Plugins/WxCore/Source/WxCore/Public/WxAnimNotifySettings.h`, `Private/WxAnimNotifySettings.cpp`. 프로젝트 설정 Wx → Wx Anim Notify Settings에서 6색을 조정한다. 기존 Wx Combat Settings의 색상 3개는 제거했다.
- `WxCore`에는 엔진 `DeveloperSettings` 의존성만 더했다. `WxAI`·`WxInventory`가 `WxCombat`을 참조하지 않는다.
- 타임라인 라벨, 노티파이 실행 로직, 에셋은 바꾸지 않았다. 변경 범위 `git diff --check` 통과.

## 검증 범위

- AI 정적 검사(통과): 17종 매핑·6개 sRGB 값·AreaDamage·SnapToTarget 미리보기 색상 참조, 기존 색상 필드 참조 없음. 전처리 보호는 34개 파일의 선언·정의·include 검사로 확인했다. 패키지 빌드는 실행하지 않았다.
- AI 빌드(통과): WxEditor Win64 Development, `Saved/Logs/BuildDoctor/build_2026-09-26_000757_357_2060.log` Result Succeeded.
- AI 설정 확인(통과): `DefaultConfig` 유지, Config에 이전 색상 키 없음, `WxEffect_Damage.cpp`가 `DefenseConstant` 사용.
- 사람 확인(통과, 이우성 2026-09-25): 공용 색상 설정 이동과 17종 `GetEditorColor` 매핑 코드 리뷰, 에디터 재시작 후 6색·라벨 가독성과 설정의 `DefaultEditor.ini` 저장·재로드.
- AI 완료 정리의 전체 Wiki lint는 범위 밖 원자료(module-review-contracts)의 경고로 실패로 기록됐고, 완료 확정 시점에는 lint 0건이었다고 적혀 있다. 이는 문서 검사이며 게임 동작 검증이 아니다.

## 관련 주제

- [[에디터 도구]]
- [[모듈 구조와 코드 정리]]
- [[작업 - animnotify-labels]]

## 핵심 주장

- animnotify-categories 작업은 AnimNotify 17종을 Attack·AbilityFlow·Effect·Movement·Presentation·Misc 6개 분류 색상으로 나눴다. ^c1
- animnotify-categories 작업은 노티파이 색상 설정을 WxCore의 WxAnimNotifySettings로 두고 Config=Editor·DefaultConfig로 저장하게 했다. ^c2
- animnotify-categories 작업에서 공용 색상 필드는 WITH_EDITORONLY_DATA로, 17종 GetEditorColor는 WITH_EDITOR로 보호되며 패키지 빌드는 실행하지 않았다. ^c3
- animnotify-categories 작업의 사람 확인 2항목(코드 리뷰, 타임라인 표시)은 이우성이 2026-09-25에 통과로 기록했다. ^c4

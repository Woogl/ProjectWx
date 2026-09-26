---
type: source
title: "작업 - cooldown-unification"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "작업-기록"
  - "쿨다운"
  - "GAS"
summary: "쿨다운 그룹별 UWxEffect_Cooldown 파생 클래스를 공용 GE 하나로 통합하고 CooldownTags로 구분하게 바꾼 작업 기록으로, 사람 확인 4/4 통과로 완료됐다."
source_type: task-record
source_id: src-72e77d9f8acb89ba962a
sha256: 4a3c94e2677551a53fb9e381698c7cd8d429322687eaa89a73d819e144ded239
authority: primary
independence_key: ".agents/workflow/tasks/cooldown-unification.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".agents/workflow/tasks/cooldown-unification.md"
raw_copy: ".raw/captured/4a3c94e2677551a53fb9e381698c7cd8d429322687eaa89a73d819e144ded239.md"
claim_ids:
  - clm-2ca1fefddc-c1
  - clm-2ca1fefddc-c2
  - clm-2ca1fefddc-c3
  - clm-2ca1fefddc-c4
key_claims:
  - "cooldown-unification 작업은 쿨다운 그룹별 UWxEffect_Cooldown 파생 클래스를 지우고 공용 UWxEffect_Cooldown 하나를 UWxAbilityBase의 CooldownGameplayEffectClass 기본값으로 두었다."
  - "cooldown-unification 작업에서 쿨다운 구분은 어빌리티의 CooldownTags를 ApplyCooldown이 스펙 DynamicGrantedTags에 붙이는 방식으로 한다."
  - "cooldown-unification 작업은 회피 쿨다운 4초 버그의 원인인 UWxMMC_CooldownDuration을 지우고 ApplyCooldown이 SetByCaller.Duration으로 지속시간을 넘기게 했다."
  - "cooldown-unification 작업의 사람 확인 4항목(회피 쿨다운·UI, 소환물 쿨다운 무시, 네트워크 복제, 코드 리뷰)은 2026-09-25 모두 통과했다."
---

# 작업 - cooldown-unification

- 원본: `.agents/workflow/tasks/cooldown-unification.md`
- 원자료 사본: `.raw/captured/4a3c94e2677551a53fb9e381698c7cd8d429322687eaa89a73d819e144ded239.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

BP 커스텀·오버라이드 때문에 AI가 쿨다운 구조를 헷갈리지 않도록, 쿨다운 그룹별 GE 파생 클래스를 지우고 공용 `UWxEffect_Cooldown` 하나로 통합한 작업 기록이다. 상태는 완료(체크리스트 4/4 통과)다.

## 요청

- `Skill.1`·`Ultimate.1` 같은 번호 태그가 없어지기를 원했다(기록은 요청을 요약으로 적었다).

## 사람의 판단

- `Ability.Pattern.N`은 BT 재정비에 필요할 수 있어 유지하고, `Ability.Skill.N` 제거는 BT 재정비 때 한다. 스킬·쿨다운 파생 클래스는 최소화한다.
- 승인 원문(날짜 미기재):

> 사용자: "쿨다운 태그는 따로 두더라도, 쿨다운 클래스는 통합"

- 필드는 `FGameplayTagContainer CooldownTags`를 유지한다. 단일 `FGameplayTag CooldownGroup` 안을 검토한 뒤의 답(날짜 미기재):

> 사용자: "그대로 계속 진행"

- `SharesCooldownGroup`은 사용자 지적으로 추가하지 않았다.
- 수정 후 인게임 확인(회피 쿨다운·UI 진행률 보고에 대한 답):

> 사용자 2026-09-25: "잘 되네요"


## 구현 결과

- 공용 `UWxEffect_Cooldown`을 `UWxAbilityBase`의 `CooldownGameplayEffectClass` 기본값으로 둔다. `ApplyCooldown`이 어빌리티 `CooldownTags`를 스펙 `DynamicGrantedTags`에 붙인다.
- 충전 하나당 GE 하나(쌓지 않음)이고, 같은 태그의 최대 남은 시간 + `CooldownTime`으로 차례 회복한다. 슬롯 VM은 `DynamicGrantedTags`로 세고 주기는 `IWxUIData::GetCooldownTime()`이다. `Cooldown.Skill.4` 태그를 지웠다.
- 순정 규칙 누락 보정: 엔진 `AbilitySystem.WarnCooldownEffectWithoutTags` 규칙을 설계 때 놓쳐 첫 검증에서 GA_ 7개가 오류였다. CVar를 끄지 않고 공용 GE가 `Cooldown` 부모 태그를 부여하고, 빈 `CooldownTags`면 `CheckCooldown`이 먼저 통과시키도록 고쳤다.
- 에셋: 헤드리스 Python으로 GA_ 9개를 이관했다(`CooldownTags` 채움, 저장된 `CooldownGameplayEffectClass` 제거). 쿨다운 0인 HGTest Skill_2·Minion Skill_1은 태그가 없다.

## 버그 수정(사용자 보고, 2026-09-25)

- 증상: 회피 1회에 쿨다운 4초, UI 진행률 1 초과.
- 원인: 지속시간 MMC. 엔진이 GE를 활성 목록에 넣은 뒤 지속시간을 재계산해 MMC가 자기 자신(2초)을 대기열로 셌다.
- 수정: 대기열 계산을 적용 전 `ApplyCooldown`으로 옮겨 `SetByCaller.Duration`으로 넘기고 `UWxMMC_CooldownDuration`을 지웠다.

## 검증 범위

- AI 빌드: WxEditor Development 빌드 성공(최초·수정 후 모두).
- AI 데이터 검증: DataValidation 커맨드릿 GameplayAbilityBlueprint 40개·WxAbilitySet 9개 오류 0. 수정 후 GA_ 40개 오류 0(에디터 실행 중이라 8000 포트 오류만 있었다).
- AI 임시 자동화 테스트(삭제함): 1회 2.0초, 연속 2회 2.0/4.0초와 충전 판정 확인.
- 사람 확인(통과, 이우성 2026-09-25): 회피 쿨다운·UI 진행률, 소환물 쿨다운 무시, 리슨 서버·클라이언트 PIE 네트워크 복제, 코드 리뷰.
- 사용자 에디터의 DebugGame 바이너리는 다시 빌드해야 한다고 기록돼 있다.

## 관련 주제

- [[어빌리티와 GAS]]
- [[UI 표시 구조]]
- [[결정 노트 - 2026-09-25-cooldown-single-ge]]
- [[결정 노트 - 2026-09-26-cooldown-play-acceptance]]

## 핵심 주장

- cooldown-unification 작업은 쿨다운 그룹별 UWxEffect_Cooldown 파생 클래스를 지우고 공용 UWxEffect_Cooldown 하나를 UWxAbilityBase의 CooldownGameplayEffectClass 기본값으로 두었다. ^c1
- cooldown-unification 작업에서 쿨다운 구분은 어빌리티의 CooldownTags를 ApplyCooldown이 스펙 DynamicGrantedTags에 붙이는 방식으로 한다. ^c2
- cooldown-unification 작업은 회피 쿨다운 4초 버그의 원인인 UWxMMC_CooldownDuration을 지우고 ApplyCooldown이 SetByCaller.Duration으로 지속시간을 넘기게 했다. ^c3
- cooldown-unification 작업의 사람 확인 4항목(회피 쿨다운·UI, 소환물 쿨다운 무시, 네트워크 복제, 코드 리뷰)은 2026-09-25 모두 통과했다. ^c4

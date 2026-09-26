---
type: source
title: "결정 노트 - 2026-09-25-ability-data-on-ga"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "GAS"
  - "어빌리티"
  - "데이터"
summary: "어빌리티 테이블 구동을 시도했다가 AI 작업 편의를 기준으로 데이터 전용 GA_로 돌아가고 DT_Ability·DT_Effect를 지운 결정과 구현·검증 기록"
source_type: decision-note
source_id: src-9fbe4572f989de2390d1
sha256: ab29b513c45a013e1bb4d2371d110d4e86ade7dfd643e85572dfe66874edbb40
authority: primary
independence_key: ".wiki/raw/notes/2026-09-25-ability-data-on-ga.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-25-ability-data-on-ga.md"
raw_copy: ".raw/captured/ab29b513c45a013e1bb4d2371d110d4e86ade7dfd643e85572dfe66874edbb40.md"
claim_ids:
  - clm-eedb52027c-c1
  - clm-eedb52027c-c2
  - clm-eedb52027c-c3
  - clm-eedb52027c-c4
key_claims:
  - "2026-09-25 사용자는 프로젝트의 최우선 기준을 AI 입장의 작업 편의로 정했고 이를 근거로 어빌리티 테이블 구동 대신 데이터 전용 GA_로 돌아갔다."
  - "이 결정의 데이터 배치 규칙은 행이 에셋과 1:1이면 값을 그 에셋에 두고 테이블은 DT_Damage 같은 공유 정의와 레벨 곡선에만 쓰는 것이다."
  - "커밋 e305161ee·7c52ce0ce로 DT_Ability와 DT_Effect 및 그 행 구조체가 삭제됐다."
  - "GA_ 부여는 PIE 단독과 리슨 서버 PIE에서 확인됐지만 락온·가드 경감률·조작감은 미확인이다."
---

# 결정 노트 - 2026-09-25-ability-data-on-ga

- 원본: `.wiki/raw/notes/2026-09-25-ability-data-on-ga.md`
- 원자료 사본: `.raw/captured/ab29b513c45a013e1bb4d2371d110d4e86ade7dfd643e85572dfe66874edbb40.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 옛 LLM Wiki 결정 노트. frontmatter: 출처 `MANUAL`, 수집일 2026-09-25. 커밋 `5153da936`·`0473e201b`·`e305161ee`·`7c52ce0ce`·`d63ce0630`(2026-09-25).
- 결정·설계·검증 원문은 [[작업 - ability-table-driven]]의 "외부 사례 조사", "GA_ 복귀 설계·결과", "DT_Effect 제거", "락온 프리셋 복귀와 몽타주 검증 제거" 절이다. 테이블 구동 단계는 구현했지만 커밋하지 않았고, 커밋들은 HEAD `b93072ef8` 대비 최종 결과만 담는다.
- 노트 날짜 기준이라 현재 코드와 다를 수 있다.

## 사람의 판단 원문

> 사용자 2026-09-25: "우리 프로젝트는 AI 입장에서 작업 편의가 최우선입니다."

> 사용자: "나중에 필요하면 재검토" (타입별 몽타주 규칙·세트의 피격 반응 섹션 누락 검사를 지우라는 지시에서)

노트가 요약으로만 적은 사용자 결정: 엔진 칸을 `HideCategories`로 숨기지 않는다, 동적 태그(`DynamicSpecSourceTags`)는 쓰지 않는다, BP 그래프에 로직을 두지 않는다(관례), 규칙 칸을 GA_에서 바꾸지 않는다(관례).

## 판단 근거와 데이터 배치 규칙(확정)

- 순정 구조는 AI가 이미 아는 방식이라 함정 규칙이 적고, 값이 에셋 하나에 모이면 로그·디버거·git 이력·파일 목록에서 이름으로 보인다. 테이블 행은 바이너리 안에 있어 에디터 없이 보이지 않는다.
- 외부 사례 조사: 한 타입 클래스를 여러 스펙이 공유하고 테이블 행으로 구동하는 공개 선례를 찾지 못했다(Lyra·구 Action RPG·Slitterhead·Ninja Combat 등은 어빌리티 하나에 클래스·에셋 하나).
- 규칙: 행이 에셋과 1:1이면 값은 그 에셋(GA_·GE_)에 둔다. 테이블은 짝 에셋 없이 여러 곳이 골라 쓰는 정의(`DT_Damage`)나 레벨별 수치(`FScalableFloat`+CurveTable)에만 쓴다.

## 어빌리티 구현 관찰 (`e305161ee`)

- `UWxAbilityBase`는 `Blueprintable`, 어빌리티 하나가 데이터 전용 GA_ 하나다. C++ 파생 타입은 생성자에서 태그 관계·발동 그룹 같은 규칙 기본값을 정한다.
- Wx 프로퍼티: `ActivationInputAction`, `ActivationGroup`, `ActivationOwnedEffects`, `CostResource`·`CostAmount`, `AbilityMontage`·`CooldownTime`·`MaxRecharges`·`Title`·`Description`·`Icon`.
- GA_가 채우는 엔진 칸: `AbilityTags`(BT용 번호 태그 `Ability.Skill.N`·`Ability.Pattern.N`, `ActivationOwnedTags`에도 넣음), `ActivationRequiredTags`·`ActivationBlockedTags`(`Master.*`), `CooldownGameplayEffectClass`(`UWxEffect_Cooldown_*`), 패시브의 `AbilityTriggers`.
- 비용·쿨다운 MMC는 CDO 값을 읽는다(클래스당 스펙 하나). 쿨다운 시간이 0 이하면 쿨다운 GE가 없다.
- 공격 타입은 `UWxAbility_Attack_Light`·`_Heavy`·`_Air`·`_DodgeCounter` 넷이고 `UWxAbility_Attack`이 콤보 로직 추상 기반이다. 내용이 필요한 타입은 `Abstract`다.
- `UWxAbilitySet`은 `GrantedAbilities`를 `FGameplayAbilitySpec(클래스, 1)`로 부여하고 같은 클래스가 이미 있으면 경고 후 건너뛴다(이벤트 어빌리티 이중 반응 방지).
- `IsDataValid` 검증: GA_ 쿨다운 GE 타입(오류), 세트의 풀리지 않는 속성 행(오류), 중복·발동 조건 겹침·쿨다운 그룹 불일치(경고). 타입별 몽타주 규칙 검사는 사용자 지시로 지웠다.
- 에셋: GA_ 40개(액션 38, 패시브 2), `GA_Minion_Death` 삭제(사망은 `GA_Shared_Death` 하나). `DT_Ability`와 `FWxAbilityTableRow`를 지웠다. `GA_HGTest_Attack_Heavy_2` 쿨다운은 0(기존에 적용되지 않던 상태 그대로), `ABS_Soldier` 속성 행은 `TemplateEnemy`로 재연결.

## GE 구현 관찰 (`7c52ce0ce`)

- `UWxEffectComponent_Table` → `UWxEffectComponent_UIData`(`Title`·`Description`·`Icon`). 베이스가 `UGameplayEffectUIData`라 WxUI 조회 앵커는 그대로이고, 버프 목록은 아이콘을 채운 GE만 그린다.
- `FWxEffectTableRow`, `UWxMMC_EffectMagnitude`·`UWxMMC_EffectDuration`, `DT_Effect`를 지웠다. `GE_Shared_GuardReduction`은 경감 0.5, 아이콘 `T_UI_Shield`.

## 그 밖의 커밋

- `0473e201b`: `UWxViewModel_Ability`·`UWxInteractionScannerComponent`의 발동 판정을 스펙 기본 인스턴스로 한다.
- `5153da936`: 몽타주 도구에 `SnapNotifyStartsToSections`를 더하고 `AM_Shared_Dodge`의 구간 9개를 섹션 시작 + 0.0001초로 옮겨 앞 방향 회피 끝의 여분 무적·슬로모션을 없앴다.

## 검증 범위

- 확인: Development·DebugGame 빌드 경고 없음, GA_ 40개 이관 값 두 차례 대조 불일치 0, 데이터 검증 커맨드릿(GA_ 40·세트 9) 오류·경고 없음.
- PIE 단독(`LV_DevCombat`): 플레이어 19·솔저 8·템플릿 적 7·샌드백 5개가 GA_ 클래스로 부여, 솔저 BT가 패턴 1·2 발동. PIE 리슨 서버 + 클라이언트 1: 스펙 19개 복제.
- 미확인: HGTest·분신·도플갱어, 가드 경감률·아이콘, 템플릿 패시브 UP 지급, 락온, 조작감.

## 관련 주제

- [[어빌리티와 GAS]]
- [[UI 표시 구조]]
- [[에디터 도구]]

## 핵심 주장

- 2026-09-25 사용자는 프로젝트의 최우선 기준을 AI 입장의 작업 편의로 정했고 이를 근거로 어빌리티 테이블 구동 대신 데이터 전용 GA_로 돌아갔다. ^c1
- 이 결정의 데이터 배치 규칙은 행이 에셋과 1:1이면 값을 그 에셋에 두고 테이블은 DT_Damage 같은 공유 정의와 레벨 곡선에만 쓰는 것이다. ^c2
- 커밋 e305161ee·7c52ce0ce로 DT_Ability와 DT_Effect 및 그 행 구조체가 삭제됐다. ^c3
- GA_ 부여는 PIE 단독과 리슨 서버 PIE에서 확인됐지만 락온·가드 경감률·조작감은 미확인이다. ^c4

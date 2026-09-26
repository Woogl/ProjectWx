---
type: source
title: "작업 - ability-table-driven"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "작업-기록"
  - "GAS"
  - "어빌리티"
  - "DataTable"
summary: "어빌리티를 DataTable 행으로 구동하려던 전환 작업 기록으로, 여러 단계 구현 끝에 GA_ 에셋 방식으로 복귀해 테이블화 없이 체크리스트 7/7 통과로 마무리됐다."
source_type: task-record
source_id: src-725958e8aa2fa553c301
sha256: e08ab0df4442cca63ac5a88a17400d167b4417518b3b7e17d258986ab9cdf24e
authority: primary
independence_key: ".agents/workflow/tasks/ability-table-driven.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".agents/workflow/tasks/ability-table-driven.md"
raw_copy: ".raw/captured/e08ab0df4442cca63ac5a88a17400d167b4417518b3b7e17d258986ab9cdf24e.md"
claim_ids:
  - clm-ba97418dba-c1
  - clm-ba97418dba-c2
  - clm-ba97418dba-c3
  - clm-ba97418dba-c4
key_claims:
  - "ability-table-driven 작업은 어빌리티 테이블 구동 전환을 구현한 뒤 사용자 결정(2026-09-25)으로 어빌리티마다 데이터 전용 GA_ 에셋을 두는 방식으로 돌아가 테이블화 없이 마무리됐다."
  - "ability-table-driven 작업은 GA_ 복귀 뒤에도 공격 4분할 타입, 번호 섹션 몽타주 모델, 처형·컷신·아이템 노티파이를 유지했다."
  - "ability-table-driven 작업은 DT_Effect를 지우고 UWxEffectComponent_Table을 Title·Description·Icon 프로퍼티를 가진 UWxEffectComponent_UIData로 바꿨으며, DT_Damage는 유지했다."
  - "ability-table-driven 작업의 사람 인게임 확인 7항목(HGTest, 분신, 도플갱어, 조작감, 가드 경감·버프 아이콘, 템플릿 패시브 UP, 락온)은 이우성이 2026-09-25 통과로 기록했다."
---

# 작업 - ability-table-driven

- 원본: `.agents/workflow/tasks/ability-table-driven.md`
- 원자료 사본: `.raw/captured/e08ab0df4442cca63ac5a88a17400d167b4417518b3b7e17d258986ab9cdf24e.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

어빌리티를 GA_ 에셋 대신 DataTable 행으로 완전 구동하려던 작업 기록이다(2026-09-24 시작, 기준 HEAD `142fab5d6`). 여러 단계의 테이블 구동 구현과 행 찾기 재설계를 거친 뒤, 사용자가 GA_ 에셋 방식으로 돌아가기로 해 **테이블화 없이 마무리**됐다. 상태는 완료(체크리스트 7/7 통과)다. 기록의 테이블 관련 절(행 컬럼, 세트 = 테이블 목록 등)은 이력으로만 남는다.

## 요청

- 최초 요청(2026-09-24, 요약 기록): GA_를 하나하나 만드는 방식을 폐기하고 어빌리티를 테이블로 완전 구동한다. 사용자 초안 컬럼은 어빌리티 이름·타입·발동 타입·몽타주·키입력·코스트 종류/양·쿨타임·쿨타임 스택·아이콘·표기명·설명문이었다.
- 마무리 통보:

> 이우성 2026-09-25: "이 작업은 테이블화를 안하는 쪽으로 마무리되었습니다. 기능은 전부 정상 작동하고 있습니다."

## 단계별 경과

- **1-1단계(코드 규칙, 사용자 플레이 확인 2026-09-24 "잘 작동되네요")**: 콤보 창이 닫히면 1단부터(`OnComboWindowClosed`), 늦게 도착한 노티파이를 몽타주 인스턴스 ID로 거름, `UWxCharacterMovementComponent::JumpToLandingSection`이 재생 중 몽타주 전부에서 `Grounded`를 찾음, 그로기 중 넉 계열을 `HitReact.Normal`로 강등(`UWxEffectComponent_DamageReaction`), 시체 수명 `CorpseLifeSpan`을 `AWxCharacterBase`로 이동, 컷신 중 입력형 어빌리티 차단 GE(`UWxEffect_SkillCutscene`, `Effect.SkillCutscene`), 소비 아이템은 인벤토리가 선택(`CanUseConsumable`/`UseConsumable`).
- **1-2단계(몽타주 섹션 모델)**: 콤보·패턴은 번호 섹션 `1`, `2`… 몽타주 하나로 병합(9개), `AM_Shared_Dodge`에 `Backstep`·`Success<방향>` 추가, `AM_Shared_GuardReact`, `AM_Shared_HitReact_Knock`, 처형 노티파이 `UWxAnimNotify_FinisherVictim`·`UWxAnimNotify_FinisherDamage`, 궁극기 컷신 표식 `UWxAnimNotify_SkillCutscene`. 원래 몽타주 38개 삭제. 병합 중 평타 2타 버그(콤보 창 끝 6e-8초 오차)를 `WxAnimMontageToolset`의 `SnapNotifyEndsToSections`로 고쳤다(사용자 재확인 "이제 해결되었네요").
- **2단계(테이블 구동 전환, 사용자 확인 2026-09-24 "잘 되네요")**: 행 태그를 스펙 `DynamicSpecSourceTags`에 싣고, 공격을 `UWxAbility_Attack_Light`·`_Heavy`·`_Air`·`_DodgeCounter`로 분할, 테이블 6장 생성과 GA_ 41개·`DT_Ability` 삭제. 제출은 사용자 지시로 보류.
- **4단계(검증기)**: 행·세트 `IsDataValid`와 WxEditor `UWxAbilityMontageValidator`, 타입별 몽타주 규칙. `AM_Shared_Dodge` 구간 9개 시작을 섹션 시작 + 0.0001초로 옮김(Q-회피 A).
- **행 찾기 재설계(2026-09-25)**: 타입 칸 제거(태그로 타입 유추) → 안 D(행 항목 객체 `UWxAbilityEntry`) 구현 → BT를 태그 발동으로 되돌리며 행 `DynamicTags` 도입 → 세트 = 테이블 목록(`SourceObject` = 테이블). 각 단계 빌드와 네트워크 PIE를 거쳤다.
- **외부 사례 조사**: 같은 조합(타입 클래스 하나를 여러 스펙이 공유하고 테이블 행으로 구동)의 공개 사례는 찾지 못했다. Lyra·Action RPG·Slitterhead 등은 어빌리티 하나에 GA 하나다.

## 확정 결정(최종)

- **GA_ 복귀**:

> 사용자 2026-09-25: "AI 관점에서 구조 파악하고 작업하기 가장 편리한 방법으로 하고 싶습니다. 반드시 테이블화를 고집하려고 하는 것은 아닙니다."

> 사용자 2026-09-25: "네, 좋습니다. GA_ 에셋으로 돌아갑시다."

> 사용자 2026-09-25: "HideCategories로 숨기지는 마세요. 그리고 동적 태그는 테이블에서나 필요한 거였지, GA 에셋으로 하게 된다면 굳이 동적 태그 필요 없을거 같아요. BP 그래프에 로직을 두지는 않을 것입니다."

- 원칙: 어빌리티 하나에 데이터 전용 GA_ 하나, 부모는 구체 타입 C++. 규칙 칸 잠금과 그래프 로직 금지는 관례다.
- 테이블 전환에서 얻은 것은 유지: 공격 4분할과 타입 규칙, 몽타주 섹션 규칙, 노티파이(컷신·처형·아이템), 인스턴스로 판정하는 HUD.
- BT가 부르는 GA_는 엔진 `AssetTags`에 번호 태그(`Ability.Skill.1`~`3`, `Ability.Pattern.1`~`3`)를 더한다. 구분용 태그 8개는 지운다.
- **DT_Effect 제거**(사용자 "네, 제거합시다"): 행이 에셋과 1:1이면 값은 그 에셋에 둔다. `DT_Damage`는 여러 노티파이·투사체가 행을 골라 쓰므로 유지한다.
- **락온 프리셋 복귀**(사용자 "락온 어빌리티의 타게팅 프리셋도 프로젝트 세팅이 아니라 다시 어빌리티의 데이터로 복원시키는게 낫겠네요").
- **몽타주 검증 제거**(사용자 "ValidateMontage도 지금은 제거해주세요. 나중에 필요하면 재검토하겠습니다.").
- 솔저 속성 행: 사용자 결정 A, 지금 동작 유지(`ABS_Soldier` 속성 행을 `TemplateEnemy`로 재연결). 사용자 판단 "분신과 도플갱어가 함께 있을 수 없습니다", "DT_Dialogue는 지금은 무시하세요".

## 구현 결과(최종)

- `UWxAbilityBase`는 몽타주 칸 `AbilityMontage` 등 Wx 프로퍼티를 직접 읽고, 쿨다운 시간이 있으면 쿨다운 GE가 `UWxEffect_Cooldown` 파생인지 `IsDataValid`로 검사한다. `FWxAbilityTableRow`·`FWxPassiveTableRow`와 테이블 10장을 지웠다.
- 타입·패시브·WxAI BT 노드·`WxGameplayTags`·ASC 입력 라우팅·입력 버퍼·쿨다운 MMC는 HEAD 형태로 되돌렸다. 스펙에 `SourceObject`를 싣지 않는다.
- GA_ 40개(액션 38, 패시브 2)를 HEAD 경로·이름으로 헤드리스 Python(`GameplayAbilitiesBlueprintFactory`)으로 다시 만들었다. `GA_Minion_Death`는 `GA_Shared_Death`로 합친 상태를 유지한다.
- `UWxEffectComponent_Table` → `UWxEffectComponent_UIData`, `FWxEffectTableRow`와 MMC 둘 삭제, `GE_Shared_GuardReduction` 모디파이어 0.5, `GE_Template_Passive_AddUP` 5.
- 남은 검증: GA_ `IsDataValid`, 세트 `IsDataValid`(속성 행, 빈 칸, 중복, 같은 입력 조건 겹침, 같은 쿨다운 GE의 다른 값), 패시브 트리거 검사.
- 커밋: `5153da936`(회피 몽타주), `0473e201b`(슬롯 VM·스캐너), `e305161ee`(GA_ 데이터), `7c52ce0ce`(DT_Effect), Wiki `5c66bfaf9`.

## 검증 범위

- AI 빌드: 각 단계 Development·DebugGame 빌드 성공(GA_ 복귀 후 경고 없음).
- AI 데이터 검증: 커맨드릿으로 GA_ 40개·세트 9개 오류·경고 없음(`DT_Dialogue` 기존 오류 제외). GA_ 값을 스냅숏과 대조해 불일치 0.
- AI PIE(`LV_DevCombat`): 단독에서 플레이어 19, 솔저 8, 템플릿 적 7, 샌드백 5 스펙이 GA_ 클래스로 부여, 솔저 BT가 에셋 태그로 패턴 발동. 리슨 서버 + 클라이언트 1에서 클라이언트 스펙 19개 복제.
- 사람 인게임 확인(통과, 이우성 2026-09-25): HGTest 어빌리티, 분신, 도플갱어, 블렌드 인 0.05초 통일 뒤 조작감, 가드 경감·방패 버프 아이콘, 템플릿 패시브 UP 5, 락온 타게팅 프리셋.
- 1-1·2단계 사용자 확인은 항목별 범위를 따로 받지 않았다고 기록돼 있다.

## 미결정·충돌

- 테이블·클래스·몽타주 사이 암묵적 계약을 드러내는 작업(A 선언과 편집 화면 표시, B 역할 표식, C 상황별 몽타주)은 미결로 남았다.
- 기획 브리핑(`Docs/Meeting/어빌리티 규칙 브리핑.md`)의 GA_·ABS_ 설명 수정 제안이 남은 일로 적혀 있다(직접 수정 금지).
- 기존 데이터 결함: `DT_Dialogue`가 없는 `AM_Death`를 소프트 참조(무시 지시), `GA_Shared_Sprint` 진입 SP 비용 0, HGTest_Attack_Heavy_2 5초 쿨다운 미적용(0초로 이관).
- 재도입 참고: 몽타주 검증 규칙과 반례 시험 결과는 기록의 4단계 절에 남아 있다.

## 관련 주제

- [[어빌리티와 GAS]]
- [[에디터 도구]]
- [[적 AI와 몬스터]]
- [[결정 노트 - 2026-09-25-ability-data-on-ga]]
- <!--wl-->결정 노트 - 2026-09-26-ability-ga-play-acceptance

## 핵심 주장

- ability-table-driven 작업은 어빌리티 테이블 구동 전환을 구현한 뒤 사용자 결정(2026-09-25)으로 어빌리티마다 데이터 전용 GA_ 에셋을 두는 방식으로 돌아가 테이블화 없이 마무리됐다. ^c1
- ability-table-driven 작업은 GA_ 복귀 뒤에도 공격 4분할 타입, 번호 섹션 몽타주 모델, 처형·컷신·아이템 노티파이를 유지했다. ^c2
- ability-table-driven 작업은 DT_Effect를 지우고 UWxEffectComponent_Table을 Title·Description·Icon 프로퍼티를 가진 UWxEffectComponent_UIData로 바꿨으며, DT_Damage는 유지했다. ^c3
- ability-table-driven 작업의 사람 인게임 확인 7항목(HGTest, 분신, 도플갱어, 조작감, 가드 경감·버프 아이콘, 템플릿 패시브 UP, 락온)은 이우성이 2026-09-25 통과로 기록했다. ^c4

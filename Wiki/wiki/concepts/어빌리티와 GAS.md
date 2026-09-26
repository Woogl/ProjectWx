---
type: concept
title: "어빌리티와 GAS"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - concept
summary: "GA·GE·쿨다운·차단 태그 등 GAS 어빌리티 구조"
sources:
  - "[[기획서 - PC규격서]]"
  - "[[기획서 - WA_PC_규격서]]"
  - "[[기획서 - WA_주인공_캐릭터]]"
  - "[[기획서 - WX_AM_기능정리]]"
  - "[[기획서 - 경직 수정본]]"
  - "[[기획서 - 그로기_피니시_시스템_기획서]]"
  - "[[작업 - ability-table-driven]]"
  - "[[작업 - cooldown-unification]]"
  - "[[작업 - ui-data-interface-removal]]"
---

# 어빌리티와 GAS

GA·GE·쿨다운·차단 태그 등 GAS 어빌리티 구조에 관한 원자료 요약을 모은 주제 페이지입니다. 문장마다 끝의 링크가 출처이며, 절 이름으로 기획 요구사항·사람의 확정 결정·코드 구현 관찰·검증 범위·미결정을 구분합니다. 구현 관찰은 원자료 작성 시점의 코드 기준이고, 문서 갱신이나 Wiki lint 통과는 게임 동작 검증이 아닙니다.

## 요구사항

- PC규격서는 7종 GA(약공격·강공격·스킬·궁극기·가드·점프·포션)가 AM의 AN_StartRecovery 이후 프레임에서 실행 중 GA를 캔슬하는 후딜 캔슬을 요구한다. ([[기획서 - PC규격서]])
- PC규격서는 AM 전 구간에서 가능한 전체 캔슬을 캔슬로 발동할 GA의 태그로 관리하고 특별한 경우에만 쓰도록 한다. ([[기획서 - PC규격서]])
- PC규격서는 캔슬 발동 GA가 활성화 차단 태그만 무시하고 쿨타임·코스트·스택은 지키며, 사망·그로기·히트 리액션 GA는 캔슬 규칙에서 제외하도록 요구한다. ([[기획서 - PC규격서]])
- WA_PC_규격서는 개별 어빌리티를 캐릭터 폴더로 관리하고, 공용 어빌리티와 입력이 같아도 조건·판정이 다르면 캐릭터 ABS에서 공용 어빌리티를 빼도록 요구한다. ([[기획서 - WA_PC_규격서]])
- WA_주인공_캐릭터 기획서는 강화 버프 동안 일반 공격·강공격·E 스킬을 다른 애니메이션의 강화 버전으로 바꾸고, 버프 종료 시 진행 중 애니메이션은 끝내되 다음 강화 콤보는 막도록 요구한다. ([[기획서 - WA_주인공_캐릭터]])
- WA_주인공_캐릭터 기획서는 강화 E 스킬 발동 시 강화 버프 지속시간을 2초 늘리도록 요구한다. ([[기획서 - WA_주인공_캐릭터]])
- WX_AM_기능정리 문서는 WX_Combo_Window가 GA에 지정된 다음 콤보로 넘어갈 유효 프레임 구간을 정하고, 그 구간에 같은 키를 누르면 다음 콤보 AM을 실행한다고 적는다. ([[기획서 - WX_AM_기능정리]])
- WX_AM_기능정리 문서는 WX_Invincible이 몽타주 구간 동안 캐릭터를 무적으로 만들고, WX_Perfect_Guard가 가드 어빌리티 중 패링 가능 구간을 지정한다고 적는다. ([[기획서 - WX_AM_기능정리]])
- WX_AM_기능정리 문서는 WX_Snap_To_Target이 구간 동안 타겟 프리셋 규칙으로 고른 대상을 바라보게 해 공격 방향을 보정한다고 적는다. ([[기획서 - WX_AM_기능정리]])
- 경직 수정본 기획서는 GA 종류를 일반 공격(Ability.Attack), 패턴(Ability.Pattern), 스킬(Ability.Skill), 가드, 회피, 궁극기로 나누고 히트 리액션 판정 기준으로 쓴다. ([[기획서 - 경직 수정본]])
- 그로기 피니시 기획서는 그로기 피니시 GA의 에셋 태그를 Ability.GroggyFinish로, 차단 태그를 Ability로, 활성화 차단 태그를 State.Dead로 지정한다. ([[기획서 - 그로기_피니시_시스템_기획서]])

## 확정 결정

- 어빌리티는 테이블 구동 대신 어빌리티마다 구체 타입 C++을 부모로 하는 데이터 전용 GA_ 에셋 하나를 두기로 사용자가 2026-09-25 확정했으며, 규칙 칸 잠금과 BP 그래프 로직 금지는 관례로 둔다. ([[작업 - ability-table-driven]])
- 공격 어빌리티는 UWxAbility_Attack_Light·Heavy·Air·DodgeCounter 네 타입으로 나누고 캐릭터 공통 발동 조건은 타입 규칙에 둔다. ([[작업 - ability-table-driven]])
- 행이 에셋과 1:1인 DT_Effect는 제거하고 값은 GE 에셋에 두며, 여러 노티파이·투사체가 행을 골라 쓰는 DT_Damage는 유지한다. ([[작업 - ability-table-driven]])
- 쿨다운은 그룹별 GE 파생 클래스 대신 공용 UWxEffect_Cooldown 하나와 어빌리티의 FGameplayTagContainer CooldownTags로 구분하기로 사용자가 승인했다. ([[작업 - cooldown-unification]])
- Ability.Pattern.N 태그는 BT 재정비에 필요할 수 있어 유지하고 Ability.Skill.N 제거는 BT 재정비 때 하기로 사용자가 정했다. ([[작업 - cooldown-unification]])

## 구현 관찰

- GA_ 복귀 후 BT가 부르는 GA_는 엔진 AssetTags에 Ability.Skill.N·Ability.Pattern.N 번호 태그를 더하고, 스펙에 SourceObject나 동적 태그를 싣지 않는다. ([[작업 - ability-table-driven]])
- 콤보·패턴 몽타주는 번호 섹션 1, 2…를 가진 몽타주 하나로 병합되었고 단계 전환은 같은 몽타주를 다음 섹션부터 새로 재생한다. ([[작업 - ability-table-driven]])
- ApplyCooldown은 CooldownTags를 스펙 DynamicGrantedTags에 붙이고, 같은 태그의 최대 남은 시간에 CooldownTime을 더한 값을 SetByCaller.Duration으로 넘겨 충전을 차례로 회복시킨다. ([[작업 - cooldown-unification]])
- 공용 쿨다운 GE는 엔진 WarnCooldownEffectWithoutTags 규칙을 만족하도록 Cooldown 부모 태그를 부여하고, 빈 CooldownTags면 CheckCooldown이 먼저 통과시킨다. ([[작업 - cooldown-unification]])
- 어빌리티 슬롯 VM은 어빌리티 제거로 인스턴스가 Garbage가 된 경우를 IsExplicitlyNull()로 처음부터 빈 슬롯과 구분해 제목·충전을 초기화한다. ([[작업 - ui-data-interface-removal]])
- 공유 AbilitySystem VM의 효과 목록 연결은 한 번만 설정되며 연결 전 조회된 빈 목록은 현재 활성 GE로 보충된다. ([[작업 - ui-data-interface-removal]])

## 검증 범위

- GA_ 복귀는 Development·DebugGame 빌드, GA_ 40개·세트 9개 데이터 검증 커맨드릿, 단독·리슨 서버 PIE로 AI가 확인했고, 사람이 HGTest·분신·도플갱어·조작감·가드 경감·패시브 UP·락온을 인게임으로 통과 확인했다. ([[작업 - ability-table-driven]])
- 쿨다운 통합은 WxEditor Development 빌드와 GA_ 40개 데이터 검증을 통과했고, 사람이 회피 쿨다운·UI, 소환물 쿨다운 무시, 리슨 서버 네트워크 복제를 인게임으로 확인했다. ([[작업 - cooldown-unification]])

## 미결정·충돌

- 어빌리티 클래스·데이터·몽타주 사이 암묵적 계약을 드러내는 방법(선언과 편집 화면 표시 등)은 미결로 남았고, 타입별 몽타주 검증기는 제거된 채 필요 시 재검토하기로 했다. ([[작업 - ability-table-driven]])

## 원자료

- [[기획서 - PC규격서]] — 모든 PC가 공유하는 HP·MP·UP·SP 자원, 공통 어빌리티 분류, 후딜·전체 GA 캔슬 규칙, 회피 스택·극한회피, 가드·패링 규격을 정의한 문서
- [[기획서 - WA_PC_규격서]] — 명조 방향 PC 구조를 속성 6종, 캐릭터 스탯, HP·궁극기 게이지·스태미나·고유 자원, 공용·개별 어빌리티 분류로 정의한 Project WX PC 규격서
- [[기획서 - WA_주인공_캐릭터]] — 주인공 캐릭터의 개별 어빌리티(5단 일반 공격·강공격 연계·회피 반격·E 스킬·고유 자원 스킬·10초 강화 버프·궁극기) 규격서
- [[기획서 - WX_AM_기능정리]] — WX 전용 애님 노티파이 2종(투사체 생성·범위 공격)과 노티파이 시퀀스 5종(콤보 창·무적·퍼펙트 가드·타겟 스냅·무기 공격)의 기능을 정리한 문서
- [[기획서 - 경직 수정본]] — GA 종류와 경직 원인·결과 판정으로 PC와 적의 히트 리액션 발생 여부를 하나의 흐름으로 통합하고 패링 경직을 정의한 기획서
- [[기획서 - 그로기_피니시_시스템_기획서]] — 그로기 상태 적에게 반경 3m 안에서 상호작용 키로 발동하는 강공격 그로기 피니시의 발동 조건·진행 규칙·예외를 정의한 기획서
- [[작업 - ability-table-driven]] — 어빌리티를 DataTable 행으로 구동하려던 전환 작업 기록으로, 여러 단계 구현 끝에 GA_ 에셋 방식으로 복귀해 테이블화 없이 체크리스트 7/7 통과로 마무리됐다.
- [[작업 - cooldown-unification]] — 쿨다운 그룹별 UWxEffect_Cooldown 파생 클래스를 공용 GE 하나로 통합하고 CooldownTags로 구분하게 바꾼 작업 기록으로, 사람 확인 4/4 통과로 완료됐다.
- [[작업 - ui-data-interface-removal]] — 공용 IWxUIData 인터페이스를 없애고 WxCombat 데이터·규칙, WxUI VM, WxGame 리졸버 연결로 역할을 나눈 완료 작업 기록

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
  - "[[결정 노트 - 2026-09-23-damage-forward-flow]]"
  - "[[결정 노트 - 2026-09-23-zero-damage-hitstop]]"
  - "[[결정 노트 - 2026-09-24-wxcombat-cleanup]]"
  - "[[결정 노트 - 2026-09-24-wxcombat-machinery-cleanup]]"
  - "[[결정 노트 - 2026-09-25-ability-block-policy-centralization]]"
  - "[[결정 노트 - 2026-09-25-ability-data-on-ga]]"
  - "[[결정 노트 - 2026-09-25-cooldown-single-ge]]"
  - "[[결정 노트 - 2026-09-25-exclusive-submission-cleanup]]"
  - "[[결정 노트 - 2026-09-25-exclusive-tag-blocking]]"
  - "[[결정 노트 - 2026-09-26-ability-ga-play-acceptance]]"
  - "[[결정 노트 - 2026-09-26-cooldown-play-acceptance]]"
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
- 사용자는 2026-09-23 회피를 ASC의 OnImmunityBlockGameplayEffectDelegate를 쓰는 방식으로 재구현하도록 지시했고, Dodge 어빌리티가 활성 동안 막힌 UWxEffect_Damage를 감지해 극한 회피로 전환한다. ([[결정 노트 - 2026-09-23-damage-forward-flow]])
- 사용자는 2026-09-24 스택형이 아닌 GE도 쓸 수 있도록 구간 GE 노티파이가 자기가 건 핸들만 제거하는 B안을 골랐다. ([[결정 노트 - 2026-09-24-wxcombat-cleanup]])
- 사용자는 2026-09-24 소환물 AbilitySet의 NoCooldown을 IgnoreCooldowns로 바꾸고 IgnoreCooldowns는 엔진 기본 GE 컴포넌트 방식으로 동작해야 한다고 결정했다. ([[결정 노트 - 2026-09-24-wxcombat-machinery-cleanup]])
- 2026-09-25 사용자는 어빌리티 공통 차단 규칙을 ASC ApplyAbilityBlockAndCancelTags 확장에서 계산해 Super에 넘기는 방식을 승인하고 이번 범위를 자식의 차단 코드 제거로 한정했다. ([[결정 노트 - 2026-09-25-ability-block-policy-centralization]])
- 2026-09-25 사용자는 AI 입장의 작업 편의를 최우선 기준으로 삼았고, 어빌리티 데이터는 어빌리티별 데이터 전용 GA_ 에셋에 둔다. ([[결정 노트 - 2026-09-25-ability-data-on-ga]])
- 행이 에셋과 1:1인 데이터는 그 GA_·GE_ 에셋에 두고, DataTable은 DT_Damage 같은 여러 곳이 골라 쓰는 정의와 레벨 곡선에만 쓴다. ([[결정 노트 - 2026-09-25-ability-data-on-ga]])
- 사용자는 엔진 칸을 HideCategories로 숨기지 않고 스펙 DynamicSpecSourceTags를 쓰지 않도록 결정했다. ([[결정 노트 - 2026-09-25-ability-data-on-ga]])
- 사용자 결정으로 쿨다운 GE는 공용 UWxEffect_Cooldown 하나로 합치고 쿨다운 식별 태그는 어빌리티의 CooldownTags 컨테이너에 둔다. ([[결정 노트 - 2026-09-25-cooldown-single-ge]])
- 사용자는 Ability.Pattern.N 태그를 BT 재정비용으로 유지하고 Ability.Skill.N 제거는 BT 재정비 때 하기로 했다. ([[결정 노트 - 2026-09-25-cooldown-single-ge]])
- 사용자는 2026-09-25에 Exclusive 어빌리티 발동 차단을 엔진 순정 태그 차단(BlockAbilitiesWithTag)으로 바꾸고 콤보 구간의 자기 재발동을 허용하기로 승인했다. ([[결정 노트 - 2026-09-25-exclusive-tag-blocking]])
- 어빌리티 테이블 전환은 철회되었고 데이터 전용 GA_를 유지하며, 행 컬럼·세트의 테이블 목록·동적 행 태그 설계는 과거 이력으로만 남는다. ([[결정 노트 - 2026-09-26-ability-ga-play-acceptance]])
- 쿨다운은 공용 GE·동적 쿨다운 태그·충전당 GE 하나·적용 전 SetByCaller 대기열 계산 구조를 유지한다. ([[결정 노트 - 2026-09-26-cooldown-play-acceptance]])

## 구현 관찰

- GA_ 복귀 후 BT가 부르는 GA_는 엔진 AssetTags에 Ability.Skill.N·Ability.Pattern.N 번호 태그를 더하고, 스펙에 SourceObject나 동적 태그를 싣지 않는다. ([[작업 - ability-table-driven]])
- 콤보·패턴 몽타주는 번호 섹션 1, 2…를 가진 몽타주 하나로 병합되었고 단계 전환은 같은 몽타주를 다음 섹션부터 새로 재생한다. ([[작업 - ability-table-driven]])
- ApplyCooldown은 CooldownTags를 스펙 DynamicGrantedTags에 붙이고, 같은 태그의 최대 남은 시간에 CooldownTime을 더한 값을 SetByCaller.Duration으로 넘겨 충전을 차례로 회복시킨다. ([[작업 - cooldown-unification]])
- 공용 쿨다운 GE는 엔진 WarnCooldownEffectWithoutTags 규칙을 만족하도록 Cooldown 부모 태그를 부여하고, 빈 CooldownTags면 CheckCooldown이 먼저 통과시킨다. ([[작업 - cooldown-unification]])
- 어빌리티 슬롯 VM은 어빌리티 제거로 인스턴스가 Garbage가 된 경우를 IsExplicitlyNull()로 처음부터 빈 슬롯과 구분해 제목·충전을 초기화한다. ([[작업 - ui-data-interface-removal]])
- 공유 AbilitySystem VM의 효과 목록 연결은 한 번만 설정되며 연결 전 조회된 빈 목록은 현재 활성 GE로 보충된다. ([[작업 - ui-data-interface-removal]])
- 2026-09-23 기준 엔진 Immunity 통지는 ApplyGameplayEffectSpecToSelf에서 CanApply보다 먼저 돌기 때문에 적대 판정을 GE CanApply로 옮기면 아군 공격에도 회피 통지가 나가 되돌렸다. ([[결정 노트 - 2026-09-23-damage-forward-flow]])
- UWxEffect_Invincible의 Immunity는 UWxEffect_Damage 클래스만 막고 그 차단 통지를 Dodge가 극한 회피로 받으며, 이미 걸린 지속 피해 GE와 치트 GE는 무적 중에도 들어간다. ([[결정 노트 - 2026-09-23-zero-damage-hitstop]])
- 커밋 ce6295184 이후 UWxAnimNotifyState_ApplyGameplayEffect는 몽타주 인스턴스 ID를 키로 AppliedEffects 맵에 핸들을 두고 끝에서 그 핸들의 스택 하나만 빼며, 몽타주가 아닌 재생에서는 적용하지 않는다. ([[결정 노트 - 2026-09-24-wxcombat-cleanup]])
- UWxAbilityBase::EndAbility의 ActivationOwnedEffects 제거와 UWxSkillCutsceneComponent::Finish의 컷신 무적 제거도 핸들의 스택 하나만 빼도록 바뀌었다. ([[결정 노트 - 2026-09-24-wxcombat-cleanup]])
- 2026-09-24 작업 트리의 UWxEffect_IgnoreCooldowns는 Infinite GE로 UImmunityGameplayEffectComponent와 URemoveOtherGameplayEffectComponent가 Cooldown 부모 태그를 부여하는 GE를 막고 걷는다. ([[결정 노트 - 2026-09-24-wxcombat-machinery-cleanup]])
- 코스트 무시 UWxEffect_IgnoreCosts는 순정 CheckCost가 면역이 아니라 어트리뷰트를 보기 때문에 태그와 AbilityBase 판정을 유지한다. ([[결정 노트 - 2026-09-24-wxcombat-machinery-cleanup]])
- 발동 그룹 점유·가드 입력·질주 SP·컷신 사용 중 같은 Wx 고유 발동 실패는 로그를 남기지 않는다. ([[결정 노트 - 2026-09-24-wxcombat-machinery-cleanup]])
- 2026-09-25 작업 트리의 UWxAbilityBase::GetAbilityBlockTags는 명시 차단 목록과 ActivationGroup 공통 규칙을 합치며, Exclusive·Override는 Attack·Skill·Pattern·Ultimate·Dodge·Guard·UseItem·Interact·Jump를 막는다. ([[결정 노트 - 2026-09-25-ability-block-policy-centralization]])
- Exclusive 약공격(Ability.Attack.Light)은 공통 Attack 부모 대신 Light·Air·DodgeCounter만 막아 강공격이 순정 CancelAbilitiesWithTag로 약공격을 끊을 수 있다. ([[결정 노트 - 2026-09-25-ability-block-policy-centralization]])
- 2026-09-25 커밋 이후 비용·쿨다운 시간·표시·몽타주는 UWxAbilityBase의 GA_ 프로퍼티이고 발동 조건·쿨다운 그룹 GE·BT 번호 태그·패시브 트리거는 엔진 칸에 둔다. ([[결정 노트 - 2026-09-25-ability-data-on-ga]])
- UWxAbilitySet은 캐릭터 ASC에 같은 어빌리티 클래스가 이미 있으면 경고하고 부여를 건너뛴다. ([[결정 노트 - 2026-09-25-ability-data-on-ga]])
- UWxAbilityBase::ApplyCooldown은 충전 하나당 공용 쿨다운 GE 하나를 걸고 같은 태그의 최대 남은 시간 + CooldownTime을 SetByCaller.Duration으로 넘겨 충전을 차례로 회복시킨다. ([[결정 노트 - 2026-09-25-cooldown-single-ge]])
- 공용 쿨다운 GE는 순정 IsDataValid 규칙을 지키려고 Cooldown 부모 태그를 부여하며, CooldownTags가 빈 어빌리티는 CheckCooldown을 바로 통과한다. ([[결정 노트 - 2026-09-25-cooldown-single-ge]])
- Exclusive 차단 정리 후 주석 기준으로 Override는 취소 면역일 뿐 발동 차단은 실제 에셋 태그와 차단 목록이 결정하며, 처형·가드 반응은 공통 차단 목록에 식별 태그가 없어 진입한다. ([[결정 노트 - 2026-09-25-exclusive-submission-cleanup]])
- Exclusive 차단 전환 후 일반 액션·반응 타입 생성자가 Attack·Skill·Pattern·Ultimate·Dodge·Guard·UseItem·Interact·Jump를 BlockAbilitiesWithTag에 더하고, 반응·처형 태그는 목록에 없어 진입할 수 있다. ([[결정 노트 - 2026-09-25-exclusive-tag-blocking]])
- ActivationGroup은 입력 버퍼 분류·단계 전이·Override 취소 면역에만 남으므로 새 액션 분류를 도입하면 공통 차단 목록과 Light 개별 목록을 함께 검토해야 한다. ([[결정 노트 - 2026-09-25-exclusive-tag-blocking]])

## 검증 범위

- GA_ 복귀는 Development·DebugGame 빌드, GA_ 40개·세트 9개 데이터 검증 커맨드릿, 단독·리슨 서버 PIE로 AI가 확인했고, 사람이 HGTest·분신·도플갱어·조작감·가드 경감·패시브 UP·락온을 인게임으로 통과 확인했다. ([[작업 - ability-table-driven]])
- 쿨다운 통합은 WxEditor Development 빌드와 GA_ 40개 데이터 검증을 통과했고, 사람이 회피 쿨다운·UI, 소환물 쿨다운 무시, 리슨 서버 네트워크 복제를 인게임으로 확인했다. ([[작업 - cooldown-unification]])
- 구간 GE 노티파이 변경은 임시 자동화 테스트 Wx.Combat.ApplyEffectNotify.OwnHandleOnly로 확인한 뒤 테스트 파일을 지웠고 플레이는 미검증이다. ([[결정 노트 - 2026-09-24-wxcombat-cleanup]])
- WxCombat 장치 정리는 빌드와 ABS_Minion·ABS_Doppelganger 재로드까지 확인됐고 인게임은 미검증이다. ([[결정 노트 - 2026-09-24-wxcombat-machinery-cleanup]])
- 공통 차단 통합은 빌드, ServerOnly 자동화 테스트 3개, GA 40개 CDO 차단 관계 1,600건 대조로 확인됐고 실제 입력·몽타주 타이밍·UI·네트워크 예측/복제는 미확인이다. ([[결정 노트 - 2026-09-25-ability-block-policy-centralization]])
- GA_ 복귀는 빌드, GA_ 40개 값 대조, 데이터 검증 커맨드릿, PIE 단독·리슨 서버 부여 확인까지 했고 락온·가드 경감률·조작감은 미확인이다. ([[결정 노트 - 2026-09-25-ability-data-on-ga]])
- 쿨다운 GE 통합은 에디터 빌드·데이터 검증 커맨드릿(GA 40개·WxAbilitySet 9개 오류 0)·임시 자동화 테스트와 사용자 인게임 회피 확인을 거쳤고, 소환물 쿨다운 무시와 네트워크 복제는 확인되지 않았다. ([[결정 노트 - 2026-09-25-cooldown-single-ge]])
- Exclusive 차단 임시 테스트와 검증 스크립트는 사용자 요청으로 제거되었고, 그 회귀 결과는 삭제 전 근거로만 남으며 사람 코드 리뷰·플레이·예측/복제는 미확인이다. ([[결정 노트 - 2026-09-25-exclusive-submission-cleanup]])
- Exclusive 태그 차단은 Editor 빌드, GA 40개 차단 관계 1,600건 대조, AbilityBlocking 자동화 테스트 2건 성공으로 확인되었고 몽타주·선입력 감각·UI·예측/복제·플레이는 미검증이다. ([[결정 노트 - 2026-09-25-exclusive-tag-blocking]])
- GA_ 유지 이후 HGTest·분신·도플갱어 어빌리티 발동과 연출, 블렌드 인 0.05초 통일 뒤 조작감, 가드 중 피해 절반 감소와 방패 버프 아이콘, 패시브 UP 5 지급, 타게팅 프리셋 락온을 사람이 2026-09-25 통과시켰다. ([[결정 노트 - 2026-09-26-ability-ga-play-acceptance]])
- GA_ 유지의 사람 테스트는 네트워크 구성이 명시되지 않아 예측·복제·BT 타이밍 검증으로 확대하지 않는다. ([[결정 노트 - 2026-09-26-ability-ga-play-acceptance]])
- 사람은 2026-09-25 공용 쿨다운 GE에 대해 회피 쿨다운·UI 진행률, 소환물 쿨다운 무시, 리슨 서버·클라이언트 PIE 복제, `UWxEffect_Cooldown`·`ApplyCooldown`·`CheckCooldown` 코드 리뷰를 통과시켰다. ([[결정 노트 - 2026-09-26-cooldown-play-acceptance]])
- 공용 쿨다운 GE 확인은 전용 서버·지연·패킷 손실 조건과 모든 어빌리티 조합으로 확대하지 않는다. ([[결정 노트 - 2026-09-26-cooldown-play-acceptance]])

## 미결정·충돌

- 어빌리티 클래스·데이터·몽타주 사이 암묵적 계약을 드러내는 방법(선언과 편집 화면 표시 등)은 미결로 남았고, 타입별 몽타주 검증기는 제거된 채 필요 시 재검토하기로 했다. ([[작업 - ability-table-driven]])
- 사용자는 어빌리티 자식 클래스도 나중에 없앨 수 있으면 없애고 싶다고 했지만 클래스 통합 여부와 시점은 정하지 않았다. ([[결정 노트 - 2026-09-25-ability-block-policy-centralization]])

## 원자료

- [[결정 노트 - 2026-09-23-damage-forward-flow]] — Hit Wrapper GE와 전용 EffectContext를 없애고 ApplyDamage 판정에서 Damage GE 컴포넌트 반응으로 결과가 앞으로만 흐르게 한 2026-09-23 결정들
- [[결정 노트 - 2026-09-23-zero-damage-hitstop]] — 히트스톱을 Hit Cue와 같은 조건(피해 0 초과 또는 퍼펙트 가드)으로 맞추고 Hit Cue 예측 발행 등 낡은 주석을 정정한 기록. 빌드 통과, 플레이 미검증.
- [[결정 노트 - 2026-09-24-wxcombat-cleanup]] — 구간 GE 노티파이가 자기 핸들만 걷게 하고 처형 피해를 처형 어빌리티가 직접 적용하며 퍼펙트 가드 Cue를 Hit Cue로 통합한 WxCombat 정리 네 건 기록
- [[결정 노트 - 2026-09-24-wxcombat-machinery-cleanup]] — 소환물 쿨다운 무시를 순정 GE 컴포넌트로 바꾸고 사망 BT 정지를 AI 컨트롤러로 단일화하며 락온 대상 사본을 없앤 WxCombat 장치 정리 기록
- [[결정 노트 - 2026-09-25-ability-block-policy-centralization]] — 어빌리티 자식 생성자의 공통 차단 코드를 지우고 ActivationGroup·태그 기반 계산을 ASC ApplyAbilityBlockAndCancelTags 확장 지점으로 통합한 결정과 검증 기록
- [[결정 노트 - 2026-09-25-ability-data-on-ga]] — 어빌리티 테이블 구동을 시도했다가 AI 작업 편의를 기준으로 데이터 전용 GA_로 돌아가고 DT_Ability·DT_Effect를 지운 결정과 구현·검증 기록
- [[결정 노트 - 2026-09-25-cooldown-single-ge]] — 쿨다운 그룹별 GE 파생 클래스를 공용 UWxEffect_Cooldown 하나로 합치고 어빌리티 CooldownTags와 SetByCaller 대기열로 충전을 차례 회복시킨 결정
- [[결정 노트 - 2026-09-25-exclusive-submission-cleanup]] — 사용자 요청으로 Exclusive 차단의 임시 C++ 테스트·검증 스크립트를 제거하고 차단 관련 주석을 태그 차단·취소 면역 기준으로 정정한 기록
- [[결정 노트 - 2026-09-25-exclusive-tag-blocking]] — Exclusive 발동 차단을 순정 BlockAbilitiesWithTag로 옮기고 콤보 자기 재발동과 도플갱어 소유자 발동 조건 면제 범위를 정한 결정과 검증
- [[결정 노트 - 2026-09-26-ability-ga-play-acceptance]] — 어빌리티 테이블 전환을 철회하고 데이터 전용 GA_를 유지한 최종 결정과 HGTest·소환물·조작감·GE·락온 사람 테스트 7개 통과 범위
- [[결정 노트 - 2026-09-26-cooldown-play-acceptance]] — 공용 쿨다운 GE 통합 후 회피 UI 진행률·소환물 쿨다운 무시·리슨 서버와 클라이언트 복제·코드 리뷰를 사람이 통과시킨 확인 범위
- [[기획서 - PC규격서]] — 모든 PC가 공유하는 HP·MP·UP·SP 자원, 공통 어빌리티 분류, 후딜·전체 GA 캔슬 규칙, 회피 스택·극한회피, 가드·패링 규격을 정의한 문서
- [[기획서 - WA_PC_규격서]] — 명조 방향 PC 구조를 속성 6종, 캐릭터 스탯, HP·궁극기 게이지·스태미나·고유 자원, 공용·개별 어빌리티 분류로 정의한 Project WX PC 규격서
- [[기획서 - WA_주인공_캐릭터]] — 주인공 캐릭터의 개별 어빌리티(5단 일반 공격·강공격 연계·회피 반격·E 스킬·고유 자원 스킬·10초 강화 버프·궁극기) 규격서
- [[기획서 - WX_AM_기능정리]] — WX 전용 애님 노티파이 2종(투사체 생성·범위 공격)과 노티파이 시퀀스 5종(콤보 창·무적·퍼펙트 가드·타겟 스냅·무기 공격)의 기능을 정리한 문서
- [[기획서 - 경직 수정본]] — GA 종류와 경직 원인·결과 판정으로 PC와 적의 히트 리액션 발생 여부를 하나의 흐름으로 통합하고 패링 경직을 정의한 기획서
- [[기획서 - 그로기_피니시_시스템_기획서]] — 그로기 상태 적에게 반경 3m 안에서 상호작용 키로 발동하는 강공격 그로기 피니시의 발동 조건·진행 규칙·예외를 정의한 기획서
- [[작업 - ability-table-driven]] — 어빌리티를 DataTable 행으로 구동하려던 전환 작업 기록으로, 여러 단계 구현 끝에 GA_ 에셋 방식으로 복귀해 테이블화 없이 체크리스트 7/7 통과로 마무리됐다.
- [[작업 - cooldown-unification]] — 쿨다운 그룹별 UWxEffect_Cooldown 파생 클래스를 공용 GE 하나로 통합하고 CooldownTags로 구분하게 바꾼 작업 기록으로, 사람 확인 4/4 통과로 완료됐다.
- [[작업 - ui-data-interface-removal]] — 공용 IWxUIData 인터페이스를 없애고 WxCombat 데이터·규칙, WxUI VM, WxGame 리졸버 연결로 역할을 나눈 완료 작업 기록

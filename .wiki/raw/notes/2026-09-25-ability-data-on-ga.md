---
title: "어빌리티·GE 데이터를 에셋 한 곳으로: 테이블 구동 시도 후 GA_ 복귀, DT_Ability·DT_Effect 제거"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, combat, ui, architecture, decision]
summary: "어빌리티 테이블 구동을 구현했다가 AI 작업 편의를 최우선 기준으로 삼아 어빌리티별 데이터 전용 GA_로 돌아갔다. 행이 에셋과 1:1인 데이터는 그 에셋에 두고, 테이블은 여러 곳이 골라 쓰는 정의(DT_Damage)와 레벨 곡선에만 쓴다. DT_Ability·DT_Effect를 지우고, 비용·쿨다운·표시·몽타주는 GA_ 프로퍼티, 발동 조건·쿨다운 그룹·BT 번호 태그·패시브 트리거는 엔진 칸, GE 표시는 UWxEffectComponent_UIData에 둔다."
---

# 어빌리티·GE 데이터를 에셋 한 곳으로

2026-09-25 커밋 `5153da936`·`0473e201b`·`e305161ee`·`7c52ce0ce`·`d63ce0630`. 결정·설계·검증 원문은 [어빌리티 테이블 구동 전환](../../../.agents/workflow/tasks/ability-table-driven.md)의 "외부 사례 조사", "GA_ 복귀 설계·결과", "DT_Effect 제거", "락온 프리셋 복귀와 몽타주 검증 제거" 절이다. 테이블 구동 단계(2·4단계, 행 찾기 변천)는 구현했지만 커밋하지 않았고, 이 커밋들은 HEAD `b93072ef8` 대비 최종 결과만 담는다.

## 결정

- 기준(사용자, 2026-09-25): "우리 프로젝트는 AI 입장에서 작업 편의가 최우선입니다." 순정 구조는 AI가 이미 아는 방식이라 함정 규칙이 적고, 값이 에셋 하나에 모이면 엔진 로그·디버거·git 이력·파일 목록에서 이름으로 보인다. 테이블 행은 바이너리 안에 있어 에디터 없이는 목록도 보이지 않는다.
- 외부 사례 조사: 한 타입 클래스를 여러 스펙이 공유하고 테이블 행으로 구동하는 공개 선례는 찾지 못했다. Lyra·구 Action RPG·Slitterhead(GA 380개)·Ninja Combat 등은 어빌리티 하나에 클래스·에셋 하나다.
- 데이터 배치 규칙: 행이 에셋과 1:1이면 값은 그 에셋(GA_·GE_)에 둔다. 테이블은 짝 에셋이 없고 여러 곳이 골라 쓰는 정의(`DT_Damage`)나 레벨별 수치(`FScalableFloat`+CurveTable, 엔진 순정)에만 쓴다.
- 사용자 결정: 엔진 칸을 `HideCategories`로 숨기지 않는다. 동적 태그(스펙 `DynamicSpecSourceTags`)는 쓰지 않는다. BP 그래프에 로직을 두지 않는다(관례). 규칙 칸을 GA_에서 바꾸지 않는 것도 관례다.

## 어빌리티 (`e305161ee`)

- `UWxAbilityBase`는 `Blueprintable`이다. 어빌리티 하나가 데이터 전용 GA_ 하나이고, C++ 파생 클래스(타입)는 생성자에서 태그 관계·발동 그룹 같은 규칙 기본값을 정한다.
- Wx 프로퍼티: `ActivationInputAction`, `ActivationGroup`(EditDefaultsOnly), `ActivationOwnedEffects`(타입 기본값에 GA_가 더함), `CostResource`·`CostAmount`(공개), `AbilityMontage`·`CooldownTime`·`MaxRecharges`·`Title`·`Description`·`Icon`(보호). 몽타주는 `GetMontage()`로 읽는다. `EWxAbilityCostResource`는 `WxAbilityBase.h`로 옮겼다. 멤버 이름을 `Montage`로 두면 몽타주 인자를 받는 함수의 매개변수가 가려져 `AbilityMontage`로 했다.
- GA_가 채우는 엔진 칸: `AbilityTags`(BT가 부르는 번호 태그 `Ability.Skill.N`·`Ability.Pattern.N`을 더하고 `ActivationOwnedTags`에도 같이 넣음), `ActivationRequiredTags`·`ActivationBlockedTags`(캐릭터 상태 `Master.*`), `CooldownGameplayEffectClass`(쿨다운 그룹 GE `UWxEffect_Cooldown_*`), 패시브의 `AbilityTriggers`.
- `GetCooldownGameplayEffect`는 쿨다운 GE가 `UWxEffect_Cooldown` 파생이고 `CooldownTime`이 0 이하면 nullptr을 돌려준다. 비용 MMC(`UWxMMC_Cost`)와 쿨다운 MMC(`UWxMMC_CooldownDuration`)는 컨텍스트의 CDO(`GetAbility()`)에서 값을 읽는다. 클래스당 스펙이 하나라 CDO 값이 곧 그 어빌리티 값이다.
- 타입:
  - 공격은 넷이다. `UWxAbility_Attack_Light`(공중·회피 중 금지), `_Heavy`(Light 취소, 공중·회피 중 금지), `_Air`(`Movement.InAir` 필요), `_DodgeCounter`(`Ability.Dodge` 필요, 공중 금지). `UWxAbility_Attack`은 콤보 로직을 가진 추상 기반이다.
  - `UWxAbility_Skill`의 에셋·소유 태그는 `Ability.Skill`이다. 스킬 번호는 GA_가 더한다.
  - 내용이 필요한 타입은 `Abstract`다(공격 넷, Dodge, Finisher, Guard, GuardReact, HitReact, Pattern, Skill, Passive, WxGame의 Interact·UseItem). Death·Groggy·LockOn·Sprint·Ultimate·PlayMontageOnce는 구체 클래스다.
  - 타입 튜닝 값(락온 7개, 질주 2개, 넉업 속도, 상호작용 사거리, 처형 문구)은 HEAD처럼 `EditDefaultsOnly`다. 락온 타게팅 프리셋도 HEAD처럼 `UWxAbility_LockOn::TargetingPreset`이고 `GA_Shared_LockOn`이 `TP_LockOn`을 가리킨다.
  - `UWxAbilityBase::LandingSectionName`(`Grounded`)을 이동 컴포넌트가 쓴다. 회피 `Backstep`·`Success` 접두사, 가드 반응 섹션 넷은 각 타입의 정적 상수다.
- 패시브: `UWxAbility_Passive`(부모 `UWxAbilityBase`, 효과 `TriggeredEffects`). `IsDataValid`는 트리거가 GameplayEvent인지, 트리거끼리 조상 관계가 아닌지 본다(조상과 자식을 함께 걸면 한 이벤트에 두 번 발동한다).
- `UWxAbilitySet`: `GrantedAbilities`(`TSubclassOf<UWxAbilityBase>` 목록)를 `FGameplayAbilitySpec(클래스, 1)`로 부여한다. `SourceObject`는 싣지 않는다. 캐릭터 ASC에 같은 클래스가 이미 있으면 경고하고 건너뛴다(`FindAbilitySpecFromClass`). 세트 사이에 같은 어빌리티가 겹치면 이벤트로 도는 어빌리티가 한 번에 두 번 반응하기 때문이다.
- 검증(`IsDataValid`, 에디터 검증기 없음):
  - GA_: 쿨다운 시간이 있으면 쿨다운 GE가 `UWxEffect_Cooldown` 파생이어야 한다(오류). 블루프린트 컴파일도 CDO의 `IsDataValid`를 부른다.
  - 세트: 풀리지 않는 속성 행(오류), 빈 칸·같은 어빌리티 중복·같은 입력의 발동 조건 겹침·같은 쿨다운 GE의 다른 시간이나 충전 수(경고). 엔진이 태그 조건과 쿨다운 GE를 protected로 두어 판정 도우미 `IsActivationExclusive`·`SharesCooldownGroup`을 `UWxAbilityBase`에 둔다.
  - 타입별 몽타주 규칙(`ValidateMontage` 계열)과 세트의 피격 반응 섹션 누락 검사는 사용자 지시로 지웠다("나중에 필요하면 재검토").
- 에셋:
  - GA_ 40개(액션 38, 패시브 2)는 HEAD 경로·이름의 순정 `GameplayAbilityBlueprint`다. `GA_Minion_Death`는 지웠다. 사망은 `GA_Shared_Death` 하나이고 `ABS_Minion`은 더 이상 사망을 주지 않는다(`ABS_Shared_Enemy`가 준다).
  - 세트 부여 수: `ABS_Shared_Player` 12(피격 3·사망 + 가드·회피·가드 반응·처형·락온·질주·상호작용·아이템), `ABS_Shared_Enemy`·`ABS_Sandbag` 5(피격 3·사망·그로기), `ABS_HGTest` 12(패시브 포함), `ABS_Minion` 3, 템플릿 플레이어 7(패시브 포함), 템플릿 적 2, `ABS_Soldier` 3, `ABS_Doppelganger` 0.
  - `DT_Ability`(HEAD의 `AbilityDataRow`가 가리키던 비용·쿨다운·표시 테이블)와 행 구조체 `FWxAbilityTableRow`를 지웠다.
  - 데이터 수정: `GA_HGTest_Skill_3`이 `Master.Minion`에 막힌다. `GA_HGTest_Attack_Heavy_2`의 쿨다운은 0이다(행에 5초가 있었지만 GE가 없어 적용되지 않던 상태를 그대로 옮김). `ABS_Soldier`의 속성 행은 `TemplateEnemy`다(없는 `Enemy` 행을 가리키던 것을 다시 연결, 수치 불변).
  - 설명 문구의 현지화 키는 새로 잡혔다. 현지화 데이터가 없어 영향이 없다.

## GE (`7c52ce0ce`)

- `UWxEffectComponent_Table` → `UWxEffectComponent_UIData`. 행 핸들 대신 `Title`·`Description`·`Icon` 프로퍼티를 가진다. 베이스가 엔진 `UGameplayEffectUIData`라 WxUI의 조회 앵커(`FindComponent<UGameplayEffectUIData>` 뒤 `IWxUIData` 캐스트)는 그대로다. 버프 목록은 아이콘을 채운 GE만 그린다.
- `FWxEffectTableRow`, `UWxMMC_EffectMagnitude`·`UWxMMC_EffectDuration`, `DT_Effect`를 지웠다.
- `UWxEffect_GuardReduction`의 경감 모디파이어는 `FScalableFloat`이고 값은 GE_ 에셋이 채운다. `GE_Shared_GuardReduction`은 0.5, 아이콘 `T_UI_Shield`다. `GE_Template_Passive_AddUP`은 UP 5를 직접 쓰고 UI 컴포넌트가 없다(`GE_HGTest_AddUP`과 같은 형태).
- 클래스 이름 변경은 임시 `ClassRedirects`로 옮기고 재저장한 뒤 리디렉트를 뺐다. `GE_Shared_GuardReduction` 파일에는 컴포넌트 인스턴스 이름 `WxEffectComponent_Table_0`이 남는다.

## 그 밖의 커밋

- `0473e201b`: `UWxViewModel_Ability`와 `UWxInteractionScannerComponent`의 발동 판정(조건·비용·발동 가능)을 스펙의 기본 인스턴스로 한다. 엔진 발동 경로(`InternalTryActivateAbility`)도 인스턴스가 있으면 인스턴스로 판정한다.
- `5153da936`: 몽타주 도구에 `SnapNotifyStartsToSections`(섹션 시작 직전에서 트리거되어 그 섹션으로 넘어가는 구간의 시작을 섹션 시작 + Offset으로 옮김)를 더하고, `AM_Shared_Dodge`의 구간 9개(방향 섹션 무적 7개, 극한 회피 슬로모션 2개)를 자기 섹션 시작 + 0.0001초로 옮겼다. 앞 방향 회피 끝에 붙던 여분 무적·슬로모션이 없어졌다.

## 검증

- Development·DebugGame 빌드 경고 없음.
- 이관 값 대조: GA_ 40개를 이관 직후와 순정 형식 재생성 뒤 새 프로세스에서 두 번 대조했다(불일치 0). 두 GE도 리디렉트 없이 새 프로세스로 로드해 확인했다.
- 데이터 검증 커맨드릿: GA_ 40개, 세트 9개 오류·경고 없음.
- PIE 단독(`LV_DevCombat`): 플레이어 19, 솔저 8, 템플릿 적 7, 샌드백 5개가 GA_ 클래스로 부여됐다. 솔저 BT가 에셋 태그로 패턴 1·2를 발동했고 소유 태그에 `Ability.Pattern.N`이 실렸다(플레이어 HP 100→0).
- PIE 네트워크(리슨 서버 + 클라이언트 1): 클라이언트 자기 캐릭터에 스펙 19개가 GA_ 클래스로 복제됐다.
- 미확인: HGTest·분신·도플갱어(맵에 없음), 가드 경감률·아이콘, 템플릿 패시브 UP 지급, 락온, 조작감.

## 도구에서 확인한 사실

- MCP `BlueprintTools.create`의 `asset_type`은 부모 클래스이고 결과는 일반 `Blueprint`다. 순정 `GameplayAbilityBlueprint`는 헤드리스 Python의 `unreal.GameplayAbilitiesBlueprintFactory`(`parent_class` 지정)로 만든다.
- git에 삭제가 스테이징된 경로에 에셋을 만들면 에디터 git 연동이 모달을 띄워 MCP 호출이 멈춘다. 그 세션만 `-SCCProvider=None`으로 띄워 피했다.

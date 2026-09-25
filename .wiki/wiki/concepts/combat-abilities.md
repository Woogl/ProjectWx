---
title: "전투 어빌리티와 이펙트"
category: concept
sources:
  - "raw/notes/2026-09-22-current-combat.md"
  - "raw/notes/2026-09-22-current-ability-cost-cooldown.md"
  - "raw/notes/2026-09-24-wxcombat-cleanup.md"
  - "raw/notes/2026-09-24-wxcombat-machinery-cleanup.md"
  - "raw/notes/2026-09-25-ability-montage-section-model.md"
  - "raw/notes/2026-09-25-ability-data-on-ga.md"
  - "raw/notes/2026-09-25-cooldown-single-ge.md"
created: 2026-09-22
updated: 2026-09-25
tags: [wx, combat]
aliases: ["GAS"]
confidence: medium
volatility: warm
verified: 2026-09-25
summary: "어빌리티 하나는 데이터 전용 GA_ 하나다. C++ 타입이 규칙을, GA_가 몽타주·입력·수치·표시를 가진다. 콤보·패턴·반응은 한 몽타주의 섹션으로 고른다."
---

# 전투 어빌리티와 이펙트

어빌리티 하나는 데이터 전용 GA_ 하나다. C++ 타입이 규칙을, GA_가 몽타주·입력·수치·표시를 가진다. 콤보·패턴·반응은 한 몽타주의 섹션으로 고른다.

## 타입과 GA_

- C++ 파생 클래스가 타입이다. 생성자에서 에셋·소유 태그, 취소·차단 관계, 발동 그룹, 홀드·토글 같은 규칙 기본값을 정한다. GA_는 부모 타입을 고르고 콘텐츠를 채운다. GA_에서 규칙 칸을 바꾸지 않고 BP 그래프에 로직을 두지 않는 것은 관례다(엔진 칸을 숨기지 않기로 한 사용자 결정, 2026-09-25).
- GA_가 채우는 Wx 프로퍼티: `ActivationInputAction`, `AbilityMontage`(`GetMontage()`), `ActivationOwnedEffects`(타입 기본값 뒤에 더함), `CooldownTime`·`CooldownTags`·`MaxRecharges`, `CostResource`(Custom/SP/MP/UP)·`CostAmount`, `Title`·`Description`·`Icon`. `ActivationGroup`과 타입 튜닝 값(락온·질주·넉업 속도·상호작용 사거리·처형 문구)도 `EditDefaultsOnly`다.
- GA_가 채우는 엔진 칸: 캐릭터 상태 조건(`Master.*`)은 `ActivationRequiredTags`·`ActivationBlockedTags`, 패시브 트리거는 `AbilityTriggers`다. BT가 부르는 번호 태그(`Ability.Skill.N`, `Ability.Pattern.N`)는 `AbilityTags`와 `ActivationOwnedTags`에 함께 더한다.
- 공격 타입은 넷이다. `UWxAbility_Attack_Light`(공중·회피 중 금지), `_Heavy`(Light 취소, 공중·회피 중 금지), `_Air`(`Movement.InAir` 필요), `_DodgeCounter`(`Ability.Dodge` 필요, 공중 금지). `UWxAbility_Attack`은 콤보 로직을 가진 추상 기반이다. 스킬 타입의 식별 태그는 `Ability.Skill`이다.
- 콘텐츠가 있어야 하는 타입은 `Abstract`라 GA_로만 부여된다(공격 넷, Dodge, Finisher, Guard, GuardReact, HitReact, Pattern, Skill, Passive, WxGame의 Interact·UseItem). Death·Groggy·LockOn·Sprint·Ultimate·PlayMontageOnce는 구체 클래스다.
- 데이터 배치 규칙(2026-09-25 사용자 확정): 행이 에셋과 1:1이면 값은 그 에셋에 둔다. 그래서 어빌리티 표(`DT_Ability`)와 효과 표(`DT_Effect`)를 없앴다. 테이블은 짝 에셋이 없고 여러 곳이 골라 쓰는 정의(`DT_Damage`)나 레벨별 수치(`FScalableFloat`+CurveTable)에만 쓴다. 어빌리티를 테이블 행으로 구동하는 구조를 구현했다가 되돌렸고, 판단 근거는 AI 작업 편의(순정 구조, 값이 한 곳, 로그·git에서 이름으로 보임)다.
- `UWxAbilitySet`은 최대값→현재값 속성 초기화, `GrantedEffects` 적용, `GrantedAbilities` 부여 순서로 동작한다. 부여는 `FGameplayAbilitySpec(클래스, 1)`이고 `SourceObject`를 싣지 않는다. 캐릭터 ASC에 같은 클래스가 이미 있으면 경고하고 건너뛴다. 세트 사이에 같은 어빌리티가 겹치면 이벤트로 도는 어빌리티가 한 번에 두 번 반응하기 때문이다. 같은 입력의 어빌리티가 여럿이면 세트 순서대로 시도해 처음 성공한 것을 쓴다.

## 발동과 취소

`UWxAbilityBase`는 InstancedPerActor와 LocalPredicted를 기본값으로 둔다. `Independent`는 배타 점유에 참여하지 않고, `Exclusive`는 다른 배타 액션을 막으며, `Override`는 반응·사망 등 강제 행동에 사용한다. `CancelAbilitiesWithTag`로 지목한 점유자에 대한 우선 규칙도 있으므로 그룹 이름만으로 발동 결과를 결정하면 안 된다.

발동 실패 사유는 엔진이 자기 판정(태그·쿨다운·코스트 등)만 `LogAbilitySystem` Verbose로 남긴다. Wx 고유 판정(발동 그룹 점유·가드 입력·질주 SP·컷신 사용 중)의 실패는 로그가 없다. 엔진의 로그 게이트 `FScopedCanActivateAbilityLogEnabler`는 GAS 모듈 밖에서 링크되지 않는다. 게이트 없는 로그는 UI·상호작용이 `CanActivateAbility`를 조회할 때마다 찍히므로 쓰지 않는다.

`Exclusive`의 실행 단계는 `Blocking`, `ComboWindow`, `Recovery`다. 콤보 창은 자기 재발동을 허용하고, Recovery는 다른 배타 액션이 끊고 들어올 수 있게 한다. `OpenComboWindow`와 `StartRecovery`는 입력 버퍼를 다시 처리한다. `CloseComboWindow`는 이미 Recovery로 넘어간 동작을 Blocking으로 되돌리지 않는다.

- 콤보 창이 닫히면 `OnComboWindowClosed`가 공격·스킬의 단계를 되돌린다. 창이 닫힌 뒤 누르면 후딜 중이어도 1단부터다.
- 콤보 창·후딜 노티파이는 몽타주 인스턴스 ID를 넘기고, 어빌리티는 지금 재생 중인 인스턴스가 아니면 무시한다(`IsPlayingMontageInstance`). 끊긴 앞 단 몽타주도 블렌드아웃 동안 노티파이를 보내기 때문이다. 브랜칭 포인트 경로도 오버라이드한다. 엔진 기본 구현이 빈 이벤트 참조를 넘겨 인스턴스를 잃는다.
- 스킬 컷신 동안 `UWxSkillCutsceneComponent`가 모든 플레이어 ASC에 `UWxEffect_SkillCutscene`을 걸어 입력형 어빌리티(Attack·Skill·Ultimate·Dodge·Guard·UseItem·Sprint·LockOn·Interact)를 막는다. 피격·사망 같은 반응 어빌리티는 막지 않는다.

## 몽타주 섹션

어빌리티당 몽타주는 하나이고 변형은 섹션으로 고른다. `UWxAbilityBase::PlayMontage`는 없는 시작 섹션이면 경고하고 실패한다. 엔진은 그런 섹션을 무시하고 처음부터 재생한다.

- 콤보·패턴: 단계 섹션 `1`, `2`, …를 두고 섹션 사이 링크는 끊는다. 단계 수와 섹션은 `GetComboStageCount`·`GetComboStageSection`이 정하고, 번호 섹션이 없으면 처음부터 한 단계다. 다음 단은 같은 몽타주를 그 섹션부터 새로 재생해 교차 블렌드를 유지한다. 콤보는 재발동으로, 패턴은 블렌드아웃 때 넘어간다. 재생 중인 인스턴스 안에서 `JumpToSection`으로 넘기지 않는다.
- 회피: 8방향 섹션, `Backstep`, 극한 회피 `Success<방향>`. 이동 입력이 없으면 `Backstep`(없으면 `Back`), 구성되지 않은 방향은 `Forward`다.
- 가드 반응: `GuardHit`, `GuardKnockback`, `GuardBreak`, `PerfectGuard`. 피격 반응은 반응 태그 끝 이름과 같은 섹션이 자기 몽타주에 있는 어빌리티만 반응한다([피해 처리와 전투 연출](combat-damage.md)).
- 착지: 공중에서 도는 몽타주(공중 공격·넉업)는 `Grounded` 섹션을 둔다. `UWxCharacterMovementComponent::JumpToLandingSection`이 재생 중인 모든 몽타주 인스턴스에서 `UWxAbilityBase::LandingSectionName`을 찾아 옮긴다.
- 처형의 짝 몽타주·피해 행과 궁극기 컷신은 몽타주 노티파이가 담는다([처형](combat-finisher.md)). 궁극기는 재생 전에 `UWxAnimNotify_SkillCutscene` 표식을 읽어 컷신 → 몽타주 순서로 재생한다.
- 섹션 편집 주의: 0초가 아닌 섹션 시작에 놓인 노티파이는 앞 섹션 끝에서 불린다(`AnimMontage.cpp:887`). 섹션 경계를 부동소수 오차만큼 넘은 구간은 다음 섹션을 트는 새 인스턴스가 이어받는다(`AnimInstance.cpp:1890`). 몽타주 도구의 `SnapNotifyEndsToSections`·`SnapNotifyStartsToSections`로 경계를 맞춘다([편집기 도구](../references/editor-tools.md)).

## 비용과 쿨다운

- 비용 GE `UWxEffect_Cost`는 모든 어빌리티가 함께 쓰는 Instant 효과다(`CostGameplayEffectClass` 기본값). MP·UP·SP에 Additive 모디파이어를 하나씩 두고, 각 MMC가 소스 어빌리티 CDO(`GetAbility()`)의 `CostResource`가 자기 자원일 때만 `-CostAmount`를 낸다. 엔진 순정 CheckCost·ApplyCost를 그대로 쓰며, 순정 자원 부족 판정이 Additive 모디파이어만 보기 때문에 Additive를 쓴다.
- 쿨다운 GE `UWxEffect_Cooldown`도 모든 어빌리티가 함께 쓴다(`CooldownGameplayEffectClass` 기본값). 쿨다운의 식별자는 GA_의 `CooldownTags`(`Cooldown.*`)다. `ApplyCooldown`이 공용 GE 스펙의 `DynamicGrantedTags`에 붙이고 `GetCooldownTags()`가 돌려주므로 순정 쿨다운 판정·쿼리가 그대로 본다. 같은 태그를 고른 어빌리티끼리 쿨다운을 나눠 쓴다. 엔진은 스택을 GE 클래스로만 합쳐 태그를 가리지 않으므로 공용 GE는 쌓지 않는다(2026-09-25 그룹별 파생 클래스를 합침).
- 소모한 충전 하나가 쿨다운 GE 하나다. `UWxAbilityBase::CheckCooldown`은 `CooldownTags`로 찾은 활성 GE가 `MaxRecharges`보다 적으면 통과시킨다. 지속시간 MMC는 소스 어빌리티 CDO의 `CooldownTime`에 같은 태그 쿨다운의 최대 남은 시간을 더해, 충전이 차례로 돌아온다(회피 2초·충전 2를 0초·0.5초에 쓰면 2초·4초에 회복). 어빌리티를 거치지 않고 적용되면 경고와 함께 즉시 만료된다. UI는 GE 지속시간이 아니라 `IWxUIData::GetCooldownTime()`을 충전 하나의 주기로 쓴다.
- `CooldownTime`이 0 이하면 `GetCooldownGameplayEffect`가 nullptr을 돌려 쿨다운이 없다. 지속시간 0인 GE는 만료 타이머가 걸리지 않기 때문이다.
- 쿨다운 무시(`UWxEffect_IgnoreCooldowns`)는 Infinite GE다. 순정 Immunity·RemoveOther 컴포넌트가 `Cooldown` 부모 태그를 부여하는 GE를 막고 걷는다. 코스트 무시(`UWxEffect_IgnoreCosts`)는 `Effect.IgnoreCosts` 태그를 세우고 `UWxAbilityBase`가 코스트 검사·적용을 건너뛴다. 순정 CheckCost는 면역이 아니라 어트리뷰트를 보기 때문이다. 둘 다 소환물 AbilitySet이 부여한다.
- AbilitySet의 `GrantedEffects`는 SetByCaller를 채우지 않는다. SetByCaller로 지속시간을 받는 GE를 넣으면 엔진이 경고 없이 1초로 둔다.
- `ActivationOwnedEffects`는 발동 수명에 묶는 효과 목록이다. 지속시간이 별도인 효과를 무조건 이 목록으로 옮기지 않는다. 종료 때 각 핸들의 스택 하나만 뺀다. 스택형 GE는 다른 소유자의 적용과 한 핸들로 합쳐지기 때문이다.

## 검증

에디터 검증기(`UEditorValidatorBase` 파생)는 두지 않고 `IsDataValid`만 쓴다. 블루프린트 컴파일도 CDO의 `IsDataValid`를 부른다.

- GA_: 쿨다운 시간이 있는데 `CooldownTags`가 비면 오류다.
- 패시브: 트리거는 GameplayEvent여야 하고 서로 조상 관계면 안 된다(오류). 조상과 자식을 함께 걸면 한 이벤트에 두 번 발동한다.
- 세트: 풀리지 않는 속성 행은 오류다. 빈 칸, 같은 어빌리티 중복, 같은 입력의 발동 조건 겹침, 같은 쿨다운 태그인데 시간이나 충전 수가 다른 경우는 경고다. 엔진이 태그 조건을 protected로 두어 발동 조건 겹침은 `UWxAbilityBase::IsActivationExclusive`가 판정한다.
- 타입별 몽타주 섹션 규칙은 사용자 지시로 지웠다(2026-09-25, 필요하면 재검토). 규칙과 반례 시험 결과는 [작업 기록](../../../.agents/workflow/tasks/ability-table-driven.md)의 4단계 절에 있다.

## 몽타주 구간 상태 GE

무적·퍼펙트 가드처럼 몽타주 구간 동안만 거는 상태 GE는 `UWxAnimNotifyState_ApplyGameplayEffect`로 건다. 적용은 `UWxCombatLibrary::ApplyEffect`가 하고, 적용한 핸들을 돌려준다.

- `EffectClass`는 지속시간이 없는(Infinite) GE여야 한다. Instant나 HasDuration이면 구간이 성립하지 않는다.
- 서버 권위에서만 걸고 클라이언트는 GE 복제를 따른다. 비동기 노티파이라 발동의 예측 키를 재사용하지 않는다.
- 끝에서는 그 구간이 건 핸들의 스택 하나만 뺀다(2026-09-24, 커밋 `ce6295184`). 같은 GE를 건 구간이 겹쳐도(극한 회피·처형·컷신 무적, 가드→가드히트 퍼펙트 가드) 서로 걷어내지 않으므로 스택형이 아닌 GE도 쓸 수 있다. 이전에는 같은 클래스의 GE를 전부 한 스택씩 걷어 겹친 구간이 함께 사라졌다.
- 노티파이 객체는 몽타주 에셋에 하나라 여러 캐릭터가 공유한다. 그래서 구간별 핸들을 전역 고유한 몽타주 인스턴스 ID로 가른다. 큐 경로는 이벤트 참조의 `FAnimNotifyMontageInstanceContext`에서, 브랜칭 포인트 경로는 페이로드에서 ID를 받는다. 엔진 기본 브랜칭 구현은 빈 이벤트 참조를 넘겨 ID를 잃는다.
- 몽타주가 아닌 재생에서는 경고를 남기고 적용하지 않는다.
- 컷신 무적(`UWxSkillCutsceneComponent`)도 종료 때 자기 핸들의 스택 하나만 뺀다.

겹침·브랜칭 경로·비몽타주 미적용은 임시 자동화 테스트로 확인했고(테스트는 삭제), 플레이는 확인하지 않았다.

## 변경 시 확인

[AbilityBase](../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h)의 프로퍼티와 [구현](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp)의 활성화 조건·종료 정리·`IsDataValid`를 함께 본다. 부여와 세트 규칙은 [AbilitySet](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp)에서, 비용·충전 규칙은 [비용 GE](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cost.cpp)와 [쿨다운 GE](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cooldown.cpp)에서, 구간 상태 GE는 [구간 GE 노티파이](../../../Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp)에서 본다. GA_·세트·몽타주의 실제 값은 에셋 안에 있어 코드만으로 단정하지 않는다.

## 관련 문서

- [[ai|WxAI — AI 인지와 행동]] ([WxAI — AI 인지와 행동](../topics/ai.md))
- [[combat|WxCombat — 전투 시스템]] ([WxCombat — 전투 시스템](../topics/combat.md))
- [[combat-damage|피해 처리와 전투 연출]] ([피해 처리와 전투 연출](../concepts/combat-damage.md))
- [[combat-resources|전투 자원과 현광의 예외]] ([전투 자원과 현광의 예외](../concepts/combat-resources.md))

## Sources

- [근거 1](../../raw/notes/2026-09-22-current-combat.md)
- [어빌리티 비용·쿨다운 GE 정적 조사](../../raw/notes/2026-09-22-current-ability-cost-cooldown.md) — 비용·쿨다운 GE 구조
- [WxCombat 정리 네 건](../../raw/notes/2026-09-24-wxcombat-cleanup.md) — 구간 GE 노티파이의 자기 핸들 제거
- [WxCombat 불필요한 장치 정리](../../raw/notes/2026-09-24-wxcombat-machinery-cleanup.md) — 쿨다운·코스트 무시 구조, AbilitySet의 SetByCaller 제약, 발동 실패 로그 범위
- [어빌리티 규칙 변경과 몽타주 섹션 모델](../../raw/notes/2026-09-25-ability-montage-section-model.md) — 콤보 재시작, 늦은 노티파이 거르기, 섹션 모델, 컷신 중 입력 차단
- [어빌리티·GE 데이터를 에셋 한 곳으로](../../raw/notes/2026-09-25-ability-data-on-ga.md) — GA_ 데이터 배치, 공격 타입 분리, 세트 부여, 검증 규칙, DT_Ability·DT_Effect 제거
- [쿨다운 GE를 하나로](../../raw/notes/2026-09-25-cooldown-single-ge.md) — 공용 쿨다운 GE, CooldownTags, 차례 회복 MMC, 순정 쿨다운 태그 규칙

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 비용·쿨다운 GE 설명은 HEAD `60c324c714b1dab10cd48d36cabad63ace232716` 기준으로 보강했다. 2026-09-24에 몽타주 구간 상태 GE 절과 발동 소유 효과의 제거 방식을 HEAD `ca84c9aac` 코드와 대조해 추가했다. 같은 날 쿨다운·코스트 무시, AbilitySet의 SetByCaller 제약, 발동 실패 로그 범위를 미커밋 작업 트리 코드와 대조해 추가했다(빌드 통과, 인게임 미검증). 2026-09-25에 타입과 GA_, 몽타주 섹션, 비용과 쿨다운, 검증 절을 HEAD `d63ce0630` 코드와 대조해 다시 썼다. 같은 날 쿨다운 GE 통합을 미커밋 작업 트리(HEAD `38d4dde08`) 코드와 대조해 반영했다(빌드·데이터 검증 통과, 인게임 미검증). GA_·세트 값과 PIE 부여 결과는 원자료의 에디터·PIE 확인을 따르며, HGTest·분신·도플갱어와 조작감은 인게임으로 확인하지 않았다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. `verified`는 순정 규칙에 따른 편찬일이다.

빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이 문서가 직접 검증하지 않았다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

</details>

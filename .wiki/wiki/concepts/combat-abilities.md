---
title: "전투 어빌리티와 이펙트"
category: concept
sources:
  - "raw/notes/2026-09-26-cooldown-play-acceptance.md"
  - "raw/notes/2026-09-26-ability-ga-play-acceptance.md"
  - "raw/notes/2026-09-26-module-review-contracts.md"
  - "raw/notes/2026-09-25-exclusive-submission-cleanup.md"
  - "raw/notes/2026-09-25-ability-block-policy-centralization.md"
  - "raw/notes/2026-09-25-exclusive-tag-blocking.md"
  - "raw/notes/2026-09-25-ui-data-interface-removal.md"
  - "raw/notes/2026-09-22-current-combat.md"
  - "raw/notes/2026-09-22-current-ability-cost-cooldown.md"
  - "raw/notes/2026-09-24-wxcombat-cleanup.md"
  - "raw/notes/2026-09-24-wxcombat-machinery-cleanup.md"
  - "raw/notes/2026-09-25-ability-montage-section-model.md"
  - "raw/notes/2026-09-25-ability-data-on-ga.md"
  - "raw/notes/2026-09-25-cooldown-single-ge.md"
created: 2026-09-22
updated: 2026-09-26
tags: [wx, combat]
aliases: ["GAS"]
confidence: medium
volatility: warm
verified: 2026-09-26
summary: "어빌리티 하나는 데이터 전용 GA_ 하나다. C++ 타입이 규칙을, GA_가 몽타주·입력·수치·표시를 가진다. 콤보·패턴 단계는 몽타주 배열로, 방향·반응은 섹션으로 고른다."
---

# 전투 어빌리티와 이펙트

어빌리티 하나는 데이터 전용 GA_ 하나다. C++ 타입이 규칙을, GA_가 몽타주·입력·수치·표시를 가진다. 콤보·패턴 단계는 몽타주 배열로, 방향·반응은 섹션으로 고른다.

## 타입과 GA_

- C++ 파생 클래스가 타입이다. 생성자에서 에셋·소유 태그, 취소 관계, 발동 그룹, 홀드·토글 같은 고유 기본값을 정한다. 그룹 공통 차단은 베이스와 ASC가 맡는다. GA_는 부모 타입을 고르고 콘텐츠를 채운다. GA_에서 규칙 칸을 바꾸지 않고 BP 그래프에 로직을 두지 않는 것은 관례다(엔진 칸을 숨기지 않기로 한 사용자 결정, 2026-09-25).
- GA_가 채우는 Wx 프로퍼티: `ActivationInputAction`, `AbilityMontage`(콤보 계열은 `ComboMontages`, 현재 선택은 `GetMontage()`), `ActivationOwnedEffects`(타입 기본값 뒤에 더함), `CooldownTime`·`CooldownTags`·`MaxRecharges`, `CostResource`(Custom/SP/MP/UP)·`CostAmount`, `Title`·`Description`·`Icon`. `ActivationGroup`과 타입 튜닝 값(락온·질주·넉업 속도·상호작용 사거리·처형 문구)도 `EditDefaultsOnly`다.
- GA_가 채우는 엔진 칸: 캐릭터 상태 조건(`Master.*`)은 `ActivationRequiredTags`·`ActivationBlockedTags`, 패시브 트리거는 `AbilityTriggers`다. BT가 부르는 번호 태그(`Ability.Skill.N`, `Ability.Pattern.N`)는 `AbilityTags`와 `ActivationOwnedTags`에 함께 더한다.
- 공격 타입은 넷이다. `UWxAbility_Attack_Light`(공중·회피 중 금지), `_Heavy`(Light 취소, 공중·회피 중 금지), `_Air`(`Movement.InAir` 필요), `_DodgeCounter`(`Ability.Dodge` 필요, 공중 금지). `UWxAbility_Attack`은 콤보 로직을 가진 추상 기반이다. 스킬 타입의 식별 태그는 `Ability.Skill`이다.
- 콘텐츠가 있어야 하는 타입은 `Abstract`라 GA_로만 부여된다(공격 넷, Dodge, Finisher, Guard, GuardReact, HitReact, Pattern, Skill, Passive, WxGame의 Interact·UseItem). Death·Groggy·LockOn·Sprint·Ultimate·PlayMontageOnce는 구체 클래스다.
- 데이터 배치 규칙(2026-09-25 사용자 확정): 행이 에셋과 1:1이면 값은 그 에셋에 둔다. 그래서 어빌리티 표(`DT_Ability`)와 효과 표(`DT_Effect`)를 없앴다. 테이블은 짝 에셋이 없고 여러 곳이 골라 쓰는 정의(`DT_Damage`)나 레벨별 수치(`FScalableFloat`+CurveTable)에만 쓴다. 어빌리티를 테이블 행으로 구동하는 구조를 구현했다가 되돌렸고, 판단 근거는 AI 작업 편의(순정 구조, 값이 한 곳, 로그·git에서 이름으로 보임)다.
- `UWxAbilitySet`은 최대값→현재값 속성 초기화, `GrantedEffects` 적용, `GrantedAbilities` 부여 순서로 동작한다. 부여는 `FGameplayAbilitySpec(클래스, 1)`이고 `SourceObject`를 싣지 않는다. 캐릭터 ASC에 같은 클래스가 이미 있으면 런타임에서 조용히 건너뛴다. 중복 경고는 에디터 `IsDataValid`가 담당한다. ASC 재등록 시에는 누락된 능력만 보충하고 속성·GE 초기화를 반복하지 않는다. 세트 사이에 같은 어빌리티가 겹치면 이벤트로 도는 어빌리티가 한 번에 두 번 반응하기 때문이다. 같은 입력의 어빌리티가 여럿이면 세트 순서대로 시도해 처음 성공한 것을 쓴다.

## 발동과 취소

`UWxAbilityBase`는 InstancedPerActor와 LocalPredicted를 기본값으로 둔다. `GetAbilityBlockTags`가 개별 `BlockAbilitiesWithTag` 선언에 `ActivationGroup`과 에셋 태그의 공통 규칙을 합친다. Independent에는 공통 차단을 더하지 않고, Exclusive·Override에는 Attack·Skill·Pattern·Ultimate·Dodge·Guard·UseItem·Interact·Jump를 더한다. 그룹은 입력 버퍼 분류·콤보/후딜 전이·Override 취소 면역에도 쓰인다.

ASC의 순정 확장 지점 `ApplyAbilityBlockAndCancelTags`는 전달받은 목록에 계산 결과를 합쳐 `Super`를 호출한다. 실제 차단 횟수와 취소 수명은 엔진이 처리한다. 자식의 공통 설정 호출·재정의는 없으며, 구체 클래스에도 의존하지 않는다. 에디터의 `BlockAbilitiesWithTag`에는 개별 선언만 보이고 최종 결과는 `GetAbilityBlockTags`로 조회한다. GA_에 자동 목록을 저장하지 않아 기존 에셋 재저장이 필요하지 않다.

가드 반응·피격·처형 등 Override의 식별 태그는 공통 대상 목록에 없어 중첩 진입할 수 있고, Override는 `CanBeCanceled`로 취소를 거부한다. Death는 사망 고유의 `Ability` 부모 전체 차단을 명시한다. 새로운 액션 태그 분류를 추가하면 공통 대상 목록과 개별 허용 관계를 검토한다.

엔진은 발동 가능 검사를 통과한 뒤 `PreActivate`에서 `CancelAbilitiesWithTag`를 처리한다. 취소 선언이 차단을 뚫지는 않는다. 그래서 Exclusive이며 `Ability.Attack.Light` 태그를 가진 어빌리티는 공통 Attack 부모 대신 Light·Air·DodgeCounter만 막아 Heavy를 허용한다. 강공격의 취소 태그가 약공격을 끝내며, 개별 선언한 추가 차단은 이 예외가 지우지 않는다. 클래스 통합을 나중에 검토해도 그룹·태그 데이터와 고유 동작을 보존하면 이 공통 규칙은 유지할 수 있다.

적용·해제·콤보 자기 기여 조회는 같은 `GetAbilityBlockTags`를 쓴다. 차단 목록은 액션 단계·체력·월드 같은 가변 상태에 의존하지 않으며, 그룹·식별 태그·명시 차단 선언도 활성 수명 동안 고정한다. Recovery는 목록을 바꾸는 대신 엔진의 차단 해제 API를 사용한다.

발동 실패 사유는 엔진이 자기 판정(태그·쿨다운·코스트 등)만 `LogAbilitySystem` Verbose로 남긴다. 배타 차단도 이제 이 태그 판정에 포함된다. Wx 고유 판정(가드 입력·질주 SP·컷신 사용 중)의 실패는 로그가 없다. 엔진의 로그 게이트 `FScopedCanActivateAbilityLogEnabler`는 GAS 모듈 밖에서 링크되지 않는다. 게이트 없는 로그는 UI·상호작용이 `CanActivateAbility`를 조회할 때마다 찍히므로 쓰지 않는다.

`Exclusive`의 실행 단계는 `Blocking`, `ComboWindow`, `Recovery`다. 콤보 창에서는 활성 인스턴스 자신이 등록한 차단 횟수 1건만 조회에서 제외해 자기 재발동을 허용한다. 같은 태그의 다른 스펙·다른 어빌리티·GE가 건 차단은 유지한다. ASC의 직접 등록 횟수(`GetExplicitTagCount`)를 읽으며, 가용성 조회 중 차단 컨테이너를 수정하지 않는다. 다음 단에서는 기존처럼 소유자 `ActivationRequiredTags`·`ActivationBlockedTags`를 재검사하지 않지만, 전달된 Source·Target 태그 조건은 검사한다.

Recovery는 `SetShouldBlockOtherAbilities(false)`로 자기 차단만 해제한다. 기존 동작의 종료는 다음 배타 발동이나 점프가 호출하는 `CancelRecoveringAbilities`가 맡는다. 점프는 GA로 만들지 않고 `Ability.Jump`의 차단 여부를 조회한다. `OpenComboWindow`와 `StartRecovery`는 입력 버퍼를 다시 처리하고, `CloseComboWindow`는 이미 Recovery로 넘어간 동작을 Blocking으로 되돌리지 않는다.

도플갱어의 `UWxEffect_IgnoreAbilityActivationTags`는 소유자 발동 조건만 면제한다. 어빌리티·GE의 차단, 전달된 Source·Target 조건, 자기 콤보/후딜 규칙은 지킨다. 비용·쿨다운 면제는 별도 효과다. 기존 `IgnoreAbilityTags`의 모든 태그 검사 우회에서 범위를 좁힌 변경이다. 미러링의 재시도와 제한은 [[ai|WxAI — AI 인지와 행동]] ([WxAI — AI 인지와 행동](../topics/ai.md))에 있다.

- 콤보 창이 닫히면 `OnComboWindowClosed`가 공격·스킬의 단계를 되돌린다. 창이 닫힌 뒤 누르면 후딜 중이어도 1단부터다.
- 콤보 창·후딜 노티파이는 몽타주 인스턴스 ID를 넘기고, 어빌리티는 지금 재생 중인 인스턴스가 아니면 무시한다(`IsPlayingMontageInstance`). 끊긴 앞 단 몽타주도 블렌드아웃 동안 노티파이를 보내기 때문이다. 브랜칭 포인트 경로도 오버라이드한다. 엔진 기본 구현이 빈 이벤트 참조를 넘겨 인스턴스를 잃는다.
- 스킬 컷신 동안 `UWxSkillCutsceneComponent`가 모든 플레이어 ASC에 `UWxEffect_SkillCutscene`을 걸어 입력형 어빌리티(Attack·Skill·Ultimate·Dodge·Guard·UseItem·Sprint·LockOn·Interact)를 막는다. 피격·사망 같은 반응 어빌리티는 막지 않는다.

## 몽타주 단계와 방향 섹션

일반 어빌리티는 `AbilityMontage`를, 콤보 계열은 `UWxAbility_Combo::ComboMontages`를 사용한다. `PlayMontage`는 명시한 섹션이 있으면 그대로 재생한다. 섹션 미지정 또는 없는 섹션일 때 `<접두사>Forward`가 있으면 몸 기준 8방향을 골라 해당 섹션을 사용하고, 없는 방향은 Forward로 대체한다. 필요한 원격 서버 인스턴스는 클라이언트의 방향 데이터를 기다린다.

- 콤보·패턴: `ComboMontages` 배열과 `ComboIndex`로 단계를 선택한다. 인덱스가 INDEX_NONE이면 첫 몽타주를 조회하고, 유효하지 않은 인덱스면 nullptr이다. Attack·Skill은 재발동 시 다음 항목 또는 첫 항목을 선택하며, Pattern은 블렌드아웃 때 다음 항목을 재생한다. 번호 섹션 단계 함수 `GetComboStageCount`·`GetComboStageSection`은 현재 구현에 없다. 각 단계 안의 방향 변형은 공통 섹션 선택을 따른다.
- 회피: 8방향 섹션, `Backstep`, 극한 회피 `Success<방향>`. 이동 입력이 없으면 `Backstep`(없으면 `Back`), 구성되지 않은 방향은 `Forward`다.
- 가드 반응: `GuardHit`, `GuardKnockback`, `GuardBreak`, `PerfectGuard`. 정확한 반응 섹션이 없고 `<반응 이름>Forward`가 있으면 공통 방향 선택으로 확장된다. 피격 반응도 반응 태그 끝 이름의 섹션 또는 그 이름에 Forward를 붙인 섹션을 가진 어빌리티가 반응한다([피해 처리와 전투 연출](combat-damage.md)).
- 착지: 공중에서 도는 몽타주(공중 공격·넉업)는 `Grounded` 섹션을 둔다. `UWxCharacterMovementComponent::JumpToLandingSection`이 재생 중인 모든 몽타주 인스턴스에서 `UWxAbilityBase::LandingSectionName`을 찾아 옮긴다.
- 처형의 짝 몽타주·피해 행과 궁극기 컷신은 몽타주 노티파이가 담는다([처형](combat-finisher.md)). 궁극기는 재생 전에 `UWxAnimNotify_SkillCutscene` 표식을 읽어 컷신 → 몽타주 순서로 재생한다.
- 섹션 편집 주의: 0초가 아닌 섹션 시작에 놓인 노티파이는 앞 섹션 끝에서 불린다(`AnimMontage.cpp:887`). 섹션 경계를 부동소수 오차만큼 넘은 구간은 다음 섹션을 트는 새 인스턴스가 이어받는다(`AnimInstance.cpp:1890`). 몽타주 도구의 `SnapNotifyEndsToSections`·`SnapNotifyStartsToSections`로 경계를 맞춘다([편집기 도구](../references/editor-tools.md)).

## 비용과 쿨다운

- 비용 GE `UWxEffect_Cost`는 모든 어빌리티가 함께 쓰는 Instant 효과다(`CostGameplayEffectClass` 기본값). MP·UP·SP에 Additive 모디파이어를 하나씩 두고, 각 MMC가 소스 어빌리티 CDO(`GetAbility()`)의 `CostResource`가 자기 자원일 때만 `-CostAmount`를 낸다. 엔진 순정 CheckCost·ApplyCost를 그대로 쓰며, 순정 자원 부족 판정이 Additive 모디파이어만 보기 때문에 Additive를 쓴다.
- 쿨다운 GE `UWxEffect_Cooldown`도 모든 어빌리티가 함께 쓴다(`CooldownGameplayEffectClass` 기본값). 쿨다운의 식별자는 GA_의 `CooldownTags`(`Cooldown.*`)다. `ApplyCooldown`이 공용 GE 스펙의 `DynamicGrantedTags`에 붙이고 `GetCooldownTags()`가 돌려주므로 순정 쿨다운 판정·쿼리가 그대로 본다. 같은 태그를 고른 어빌리티끼리 쿨다운을 나눠 쓴다. 엔진은 스택을 GE 클래스로만 합쳐 태그를 가리지 않으므로 공용 GE는 쌓지 않는다(2026-09-25 그룹별 파생 클래스를 합침).
- 소모한 충전 하나가 쿨다운 GE 하나다. `UWxAbilityBase::CheckCooldown`은 `CooldownTags`로 찾은 활성 GE가 `MaxRecharges`보다 적으면 통과시킨다. `ApplyCooldown`이 적용 전에 같은 태그 쿨다운의 최대 남은 시간에 `CooldownTime`을 더해 `SetByCaller.Duration`으로 넘기므로 충전이 차례로 돌아온다(회피 2초·충전 2를 0초·0.5초에 쓰면 2초·4초에 회복). 이 계산을 MMC에 두면 안 된다. 엔진이 GE를 활성 목록에 넣은 뒤 지속시간을 다시 계산해 MMC가 자기 자신을 대기열로 센다(1회 사용에 4초가 됐던 결함). UI는 GE 지속시간이 아니라 WxGame 리졸버가 `UWxAbilityBase::GetCooldownTime()`에서 전달한 값을 충전 하나의 주기로 쓴다.
- `CooldownTime`이 0 이하면 `GetCooldownGameplayEffect`가 nullptr을 돌려 쿨다운이 없다. 지속시간 0인 GE는 만료 타이머가 걸리지 않기 때문이다.
- 쿨다운 무시(`UWxEffect_IgnoreCooldowns`)는 Infinite GE다. 순정 Immunity·RemoveOther 컴포넌트가 `Cooldown` 부모 태그를 부여하는 GE를 막고 걷는다. 코스트 무시(`UWxEffect_IgnoreCosts`)는 `Effect.IgnoreCosts` 태그를 세우고 `UWxAbilityBase`가 코스트 검사·적용을 건너뛴다. 순정 CheckCost는 면역이 아니라 어트리뷰트를 보기 때문이다. 둘 다 소환물 AbilitySet이 부여한다.
- AbilitySet의 `GrantedEffects`는 SetByCaller를 채우지 않는다. SetByCaller로 지속시간을 받는 GE를 넣으면 엔진이 경고 없이 1초로 둔다.
- `ActivationOwnedEffects`는 발동 수명에 묶는 효과 목록이다. 지속시간이 별도인 효과를 무조건 이 목록으로 옮기지 않는다. 종료 때 각 핸들의 스택 하나만 뺀다. 스택형 GE는 다른 소유자의 적용과 한 핸들로 합쳐지기 때문이다.

## 검증

공용 쿨다운 GE는 이우성이 2026-09-25 회피 1회 뒤 쿨다운·UI 진행률, 소환물의 쿨다운 무시 발동, 리슨 서버와 클라이언트 PIE의 쿨다운 표시·차례 회복 일치, `UWxEffect_Cooldown`·`ApplyCooldown`·`CheckCooldown` 코드 리뷰를 모두 통과시켰다. 이전 소환물·네트워크 미확인 설명은 이 범위에서 대체된다. 전용 서버나 지연·패킷 손실 조건 전체의 검증을 뜻하지 않으며, 이번 Wiki 정리에서 실행 검증을 반복하지 않았다([쿨다운 사람 확인 근거](../../raw/notes/2026-09-26-cooldown-play-acceptance.md)).

GA_ 복귀 뒤 남아 있던 사람 플레이 확인은 이우성이 2026-09-25 통과시켰다. 범위는 HGTest·분신·도플갱어 어빌리티의 발동과 연출, 블렌드 인 0.05초 통일 후 조작감, 가드 피해 절반 경감과 방패 버프 아이콘, 템플릿 패시브의 UP 5 지급, 어빌리티 데이터의 타게팅 프리셋을 사용하는 락온이다. 최종 결정은 테이블 전환 없이 GA_를 유지하는 것이다. 이 결과는 해당 작업의 과거 플레이 미확인을 대체하며, 후속 차단 규칙 변경이나 네트워크 예측·복제 전체의 검증을 뜻하지 않는다([사람 확인 근거](../../raw/notes/2026-09-26-ability-ga-play-acceptance.md)).

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

[AbilityBase](../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h)의 프로퍼티와 [구현](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp)의 활성화 조건·종료 정리·`IsDataValid`를 함께 본다. 부여와 세트 규칙은 [AbilitySet](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp)에서, 비용·충전 규칙은 [비용 GE](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cost.cpp)와 [쿨다운 GE](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cooldown.cpp)에서, 구간 상태 GE는 [구간 GE 노티파이](../../../Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp)에서 본다. GA_·세트·몽타주의 실제 값은 에셋 안에 있어 코드만으로 단정하지 않는다. GA_·몽타주 저장값은 [어빌리티 목록](../references/ability-list.md)에서, 캐릭터별 WxAbilitySet과 속성 초기값은 [캐릭터 목록](../references/character-list.md)에서, 이펙트의 GE_ 저장값과 에셋·코드 사용처는 [이펙트 목록](../references/effect-list.md)에서 본다.

## 관련 문서

- [[ability-list|어빌리티 목록]] ([어빌리티 목록](../references/ability-list.md))
- [[ai|WxAI — AI 인지와 행동]] ([WxAI — AI 인지와 행동](../topics/ai.md))
- [[character-list|캐릭터 목록]] ([캐릭터 목록](../references/character-list.md))
- [[combat|WxCombat — 전투 시스템]] ([WxCombat — 전투 시스템](../topics/combat.md))
- [[combat-damage|피해 처리와 전투 연출]] ([피해 처리와 전투 연출](../concepts/combat-damage.md))
- [[combat-resources|전투 자원과 현광의 예외]] ([전투 자원과 현광의 예외](../concepts/combat-resources.md))
- [[effect-list|이펙트 목록]] ([이펙트 목록](../references/effect-list.md))

## Sources

- [공용 쿨다운 GE의 사람 확인 범위](../../raw/notes/2026-09-26-cooldown-play-acceptance.md) — 회피 UI·소환물 쿨다운 무시·리슨 서버와 클라이언트 차례 회복 및 코드 리뷰 통과

- [GA_ 유지 결정과 사람 플레이 확인 범위](../../raw/notes/2026-09-26-ability-ga-play-acceptance.md) — GA_ 복귀 후 남아 있던 7개 사람 테스트 통과

- [재등록 수정과 콤보 배열 계약 확인](../../raw/notes/2026-09-26-module-review-contracts.md) — 콤보 배열·공통 방향 섹션과 AbilitySet 부여 경계

- [Exclusive 차단 테스트 제거와 주석 정정](../../raw/notes/2026-09-25-exclusive-submission-cleanup.md) — 임시 테스트 제거, 검증 이력 보존, 태그 차단·취소 면역 설명 정정

- [어빌리티 공통 차단 규칙의 ASC 확장 지점 통합](../../raw/notes/2026-09-25-ability-block-policy-centralization.md) — 자식의 공통 차단 제거, 그룹·태그 기반 계산, 순정 ASC 확장과 회귀 검증

- [Exclusive 태그 차단과 도플갱어 발동 조건 면제](../../raw/notes/2026-09-25-exclusive-tag-blocking.md) — 순정 차단·취소 수명, 콤보 예외, 효과 이름과 에셋 참조 이관

- [UI 데이터 인터페이스 제거와 리졸버 연결](../../raw/notes/2026-09-25-ui-data-interface-removal.md) — 2026-09-25 사용자 합의와 구현

- [근거 1](../../raw/notes/2026-09-22-current-combat.md)
- [어빌리티 비용·쿨다운 GE 정적 조사](../../raw/notes/2026-09-22-current-ability-cost-cooldown.md) — 비용·쿨다운 GE 구조
- [WxCombat 정리 네 건](../../raw/notes/2026-09-24-wxcombat-cleanup.md) — 구간 GE 노티파이의 자기 핸들 제거
- [WxCombat 불필요한 장치 정리](../../raw/notes/2026-09-24-wxcombat-machinery-cleanup.md) — 쿨다운·코스트 무시 구조, AbilitySet의 SetByCaller 제약, 발동 실패 로그 범위
- [어빌리티 규칙 변경과 몽타주 섹션 모델](../../raw/notes/2026-09-25-ability-montage-section-model.md) — 콤보 재시작, 늦은 노티파이 거르기, 섹션 모델, 컷신 중 입력 차단
- [어빌리티·GE 데이터를 에셋 한 곳으로](../../raw/notes/2026-09-25-ability-data-on-ga.md) — GA_ 데이터 배치, 공격 타입 분리, 세트 부여, 검증 규칙, DT_Ability·DT_Effect 제거
- [쿨다운 GE를 하나로](../../raw/notes/2026-09-25-cooldown-single-ge.md) — 공용 쿨다운 GE, CooldownTags, 차례 회복 SetByCaller, 순정 쿨다운 태그 규칙, 4초 쿨다운 결함

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 비용·쿨다운 GE 설명은 HEAD `60c324c714b1dab10cd48d36cabad63ace232716` 기준으로 보강했다. 2026-09-24에 몽타주 구간 상태 GE 절과 발동 소유 효과의 제거 방식을 HEAD `ca84c9aac` 코드와 대조해 추가했다. 같은 날 쿨다운·코스트 무시, AbilitySet의 SetByCaller 제약, 발동 실패 로그 범위를 미커밋 작업 트리 코드와 대조해 추가했다(빌드 통과, 인게임 미검증). 2026-09-25에 타입과 GA_, 몽타주 섹션, 비용과 쿨다운, 검증 절을 HEAD `d63ce0630` 코드와 대조해 다시 썼다. 같은 날 쿨다운 GE 통합을 미커밋 작업 트리(HEAD `38d4dde08`) 코드와 대조해 반영했다(빌드·데이터 검증·임시 자동화 테스트 통과, 회피 쿨다운과 UI 진행률은 당시 사용자 인게임 확인). 당시 미확인이던 소환물 쿨다운 무시·네트워크 복제와 코드 리뷰는 후속 사람 체크리스트에서 통과했으며, 구체적인 확인 범위는 검증 절과 2026-09-26 추가 원자료를 따른다. GA_·세트 값과 PIE 부여 결과는 원자료의 에디터·PIE 확인을 따르며, 당시 미확인이던 HGTest·분신·도플갱어와 조작감은 2026-09-25 사람 테스트로 확인됐으며, 가드 경감·버프 아이콘·템플릿 패시브 UP·락온을 포함한 7개 항목의 범위는 검증 절과 추가 원자료를 따른다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. `verified`는 순정 규칙에 따른 편찬일이다.

2026-09-25에 Exclusive 태그 차단 전환 후 생성자 선언을 ASC 확장 지점으로 공통화했다. 빌드·임시 GAS 회귀 3개(AssetDefaults·HookRules·Lifecycle)·GA 40개의 계산된 차단 관계 1,600건·점프 40건·리다이렉트 없는 도플갱어 효과 재로드를 확인했다. 해당 임시 테스트 코드와 전용 friend는 사용자 요청으로 제출 전에 제거했으며 실행 결과는 당시 검증 근거로 보존한다. 자식 클래스 자체의 통합은 향후 검토다. 몽타주를 생략한 ServerOnly 테스트이므로 실제 선입력·도플갱어 BT 타이밍·UI·예측/복제와 사람의 코드 리뷰는 미확인이다.

그 밖의 빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이 문서가 직접 검증하지 않았다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

2026-09-26: ad0db6de0 작업 트리의 콤보 배열·공통 방향 섹션·AbilitySet 부여 계약을 정적으로 대조해 해당 설명만 정정했다. 빌드·플레이·예측/복제 실행을 새로 검증하지 않았다.

</details>

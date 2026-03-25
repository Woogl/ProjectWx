# 프로젝트 분석 보고서 (2026-03-25)

## 프로젝트 개요

Unreal Engine 5.7 기반 오픈월드 액션 RPG 프로젝트. 최대 4인 PC 멀티플레이어 지원. Gameplay Ability System(GAS) 기반 전투, CommonUI + MVVM 기반 UI, 플러그인 분리 아키텍처로 구성되어 있다.

---

## 모듈 구조 및 의존성

```
WxGame (게임 모듈)
├── WxCore ✓
├── WxCombat ✓
├── WxUI ✓
└── GameplayAbilities, AIModule, NavigationSystem 등

WxCombat (전투 플러그인)
├── WxCore ✓
├── GameplayAbilities, GameplayTags, GameplayTasks
├── Niagara, UMG, LevelSequence, MovieScene
└── AIModule (AISense_Damage)

WxUI (UI 플러그인)
├── WxCore ✓
├── CommonUI, ModelViewViewModel
└── GameplayAbilities, GameplayTags

WxCore (공용 플러그인)
└── Core, CoreUObject, GameplayTags
```

**의존성 규칙 준수 여부**: 모든 플러그인이 `WxCore`에만 의존하고 플러그인 간 직접 의존이 없다. **규칙 준수 ✓**

---

## 클래스별 상세 분석

### WxCore

| 클래스/파일 | 설명 |
|---|---|
| `WxGameplayTags` (namespace) | 프로젝트 전체 Native Gameplay Tag 정의 |
| `WxCollisionChannels.h` | 커스텀 충돌 채널 (`Attack = ECC_GameTraceChannel1`) |
| `FWxAssetUtils` | 비동기 에셋 로딩 유틸리티 (템플릿) |
| `EWxTeam` | 팀 구분 enum (Player, Enemy, Neutral) |

### WxCombat

**어빌리티 시스템:**

| 클래스 | 상속 | 설명 |
|---|---|---|
| `UWxAbility` | `UGameplayAbility` | 베이스 어빌리티. MPCost, CooldownDuration, CooldownTag, ActivationInputTag |
| `UWxAbility_Attack` | `UWxAbility` | 콤보 공격. NextComboMontage 체인, ANS_ComboWindow 기반 콤보 입력 |
| `UWxAbility_Guard` | `UWxAbility` | 가드. 몽타주 + ANS_Guard 태그 |
| `UWxAbility_Dodge` | `UWxAbility` | 회피. ANS_Invincible 무적 프레임 |
| `UWxAbility_Jump` | `UWxAbility` | 점프 |
| `UWxAbility_Sprint` | `UWxAbility` | 달리기 |
| `UWxAbility_Skill` | `UWxAbility` | 스킬 (몽타주 기반) |
| `UWxAbility_Ultimate` | `UWxAbility` | 컷씬(LevelSequence) → 몽타주 2단계. 컷씬 중 무적 + TimeDilation |
| `UWxAbility_Death` | `UWxAbility` | 사망 처리 (래그돌) |
| `UWxAbility_Groggy` | `UWxAbility` | 그로기 상태 (DP 드레인, 몽타주) |
| `UWxAbility_HitReact` | `UWxAbility` | 피격 리액션 (가드/일반 분기) |
| `UWxAbility_LockOn` | `UWxAbility` | 락온 토글 |

**어빌리티 태스크:**

| 클래스 | 설명 |
|---|---|
| `UWxAbilityTask_PlayCutscene` | LevelSequence 재생 + GlobalTimeDilation 관리 |

**어빌리티 시스템 컴포넌트 & 세트:**

| 클래스 | 설명 |
|---|---|
| `UWxAbilitySystemComponent` | 커스텀 ASC. 입력 태그 기반 어빌리티 활성화 (`ActivateAbilityByInputTag`) |
| `UWxAbilitySet` | 어빌리티/이펙트/어트리뷰트 일괄 부여 데이터 에셋 |
| `UWxCombatAttributeSet` | HP, MaxHP, MP, MaxMP, ATK, DEF, SPD, CritRate, CritDMG, DP, MaxDP, IncomingDamage |

**이펙트 & ExecCalc:**

| 클래스 | 설명 |
|---|---|
| `UWxDamageExecCalc` | 핵심 대미지 파이프라인. 무적/퍼펙트가드/가드/일반 분기, 치명타, DP, GameplayCue, HitReact |
| `UWxEffect_Cost` | SetByCaller 기반 MP 코스트 GE |
| `UWxEffect_MPRecovery` | CDO 기반 피격 시 MP+5 회복 GE |
| `UWxEffect_Reflect` | CDO 기반 퍼펙트가드 DP 반사 GE (SetByCaller) |

**애니메이션 노티파이:**

| 클래스 | 설명 |
|---|---|
| `UWxAnimNotifyState_WeaponCollision` | 무기 충돌 판정 구간 |
| `UWxAnimNotifyState_Guard` | 가드 판정 구간 |
| `UWxAnimNotifyState_Invincible` | 무적 구간 |
| `UWxAnimNotifyState_PerfectGuard` | 퍼펙트 가드 구간 |

**무기 & 투사체:**

| 클래스 | 설명 |
|---|---|
| `AWxWeaponBase` | 무기 베이스. BoxComponent 충돌, 멀티히트 방지 |
| `AWxProjectileBase` | 투사체 베이스. ProjectileMovementComponent |

**데미지 플로터 & GameplayCue:**

| 클래스 | 설명 |
|---|---|
| `UWxGameplayCueNotify_Damage` | GameplayCue_Damage 처리. 플로터 액터 + 히트 Niagara 스폰 |
| `AWxDamageFloaterActor` | 데미지 수치 위젯 표시 액터 |
| `IWxDamageFloaterInterface` | 데미지 플로터 초기화 인터페이스 |

### WxUI

| 클래스 | 설명 |
|---|---|
| `UWxViewModel` | 베이스 ViewModel |
| `UWxViewModel_Attribute` | 어트리뷰트 바인딩 (HP/MaxHP/MP/MaxMP/DP/MaxDP) |
| `UWxViewModel_Ability` | 어빌리티 쿨타임 바인딩 |
| `UWxViewModel_GameplayTag` | 태그 존재 여부 바인딩 (State_Groggy, State_LockOn 등) |
| `UWxPrimaryGameLayout` | CommonUI 레이어 기반 레이아웃 (Game/GameMenu/Menu/Modal) |
| `UWxUIManagerSubsystem` | UI 매니저 서브시스템 |
| `UWxActivatableWidget` | CommonUI ActivatableWidget 래퍼 |
| `UWxAsyncAction_PushWidgetToLayer` | 비동기 위젯 레이어 푸시 |
| `UWxMVVMConversionLibrary` | GameplayTag → Bool/Visibility 변환 |

### WxGame

| 클래스 | 설명 |
|---|---|
| `AWxCharacterBase` | 캐릭터 베이스. ASC, 팀, 무기, 사망/에어본 처리 |
| `AWxPlayerCharacter` | 플레이어. 3인칭 카메라, 입력 태그 바인딩 |
| `AWxEnemyCharacter` | 적. BehaviorTree, 네임플레이트 ViewModel |
| `AWxPlayerController` | 플레이어 컨트롤러. 입력 데이터, HUD, ViewModel 초기화 |
| `AWxEnemyController` | 적 AI 컨트롤러. AI Perception, BT, 사망 처리 |
| `UWxBTTask_ActivateAbility` | BT 태스크 → 태그 기반 어빌리티 활성화 |

---

## Gameplay Tag 현황

### State 태그
| 태그 | 용도 |
|---|---|
| `State.Dead` | 사망 상태 |
| `State.Airborne` | 공중 상태 |
| `State.Groggy` | 그로기 상태 |
| `State.LockOn` | 락온 상태 |

### Event 태그
| 태그 | 용도 |
|---|---|
| `Event.HitReact` | 피격 리액션 이벤트 |

### ANS 태그 (AnimNotifyState)
| 태그 | 용도 |
|---|---|
| `ANS.WeaponCollision` | 무기 충돌 판정 구간 |
| `ANS.ComboWindow` | 콤보 입력 허용 구간 |
| `ANS.Invincible` | 무적 구간 |
| `ANS.Guard` | 가드 구간 |
| `ANS.PerfectGuard` | 퍼펙트 가드 구간 |

### GameplayCue 태그
| 태그 | 용도 |
|---|---|
| `GameplayCue.Damage` | 데미지 플로터 + 히트 이펙트 |

### Damage 태그
| 태그 | 용도 |
|---|---|
| `Damage.Critical` | 치명타 표시 |

### Ability 태그
| 태그 | 용도 |
|---|---|
| `Ability` | 어빌리티 루트 |
| `Ability.Attack` | 공격 |
| `Ability.Dodge` | 회피 |
| `Ability.Guard` | 가드 |
| `Ability.Jump` | 점프 |
| `Ability.Skill` | 스킬 |
| `Ability.Sprint` | 달리기 |
| `Ability.Ultimate` | 궁극기 |

### Cooldown 태그
`Cooldown.Attack`, `Cooldown.Dodge`, `Cooldown.Guard`, `Cooldown.Jump`, `Cooldown.Skill`, `Cooldown.Ultimate`

### SetByCaller 태그
| 태그 | 용도 |
|---|---|
| `SetByCaller.Cost.MP` | MP 코스트 |
| `SetByCaller.ReflectDP` | DP 반사량 |

### Input 태그
`Input.Jump`, `Input.Attack`, `Input.Dodge`, `Input.Guard`, `Input.Skill`, `Input.Sprint`, `Input.Ultimate`, `Input.LockOn`

### UI Layer 태그
`UI.Layer.Game`, `UI.Layer.GameMenu`, `UI.Layer.Menu`, `UI.Layer.Modal`

---

## 핵심 시스템 흐름

### 대미지 파이프라인

```
무기/투사체 충돌 (WxWeaponBase/WxProjectileBase)
  → ApplyGameplayEffectToTarget (DamageEffect + WxDamageExecCalc)
    → Execute_Implementation
      ├─ ANS_Invincible → return (대미지 무효)
      ├─ ANS_PerfectGuard → DP 반사 (WxEffect_Reflect) + return
      ├─ 대미지 계산: ATK × (100/(100+DEF))
      ├─ 치명타: CritRate% 확률, (1+CritDMG%) 배율
      ├─ ANS_Guard → 50% 감소
      ├─ IncomingDamage 적용 → AttributeSet.PostGameplayEffectExecute
      │   └─ HP 감소, HP≤0 → State_Dead 태그 + Event_HitReact
      ├─ DP 증가 → MaxDP 도달 시 State_Groggy
      ├─ GameplayCue_Damage → 데미지 플로터 + 히트 이펙트
      ├─ 공격자 MP+5 회복 (WxEffect_MPRecovery)
      ├─ AI 데미지 감지 (AISense_Damage)
      └─ Event_HitReact → HitReact 어빌리티
```

### 어빌리티 활성화 흐름

```
Input Action (Enhanced Input)
  → AWxPlayerCharacter::Input_XXX
    → UWxAbilitySystemComponent::ActivateAbilityByInputTag(Input.XXX)
      → 매칭되는 ActivationInputTag 가진 어빌리티 탐색
        → CanActivateAbility (코스트, 쿨다운, 태그 조건)
          → ActivateAbility
```

### UI 바인딩 흐름 (MVVM)

```
AttributeSet 변경
  → UWxViewModel_Attribute::HandleAttributeChanged (Delegate)
    → UE_MVVM_SET_PROPERTY_VALUE (HP, MaxHP 등)
      → FieldNotify → UMG Widget 자동 갱신

GameplayTag 변경
  → UWxViewModel_GameplayTag::HandleTagChanged (Delegate)
    → UE_MVVM_SET_PROPERTY_VALUE (bGroggy, bLockOn 등)
      → WxMVVMConversionLibrary::Conv_BoolToVisibility
        → Widget Visibility 갱신
```

---

## 코딩 컨벤션 검증 결과

| # | 규칙 | 상태 | 비고 |
|---|---|---|---|
| 1 | UE5 공식 코딩 컨벤션 | ✓ | |
| 2 | Wx 접두사 | ✓ | 모든 클래스에 적용 |
| 3 | Copyright 헤더 | ✓ | 모든 파일 첫 줄 |
| 4 | 함수 선언 줄바꿈 금지 | ✓ | |
| 5 | .h inline 함수 금지 | ✓ | 템플릿/constexpr만 예외 |
| 6 | 람다 최소화 | ✓ | 델리게이트 바인딩 등 필수 경우에만 사용 |
| 7 | if-else 중괄호 | ✓ | |
| 8 | Super:: 호출 | ✓ | override 함수에서 일관 적용 |
| 9 | Native Gameplay Tag | ✓ | WxGameplayTags.h/cpp에 집중 |
| 10 | Handle 콜백 접두사 | ✓ | HandleMontageEnded, HandleDeath 등 |
| 11 | BlueprintCallable 제한 | ✓ | WxAsyncAction_PushWidgetToLayer만 사용 |
| 12 | UFUNCTION/UPROPERTY 빈 줄 | ✓ | |
| 13 | UE 5.7 API | ✓ | Deprecated API 미사용 |
| 14 | 빌드 검증 | ✓ | 최근 커밋 빌드 성공 확인 |

---

## 최근 변경 사항

| 커밋 | 내용 |
|---|---|
| `04dbe03` | 궁극기(WxAbility_Ultimate, WxAbilityTask_PlayCutscene) 및 퍼펙트 가드(WxAnimNotifyState_PerfectGuard, WxEffect_Reflect) 구현. ExecCalc의 AddToRoot 패턴을 CDO 기반 GE(WxEffect_MPRecovery, WxEffect_Reflect)로 리팩터링 |
| `aa504b0` | 데미지 플로터 시스템(WxDamageFloaterActor, WxGameplayCueNotify_Damage, WxDamageFloaterInterface) 구현 및 설계 문서 최신화 |
| `375f7ad` | 스킬 및 문서 경로를 .claude/ 하위로 정리 |

---

## 개선 제안

### 1. 퍼펙트 가드 그로기 판정 타이밍 문제
`WxDamageExecCalc.cpp:88-95` — 퍼펙트 가드 DP 반사 후 그로기 판정에서 `SourceAttrSet->GetDP()`를 읽지만, 직전에 `ApplyGameplayEffectSpecToSelf`로 DP를 가산했다. GE 적용은 즉시(Instant)이므로 `GetDP()`는 이미 반영된 값을 반환할 가능성이 높지만, **ExecCalc 실행 중 Instant GE의 적용 시점이 보장되지 않을 수 있다**. 안전하게 하려면 `GetDP() + Reflect >= GetMaxDP()` 비교로 변경하는 것이 좋다.

### 2. WxAbilityTask_PlayCutscene TimeDilation 복원 안전성
컷씬 태스크에서 `GlobalTimeDilation = 0.001f` 설정 후 `OnDestroy`에서 복원한다. 만약 컷씬 중 레벨 전환이나 비정상 종료가 발생하면 TimeDilation이 복원되지 않을 수 있다. `AWorldSettings`의 기본값 보장이나 `GameInstance` 수준의 안전장치를 고려할 수 있다.

### 3. WxDamageExecCalc 단일 책임 과부하
ExecCalc이 대미지 계산 외에 MP 회복, GameplayCue 실행, AI 감지, HitReact 이벤트까지 담당하고 있다(193줄). 현재는 관리 가능한 수준이지만, 기능 추가 시 별도 후처리 클래스나 델리게이트로 분리를 고려할 수 있다.

### 4. WxAbility_Attack 콤보 몽타주 하드코딩
공격 어빌리티에서 `NextComboMontage`를 직접 체이닝하는 구조로, 콤보 확장 시 어빌리티 인스턴스가 증가한다. 데이터 에셋 기반 콤보 테이블로 전환하면 콤보 변경이 유연해진다.

### 5. ViewModel 초기화 위치
`AWxPlayerController`에서 3종 ViewModel(Attribute, Ability, GameplayTag)을 직접 초기화하고 있다. 적 캐릭터(`AWxEnemyCharacter`)에서도 별도로 Attribute ViewModel을 초기화한다. ViewModel 팩토리나 초기화 전략 패턴을 도입하면 중복을 줄일 수 있다.

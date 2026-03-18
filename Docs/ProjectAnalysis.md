# Wx 프로젝트 핵심 구조 보고서

## 1. 프로젝트 개요

| 항목 | 내용 |
|------|------|
| 엔진 | Unreal Engine 5.7 |
| 장르 | 오픈월드 액션 RPG |
| 플랫폼 | PC |
| 플레이어 | 최대 4인 멀티플레이어 |
| 소스 규모 | 50개 소스 파일 (헤더 + 구현) |

---

## 2. 모듈 아키텍처

```
WxGame (게임 모듈 — Source/)
├──→ WxCore   (공용 정의 — Plugins/)
├──→ WxCombat (전투 시스템 — Plugins/)
└──→ WxUI     (UI 시스템 — Plugins/)
```

**의존성 규칙**: 모든 플러그인은 `WxCore`에만 의존 가능. 플러그인 간 직접 의존 금지. `WxGame`이 모든 플러그인을 조합하여 구체적 컨텐츠를 구성한다.

### 2.1 WxCore — 공용 정의

프로젝트 전체가 공유하는 Gameplay Tag와 Collision Channel을 정의한다. 의존성은 `GameplayTags` 하나뿐으로 최소화되어 있다.

**Gameplay Tags** (`WxGameplayTags.h/cpp`):

| 카테고리 | 태그 | 용도 |
|---------|------|------|
| State | `State.Dead`, `State.Airborne` | 캐릭터 상태 |
| Event | `Event.HitReact` | 피격 이벤트 트리거 |
| ANS | `ANS.WeaponCollision`, `ANS.ComboWindow`, `ANS.Invincible`, `ANS.Guard` | 몽타주 구간 제어 |
| Ability | `Ability`, `Ability.Attack`, `Ability.Dodge`, `Ability.Guard` | 어빌리티 식별/차단 |
| Input | `Input.Jump`, `Input.Attack`, `Input.Dodge`, `Input.Guard` | 입력-어빌리티 매핑 |
| UI | `UI.Layer.Game`, `UI.Layer.GameMenu`, `UI.Layer.Menu`, `UI.Layer.Modal` | UI 레이어 식별 |

**Collision Channel** (`WxCollisionChannels.h`):
- `WxCollision::Attack` = `ECC_GameTraceChannel1` — 무기/투사체의 Object Type

---

### 2.2 WxGame — 게임 모듈

다른 플러그인들을 조합하여 실제 게임 컨텐츠를 만드는 모듈.

#### 캐릭터 계층

```
AWxCharacterBase (Abstract)
├── IAbilitySystemInterface 구현
├── UWxAbilitySystemComponent 소유
├── UWxAttributeSet 소유
├── UWxRagdollComponent 소유
├── 무기 스폰/장착 (EquippedWeapon, Replicated)
├── SPD 어트리뷰트 → MaxWalkSpeed 자동 연동
├── State.Airborne 태그 자동 관리 (낙하/비행 감지)
└── HandleDeath() → 래그돌 활성화, OnDeath 브로드캐스트
    │
    ├── AWxPlayerCharacter
    │   ├── 3인칭 카메라 (SpringArm 400cm)
    │   ├── ASC Replication: Mixed (클라이언트 예측)
    │   └── EnhancedInput → InputTag → ASC 어빌리티 바인딩
    │
    └── AWxEnemyCharacter
        ├── ASC Replication: Minimal (대역폭 최적화)
        ├── AI 자동 빙의 (PlacedInWorldOrSpawned)
        ├── WidgetComponent 체력바 + ViewModel_Health 바인딩
        └── 사망 시 체력바 숨김
```

#### 컨트롤러

```
AWxPlayerController
├── InputMappingContext / InputAction 관리
├── BeginPlay → GameHUD를 UI.Layer.Game에 푸시
└── Possess/OnRep_Pawn → 플레이어 Health ViewModel 전역 초기화

AWxEnemyController
├── AIPerception (시야 1200cm + 데미지 감지)
├── Blackboard (TargetActor, Alerted)
├── BehaviorTree 실행 (OnPossess)
├── 경계 시 FOV 90° → 180° 확장
└── 사망 시 BT 정지
```

#### AI

```
UWxBTTask_ActivateAbility
├── BT 태스크로 GAS 어빌리티 활성화
├── AbilityTag로 어빌리티 탐색 → TryActivateAbility
├── OnAbilityEnded 바인딩으로 성공/실패/캔슬 감지
└── 플레이어와 동일한 어빌리티 시스템 사용
```

---

### 2.3 WxCombat — 전투 시스템 (GAS 기반)

#### ASC + AbilitySet + AttributeSet

| 클래스 | 역할 |
|--------|------|
| `UWxAbilitySystemComponent` | 입력-어빌리티 매핑, 콤보 체인 관리, AbilitySet 부여 |
| `UWxAbilitySet` | DataAsset으로 어빌리티/이펙트 번들링, 일괄 부여/제거 |
| `UWxAttributeSet` | HP, MaxHP, MP, MaxMP, ATK, DEF, SPD, IncomingDamage(메타) |

#### 어빌리티 5종

| 어빌리티 | 입력 | 핵심 동작 | 특수 메커니즘 |
|---------|------|----------|-------------|
| **Attack** | `Input.Attack` | 타겟 회전 → 공격 몽타주 | 콤보 체인 (ComboIndex), TargetingSystem 연동 |
| **Dodge** | `Input.Dodge` | 회피 몽타주 (Root Motion) | ANS_Invincible 무적 프레임 |
| **Guard** | `Input.Guard` | 홀드 입력 → 가드 몽타주 | ANS_Guard 50% 감소, 다른 Ability 전체 차단 |
| **HitReact** | GameplayEvent | 가드/비가드 몽타주 분기 | bRetriggerInstancedAbility, CancelAbilitiesWithTag |
| **Jump** | `Input.Jump` | Character::Jump/StopJumping | CanJump 체크, 몽타주 없는 즉시 실행 |

모든 어빌리티 공통: `InstancedPerActor`, `LocalPredicted`, `ActivationBlockedTags: State.Dead`

#### 데미지 파이프라인

```
무기 오버랩 → DamageEffect 적용 → DamageExecCalc 실행
│
├── 무적(ANS.Invincible) → 데미지 무시, 즉시 리턴
├── 공식: ATK × (190 / (190 + DEF))
├── 가드(ANS.Guard) → × 0.5
├── → IncomingDamage 메타 어트리뷰트에 출력
├── → AI 데미지 감지 리포트
└── → Event.HitReact 발송 → HitReact 어빌리티 트리거
         │
         └── AttributeSet::PostGameplayEffectExecute
             ├── IncomingDamage → HP 차감
             └── HP ≤ 0 → State.Dead 태그 부여
```

#### 콤보 시스템

```
ASC::OnGiveAbility → bIsComboAbility인 어빌리티를 ComboHandleMap에 캐싱
                     Key: ActivationInputTag, Index: ComboIndex-1

입력 → AbilityInputTagPressed
       ├── ComboHandleMap에 키 존재 → RequestComboAttack()
       │   ├── 콤보 윈도우 열림 + 같은 입력 → ComboIndex++
       │   ├── 윈도우 닫힘 + 이전 콤보 비활성 → ComboIndex = 1
       │   └── 최대 단계 초과 → 리셋
       └── 없으면 → 일반 어빌리티 활성화
```

#### 무기 시스템

| 클래스 | 구조 | 충돌 | 데미지 적용 |
|--------|------|------|-----------|
| `AWxWeaponBase` | GripPoint(루트) + Mesh + HitCollision(Capsule) | ANS.WeaponCollision 태그로 on/off | Overlap → SourceASC에서 DamageEffect 적용 |
| `AWxProjectileBase` | Mesh(루트) + HitCollision(Sphere) + ProjectileMovement | 항시 QueryOnly | Overlap → 주입된 SpecHandle 적용 후 Destroy |

무기: 스윙당 동일 액터 1회 피격 (`HitActorsThisSwing`), 투사체: 중력 없는 직선이 기본값

#### AnimNotifyState 4종

모두 동일 패턴: `NotifyBegin`에서 ASC에 태그 추가, `NotifyEnd`에서 제거.

| ANS | 태그 | 소비자 |
|-----|------|--------|
| ComboWindow | `ANS.ComboWindow` | ASC::IsComboWindowOpen() |
| WeaponCollision | `ANS.WeaponCollision` | WeaponBase (콜리전 토글) |
| Invincible | `ANS.Invincible` | DamageExecCalc, HitReact |
| Guard | `ANS.Guard` | DamageExecCalc, HitReact |

---

### 2.4 WxUI — UI 시스템 (CommonUI + MVVM)

#### 레이어 아키텍처

```
UWxUIManagerSubsystem (GameInstance 서브시스템)
└── UWxPrimaryGameLayout (4개 ActivatableWidgetStack)
    ├── GameLayer      ← UI.Layer.Game    (HUD)
    ├── GameMenuLayer   ← UI.Layer.GameMenu (인게임 메뉴)
    ├── MenuLayer       ← UI.Layer.Menu     (메인 메뉴)
    └── ModalLayer      ← UI.Layer.Modal    (모달)
```

- `UWxUIDeveloperSettings`에서 LayoutClass 소프트 레퍼런스 관리
- `UWxAsyncAction_PushWidgetToLayer`로 비동기 위젯 로딩/푸시 지원
- `UWxActivatableWidget`에서 Game/Menu 입력 모드 설정

#### MVVM

```
UWxViewModel (추상 베이스, UMVVMViewModelBase 상속)
└── UWxViewModel_Health
    ├── CurrentHP, MaxHP, HealthPercent (FieldNotify)
    ├── InitializeWithASC() → ASC 어트리뷰트 변경 델리게이트 바인딩
    └── 어트리뷰트 변경 → SetProperty → UI 자동 갱신
```

---

## 3. 핵심 설계 패턴

### 3.1 Gameplay Tag 기반 이벤트 드리븐

시스템 간 직접 함수 호출 없이 태그의 추가/제거로 통신한다.

```
몽타주 ANS → 태그 부여 → 무기가 태그 변화 감지 → 콜리전 활성화
몽타주 ANS → 태그 부여 → DamageExecCalc가 태그 체크 → 데미지 감소
```

### 3.2 플레이어/AI 통합 어빌리티 시스템

플레이어는 EnhancedInput → InputTag → ASC, AI는 BT → BTTask → ASC. 동일한 어빌리티 클래스를 공유하여 코드 중복이 없다.

### 3.3 Meta Attribute 패턴

`IncomingDamage`를 메타 어트리뷰트로 사용하여 데미지 계산(ExecCalc)과 HP 차감(AttributeSet)을 분리한다. ExecCalc은 최종 데미지 값을 계산하고, AttributeSet의 `PostGameplayEffectExecute`에서 실제 HP를 차감한다.

### 3.4 DataAsset 기반 캐릭터 구성

`UWxAbilitySet` DataAsset으로 캐릭터별 어빌리티/이펙트를 번들링하여, 코드 수정 없이 BP에서 캐릭터 전투 능력을 구성할 수 있다.

### 3.5 네트워크 최적화

| 대상 | Replication Mode | 이유 |
|------|-----------------|------|
| Player | Mixed | 로컬 예측 + 서버 권한 |
| Enemy | Minimal | 다수 적 동시 존재 시 대역폭 절감 |

---

## 4. 전체 데이터 플로우

```
[입력/AI] → ASC → 어빌리티 활성화
                    │
                    ├── 몽타주 재생 → ANS 태그 부여/제거
                    │                  ├── WeaponCollision → 무기 히트 on/off
                    │                  ├── ComboWindow → 콤보 입력 허용
                    │                  ├── Invincible → 데미지 무시
                    │                  └── Guard → 데미지 50% 감소
                    │
                    └── 무기 오버랩 → DamageEffect
                                      └── DamageExecCalc
                                          ├── ATK×(190/(190+DEF))
                                          ├── 무적/가드 보정
                                          ├── IncomingDamage 출력
                                          ├── AI 감지 리포트
                                          └── HitReact 이벤트
                                              │
                                              └── AttributeSet
                                                  ├── HP 차감
                                                  ├── HP ≤ 0 → State.Dead
                                                  │            └── 래그돌 + 사망 처리
                                                  └── ViewModel → UI 갱신
```

---

## 5. 미구현/확장 포인트

| 항목 | 현재 상태 | 비고 |
|------|----------|------|
| 원거리 어빌리티 | `AWxProjectileBase` 구현 완료 | 이를 사용하는 어빌리티 클래스 미작성 |
| MP 소모 | 어트리뷰트 정의됨 | Cost GE 미설정 |
| Ability ViewModel | CLAUDE.md에 명시 | 미구현 |
| Effect ViewModel | CLAUDE.md에 명시 | 미구현 |
| Confirmation Widget | CLAUDE.md에 명시 | 미구현 |
| Button / Action Button | CLAUDE.md에 명시 | 미구현 |

# 어빌리티 제작 가이드 (기획자용)

이 문서는 Blueprint에서 어빌리티를 만들고 설정하는 방법을 설명합니다.

---

## 목차

1. [기본 개념](#1-기본-개념)
2. [어빌리티 기본 설정](#2-어빌리티-기본-설정)
3. [어빌리티 종류별 제작 방법](#3-어빌리티-종류별-제작-방법)
4. [AnimNotifyState 종류와 사용법](#4-animnotifystate-종류와-사용법)
5. [AnimNotify 종류와 사용법](#5-animnotify-종류와-사용법)
6. [AbilitySet으로 캐릭터에 어빌리티 부여하기](#6-abilityset으로-캐릭터에-어빌리티-부여하기)
7. [예제: 처음부터 끝까지](#7-예제-처음부터-끝까지)

---

## 1. 기본 개념

### 어빌리티란?

어빌리티는 캐릭터가 수행할 수 있는 **행동 단위**입니다. 공격, 스킬, 회피, 가드, 점프 등이 모두 어빌리티입니다.

### 핵심 구성 요소

| 구성 요소 | 역할 |
|-----------|------|
| **WxAbility** | 어빌리티의 로직 (언제 발동하고, 무엇을 하는가) |
| **AnimMontage** | 어빌리티가 재생하는 애니메이션 |
| **AnimNotifyState** | 몽타주 구간에 배치하여 특정 상태를 활성화 (예: 무기 충돌 켜기) |
| **AnimNotify** | 몽타주의 특정 시점에 이벤트 발생 (예: 투사체 발사) |
| **GameplayEffect** | 데미지, 버프/디버프 등의 수치 변경 |
| **AbilitySet** | 캐릭터에 부여할 어빌리티/이펙트 목록을 정의하는 데이터 에셋 |

### 작업 흐름 요약

```
1. AnimMontage 준비 (애니메이션)
2. AnimMontage에 NotifyState/Notify 배치 (타이밍)
3. WxAbility BP 생성 및 설정 (로직)
4. AbilitySet에 등록 (캐릭터에 부여)
```

---

## 2. 어빌리티 기본 설정

모든 어빌리티(`WxAbility`)는 다음 공통 속성을 가집니다.

### 공통 속성

| 속성 | 타입 | 설명 |
|------|------|------|
| **ActivationPolicy** | Enum | 어빌리티 발동 방식 |
| **ActivationInputTag** | GameplayTag | 이 어빌리티를 발동하는 입력 태그 |
| **AbilityIcon** | Texture2D | UI에 표시될 아이콘 |
| **CooldownDuration** | float | 쿨다운 시간 (초). 0이면 쿨다운 없음 |
| **CooldownTag** | GameplayTag | 쿨다운 중임을 나타내는 태그 |

### ActivationPolicy (발동 방식)

| 값 | 동작 |
|----|------|
| **OnInputTriggered** | 입력 버튼을 누르면 발동 |
| **OnInputStarted** | 입력이 시작되면 발동 |
| **OnGiven** | 어빌리티가 부여되면 자동 발동 |

### ActivationInputTag 설정

입력 태그는 캐릭터의 입력 매핑과 연결됩니다. 현재 사용 가능한 입력 태그:

| 입력 태그 | 대응 입력 |
|-----------|-----------|
| `Input.Attack` | 공격 버튼 |
| `Input.Skill` | 스킬 버튼 |
| `Input.Dodge` | 회피 버튼 |
| `Input.Guard` | 가드 버튼 |
| `Input.Jump` | 점프 버튼 |
| `Input.LockOn` | 락온 버튼 |

---

## 3. 어빌리티 종류별 제작 방법

### 3.1 공격 어빌리티 (WxAbility_Attack)

콤보 공격을 구현합니다. 입력을 연속으로 누르면 다음 콤보로 이어집니다.

#### BP 설정 속성

| 속성 | 타입 | 설명 |
|------|------|------|
| **ComboMontages** | AnimMontage 배열 | 콤보 순서대로 재생할 몽타주 목록 |

#### 제작 순서

1. **BP 생성**: `WxAbility_Attack`을 부모로 하는 Blueprint 생성
2. **몽타주 준비**: 콤보 단계별 AnimMontage 준비 (예: 3단 콤보면 3개)
3. **ComboMontages 설정**: 배열에 콤보 순서대로 몽타주 추가
4. **ActivationInputTag**: `Input.Attack` 설정
5. **ActivationPolicy**: `OnInputTriggered` 설정

#### 몽타주에 배치할 Notify

각 콤보 몽타주에 다음 NotifyState를 배치해야 합니다:

```
몽타주 타임라인:
|---[RotateToTarget]---|
        |--[WeaponCollision]--|
                    |------[ComboWindow]------|
|                                              |
시작                                          끝
```

- **AN_WeaponCollision**: 무기가 실제로 데미지를 주는 구간
- **AN_ComboWindow**: 이 구간에서 공격 입력 시 다음 콤보로 연결
- **AN_RotateToTarget**: 공격 방향으로 캐릭터 회전

#### 예시: 3단 콤보 공격

```
BP_Ability_SwordCombo (Parent: WxAbility_Attack)
├── ActivationPolicy: OnInputTriggered
├── ActivationInputTag: Input.Attack
├── AbilityIcon: T_Icon_SwordAttack
├── CooldownDuration: 0 (쿨다운 없음)
└── ComboMontages:
    ├── [0] AM_Sword_Slash1   ← 좌측 베기
    ├── [1] AM_Sword_Slash2   ← 우측 베기
    └── [2] AM_Sword_Slash3   ← 내려치기
```

각 몽타주 내부:
```
AM_Sword_Slash1:
├── AN_RotateToTarget      (0.0s ~ 0.3s)  ← 적 방향으로 회전
├── AN_WeaponCollision     (0.15s ~ 0.4s) ← 데미지 판정 구간
└── AN_ComboWindow         (0.3s ~ 0.8s)  ← 다음 콤보 입력 대기
```

---

### 3.2 스킬 어빌리티 (WxAbility_Skill)

단일 몽타주를 재생하는 스킬입니다. 근접/원거리 모두 가능합니다.

#### BP 설정 속성

| 속성 | 타입 | 설명 |
|------|------|------|
| **SkillMontage** | AnimMontage | 스킬 재생 시 사용할 몽타주 |

#### 제작 순서

1. **BP 생성**: `WxAbility_Skill`을 부모로 하는 Blueprint 생성
2. **몽타주 준비**: 스킬용 AnimMontage 준비
3. **SkillMontage 설정**: 사용할 몽타주 지정
4. **ActivationInputTag**: `Input.Skill` 설정
5. **쿨다운 설정**: `CooldownDuration`과 `CooldownTag` 설정

#### 근접 스킬 몽타주 구성

```
AM_Skill_PowerSlash:
├── AN_RotateToTarget      (0.0s ~ 0.2s)
├── AN_Invincible          (0.1s ~ 0.3s)  ← 선딜 무적
└── AN_WeaponCollision     (0.3s ~ 0.6s)  ← 데미지 판정
```

#### 원거리 스킬 몽타주 구성

```
AM_Skill_Fireball:
├── AN_RotateToTarget      (0.0s ~ 0.3s)
└── ANS_SpawnProjectile    (0.4s 시점)     ← 투사체 발사 (AnimNotify)
```

#### 예시: 파이어볼 스킬

```
BP_Ability_Fireball (Parent: WxAbility_Skill)
├── ActivationPolicy: OnInputTriggered
├── ActivationInputTag: Input.Skill
├── AbilityIcon: T_Icon_Fireball
├── CooldownDuration: 5.0
├── CooldownTag: Ability.Skill.Fireball.Cooldown
└── SkillMontage: AM_Cast_Fireball
```

몽타주 내부의 SpawnProjectile Notify 설정:
```
AN_SpawnProjectile:
├── ProjectileClass: BP_Projectile_Fireball
└── SpawnSocketName: Muzzle_R (오른손 소켓)
```

---

### 3.3 회피 어빌리티 (WxAbility_Dodge)

회피 동작을 수행합니다. 무적 구간을 포함할 수 있습니다.

#### BP 설정 속성

| 속성 | 타입 | 설명 |
|------|------|------|
| **DodgeMontage** | AnimMontage | 회피 시 재생할 몽타주 |

#### 제작 순서

1. **BP 생성**: `WxAbility_Dodge`를 부모로 하는 Blueprint 생성
2. **DodgeMontage 설정**: 회피 몽타주 지정
3. **ActivationInputTag**: `Input.Dodge` 설정

#### 몽타주 구성

```
AM_Dodge_Roll:
└── AN_Invincible          (0.05s ~ 0.4s)  ← 무적 구간
```

#### 예시

```
BP_Ability_DodgeRoll (Parent: WxAbility_Dodge)
├── ActivationPolicy: OnInputTriggered
├── ActivationInputTag: Input.Dodge
├── CooldownDuration: 0.5
├── CooldownTag: Ability.Dodge.Cooldown
└── DodgeMontage: AM_Dodge_Roll
```

---

### 3.4 가드 어빌리티 (WxAbility_Guard)

방어 자세를 취합니다. 가드 중에는 데미지가 감소합니다.

#### BP 설정 속성

| 속성 | 타입 | 설명 |
|------|------|------|
| **GuardMontage** | AnimMontage | 가드 시 재생할 몽타주 |

#### 제작 순서

1. **BP 생성**: `WxAbility_Guard`를 부모로 하는 Blueprint 생성
2. **GuardMontage 설정**: 가드 몽타주 지정
3. **ActivationInputTag**: `Input.Guard` 설정

#### 몽타주 구성

```
AM_Guard:
└── AN_Guard               (0.0s ~ 끝)     ← 가드 상태 활성화
```

> **참고**: Guard NotifyState가 활성화된 동안 `State.Guard` 태그가 부여됩니다. 이 태그가 있으면 피격 시 가드 전용 HitReact가 재생됩니다.

#### 예시

```
BP_Ability_ShieldGuard (Parent: WxAbility_Guard)
├── ActivationPolicy: OnInputStarted
├── ActivationInputTag: Input.Guard
└── GuardMontage: AM_Guard_Shield
```

---

### 3.5 피격 리액션 (WxAbility_HitReact)

피격 시 자동으로 발동되는 어빌리티입니다. 기획자가 직접 입력을 할당하지 않습니다.

#### BP 설정 속성

| 속성 | 타입 | 설명 |
|------|------|------|
| **HitReactMontage** | AnimMontage | 일반 피격 시 재생할 몽타주 |
| **GuardHitReactMontage** | AnimMontage | 가드 중 피격 시 재생할 몽타주 |

#### 제작 순서

1. **BP 생성**: `WxAbility_HitReact`를 부모로 하는 Blueprint 생성
2. **HitReactMontage**: 일반 피격 몽타주 설정
3. **GuardHitReactMontage**: 가드 피격 몽타주 설정
4. **ActivationPolicy**: `OnGiven` (자동 발동이므로 입력 태그 불필요)

#### 예시

```
BP_Ability_HitReact (Parent: WxAbility_HitReact)
├── ActivationPolicy: OnGiven
├── HitReactMontage: AM_HitReact_Normal
└── GuardHitReactMontage: AM_HitReact_Guard
```

---

### 3.6 점프 어빌리티 (WxAbility_Jump)

캐릭터 점프입니다. 몽타주 없이 캐릭터의 기본 점프 기능을 사용합니다.

#### 제작 순서

1. **BP 생성**: `WxAbility_Jump`을 부모로 하는 Blueprint 생성
2. **ActivationInputTag**: `Input.Jump` 설정
3. **별도 몽타주 설정 불필요**

#### 예시

```
BP_Ability_Jump (Parent: WxAbility_Jump)
├── ActivationPolicy: OnInputStarted
└── ActivationInputTag: Input.Jump
```

---

### 3.7 락온 어빌리티 (WxAbility_LockOn)

적에게 카메라를 고정합니다.

#### BP 설정 속성

| 속성 | 타입 | 설명 |
|------|------|------|
| **TargetingPreset** | UTargetingPreset | 타겟 검색 설정 (범위, 필터 등) |
| **CameraInterpSpeed** | float | 카메라 전환 보간 속도 |
| **MaxPitchOffset** | float | 카메라 피치 최대 오프셋 |
| **MaxDistance** | float | 락온 유지 최대 거리 |
| **ReticleWidgetClass** | TSubclassOf | 락온 레티클 UI 위젯 클래스 |

#### 예시

```
BP_Ability_LockOn (Parent: WxAbility_LockOn)
├── ActivationPolicy: OnInputTriggered
├── ActivationInputTag: Input.LockOn
├── TargetingPreset: TP_LockOn_Default
├── CameraInterpSpeed: 5.0
├── MaxPitchOffset: 30.0
├── MaxDistance: 2000.0
└── ReticleWidgetClass: WBP_LockOnReticle
```

---

## 4. AnimNotifyState 종류와 사용법

AnimNotifyState는 몽타주의 **구간**에 배치하여 해당 구간 동안 특정 상태를 활성화합니다.

### 4.1 WeaponCollision (무기 충돌)

무기의 데미지 판정을 활성화합니다.

- **클래스**: `WxAnimNotifyState_GameplayTag`
- **태그**: `ANS.WeaponCollision`
- **용도**: 이 구간 동안 무기의 히트 콜리전이 활성화되어 적에게 데미지를 줌
- **배치 위치**: 무기가 실제로 스윙하는 애니메이션 프레임에 맞춤

```
|--------|==================|---------|
         ↑ 콜리전 시작       ↑ 콜리전 끝
         (스윙 시작)         (스윙 끝)
```

> **주의**: 구간이 너무 넓으면 부자연스러운 히트 판정이 발생합니다. 실제 무기가 움직이는 프레임에 정확히 맞추세요.

### 4.2 ComboWindow (콤보 입력 대기)

콤보 입력을 받을 수 있는 구간입니다.

- **클래스**: `WxAnimNotifyState_GameplayTag`
- **태그**: `ANS.ComboWindow`
- **용도**: 이 구간에서 공격 입력이 들어오면 다음 콤보 몽타주로 이어짐
- **배치 위치**: WeaponCollision 종료 부근부터 몽타주 끝까지

```
     |==WeaponCollision==|
                    |=========ComboWindow=========|
                    ↑ 여기서부터 다음 콤보 입력 가능
```

> **팁**: ComboWindow 시작을 WeaponCollision과 약간 겹치면 공격 중에도 콤보 입력이 가능해져 반응성이 좋아집니다.

### 4.3 Invincible (무적)

캐릭터에 무적 상태를 부여합니다.

- **클래스**: `WxAnimNotifyState_GameplayTag`
- **태그**: `ANS.Invincible`
- **용도**: 이 구간 동안 캐릭터가 데미지를 받지 않음
- **배치 위치**: 회피의 핵심 이동 구간, 또는 스킬 선딜 무적 구간

```
|==Invincible==|--------------------------|
↑ 회피 시작     ↑ 무적 종료 (이후 취약)
```

### 4.4 Guard (가드)

가드 상태를 활성화합니다.

- **클래스**: `WxAnimNotifyState_GameplayTag`
- **태그**: `ANS.Guard`
- **용도**: 이 구간 동안 피격 시 가드 전용 HitReact가 발동됨
- **배치 위치**: 가드 몽타주의 방어 자세 유지 구간 전체

### 4.5 RotateToTarget (타겟 방향 회전)

캐릭터를 타겟 방향으로 회전시킵니다.

- **클래스**: `WxAnimNotifyState_RotateToTarget`
- **설정 속성**:
  - **TargetingPreset**: 회전할 타겟을 찾는 설정
- **용도**: 공격/스킬 시 적 방향으로 자동 회전
- **배치 위치**: 몽타주 시작부터 실제 공격 모션 직전까지

```
|==RotateToTarget==|--WeaponCollision--|---------|
↑ 적 방향 회전      ↑ 회전 종료, 공격 시작
```

> **팁**: RotateToTarget은 WeaponCollision 시작 전에 끝나도록 배치하면 자연스럽습니다. 공격 중에도 회전하면 부자연스러워 보일 수 있습니다.

---

## 5. AnimNotify 종류와 사용법

AnimNotify는 몽타주의 **특정 시점**에 한 번 발생하는 이벤트입니다.

### 5.1 SpawnProjectile (투사체 발사)

투사체를 생성합니다.

- **클래스**: `WxAnimNotify_SpawnProjectile`
- **설정 속성**:
  - **ProjectileClass**: 생성할 투사체 블루프린트 클래스
  - **SpawnSocketName**: 투사체가 생성될 캐릭터 메시의 소켓 이름
- **용도**: 원거리 스킬에서 투사체를 발사

```
|---------|------X---------|
                 ↑ 이 시점에 투사체 생성
                 (캐스팅 모션이 끝나는 프레임)
```

#### SpawnProjectile 설정 예시

| 속성 | 예시 값 | 설명 |
|------|---------|------|
| ProjectileClass | BP_Projectile_Fireball | 발사할 투사체 BP |
| SpawnSocketName | Muzzle_R | 오른손 소켓에서 발사 |

> **참고**: 투사체 BP(`WxProjectileBase` 기반)에 `DamageEffectClass`를 설정해야 데미지가 적용됩니다.

---

## 6. AbilitySet으로 캐릭터에 어빌리티 부여하기

### AbilitySet이란?

`WxAbilitySet`은 캐릭터에 부여할 어빌리티와 이펙트를 묶어놓은 **데이터 에셋**입니다.

### 생성 방법

1. **콘텐츠 브라우저** → 우클릭 → **Miscellaneous** → **Data Asset**
2. 클래스 선택: `WxAbilitySet`
3. 이름 예시: `DA_AbilitySet_Player`, `DA_AbilitySet_Enemy_Goblin`

### 설정 속성

| 속성 | 타입 | 설명 |
|------|------|------|
| **GrantedAbilities** | 배열 | 부여할 어빌리티 목록 (어빌리티 클래스 + 레벨) |
| **GrantedEffects** | 배열 | 부여할 이펙트 목록 (이펙트 클래스 + 레벨) |

### 예시: 플레이어 AbilitySet

```
DA_AbilitySet_Player:
├── GrantedAbilities:
│   ├── BP_Ability_SwordCombo      (Level 1)  ← 기본 공격
│   ├── BP_Ability_Fireball        (Level 1)  ← 스킬
│   ├── BP_Ability_DodgeRoll       (Level 1)  ← 회피
│   ├── BP_Ability_ShieldGuard     (Level 1)  ← 가드
│   ├── BP_Ability_Jump            (Level 1)  ← 점프
│   ├── BP_Ability_LockOn          (Level 1)  ← 락온
│   └── BP_Ability_HitReact        (Level 1)  ← 피격 리액션
└── GrantedEffects:
    └── GE_InitStats               (Level 1)  ← 초기 스탯 설정
```

### 예시: 적 캐릭터 AbilitySet

```
DA_AbilitySet_Enemy_Goblin:
├── GrantedAbilities:
│   ├── BP_Ability_GoblinAttack    (Level 1)
│   └── BP_Ability_HitReact_Goblin (Level 1)
└── GrantedEffects:
    └── GE_InitStats_Goblin        (Level 1)
```

---

## 7. 예제: 처음부터 끝까지

### 예제 1: 대검 3단 콤보 만들기

**목표**: 대검으로 3번 연속 공격하는 콤보

#### Step 1: AnimMontage 준비

3개의 AnimMontage를 만듭니다:
- `AM_Greatsword_Combo1` - 횡베기
- `AM_Greatsword_Combo2` - 올려베기
- `AM_Greatsword_Combo3` - 내려찍기

#### Step 2: 각 몽타주에 Notify 배치

**AM_Greatsword_Combo1** (총 1.0초):
| Notify | 시작 | 끝 | 설명 |
|--------|------|-----|------|
| RotateToTarget | 0.0s | 0.2s | 적 방향 회전 |
| WeaponCollision | 0.2s | 0.5s | 데미지 판정 |
| ComboWindow | 0.4s | 0.9s | 다음 콤보 입력 대기 |

**AM_Greatsword_Combo2** (총 1.0초):
| Notify | 시작 | 끝 | 설명 |
|--------|------|-----|------|
| RotateToTarget | 0.0s | 0.15s | 적 방향 회전 |
| WeaponCollision | 0.15s | 0.45s | 데미지 판정 |
| ComboWindow | 0.35s | 0.85s | 다음 콤보 입력 대기 |

**AM_Greatsword_Combo3** (총 1.2초):
| Notify | 시작 | 끝 | 설명 |
|--------|------|-----|------|
| RotateToTarget | 0.0s | 0.3s | 적 방향 회전 |
| WeaponCollision | 0.3s | 0.7s | 데미지 판정 |
| *(ComboWindow 없음 - 마지막 콤보)* | | | |

#### Step 3: 어빌리티 BP 생성

1. 콘텐츠 브라우저 → 우클릭 → Blueprint Class
2. Parent Class: `WxAbility_Attack`
3. 이름: `BP_Ability_GreatswordCombo`
4. 설정:
   - ActivationPolicy: `OnInputTriggered`
   - ActivationInputTag: `Input.Attack`
   - AbilityIcon: `T_Icon_Greatsword`
   - ComboMontages:
     - [0] `AM_Greatsword_Combo1`
     - [1] `AM_Greatsword_Combo2`
     - [2] `AM_Greatsword_Combo3`

#### Step 4: AbilitySet에 등록

`DA_AbilitySet_Player`의 GrantedAbilities에 `BP_Ability_GreatswordCombo` 추가

---

### 예제 2: 아이스볼트 원거리 스킬 만들기

**목표**: 얼음 투사체를 발사하는 스킬 (쿨다운 8초)

#### Step 1: 투사체 BP 준비

1. `WxProjectileBase`를 부모로 BP 생성: `BP_Projectile_IceBolt`
2. 설정:
   - Mesh/Particle: 얼음 이펙트
   - DamageEffectClass: `GE_Damage_IceBolt` (별도 생성 필요)
   - Speed, Lifetime 등 설정

#### Step 2: AnimMontage 준비

`AM_Cast_IceBolt` 생성 (총 1.2초):
| Notify | 시점/구간 | 설명 |
|--------|-----------|------|
| RotateToTarget | 0.0s ~ 0.3s | 적 방향 회전 |
| SpawnProjectile | 0.5s (시점) | 투사체 발사 |

SpawnProjectile 설정:
- ProjectileClass: `BP_Projectile_IceBolt`
- SpawnSocketName: `Muzzle_R`

#### Step 3: 어빌리티 BP 생성

1. Parent Class: `WxAbility_Skill`
2. 이름: `BP_Ability_IceBolt`
3. 설정:
   - ActivationPolicy: `OnInputTriggered`
   - ActivationInputTag: `Input.Skill`
   - AbilityIcon: `T_Icon_IceBolt`
   - CooldownDuration: `8.0`
   - CooldownTag: `Ability.Skill.IceBolt.Cooldown`
   - SkillMontage: `AM_Cast_IceBolt`

#### Step 4: AbilitySet에 등록

---

### 예제 3: 적(고블린) 기본 공격 만들기

**목표**: 고블린이 사용하는 단순 1단 공격

#### Step 1: AnimMontage 준비

`AM_Goblin_Attack` 생성 (총 0.8초):
| Notify | 시작 | 끝 | 설명 |
|--------|------|-----|------|
| RotateToTarget | 0.0s | 0.15s | 플레이어 방향 회전 |
| WeaponCollision | 0.2s | 0.45s | 데미지 판정 |

#### Step 2: 어빌리티 BP 생성

1. Parent Class: `WxAbility_Attack`
2. 이름: `BP_Ability_GoblinAttack`
3. 설정:
   - ActivationPolicy: `OnGiven` (BT에서 활성화)
   - ComboMontages: [0] `AM_Goblin_Attack` (1단만)

#### Step 3: 적 전용 AbilitySet 생성

```
DA_AbilitySet_Goblin:
├── GrantedAbilities:
│   ├── BP_Ability_GoblinAttack    (Level 1)
│   └── BP_Ability_HitReact        (Level 1)
└── GrantedEffects:
    └── GE_InitStats_Goblin        (Level 1)
```

---

### 예제 4: 무적 회피 + 반격 스킬

**목표**: 회피 후 무적 상태에서 반격하는 스킬

#### Step 1: AnimMontage 준비

`AM_Skill_CounterDash` 생성 (총 1.5초):
| Notify | 시작 | 끝 | 설명 |
|--------|------|-----|------|
| Invincible | 0.0s | 0.5s | 돌진 중 무적 |
| RotateToTarget | 0.0s | 0.4s | 적 방향 돌진 |
| WeaponCollision | 0.5s | 0.8s | 반격 데미지 판정 |

#### Step 2: 어빌리티 BP 생성

1. Parent Class: `WxAbility_Skill`
2. 이름: `BP_Ability_CounterDash`
3. 설정:
   - ActivationPolicy: `OnInputTriggered`
   - ActivationInputTag: `Input.Skill`
   - CooldownDuration: `12.0`
   - CooldownTag: `Ability.Skill.CounterDash.Cooldown`
   - SkillMontage: `AM_Skill_CounterDash`

---

## 부록: 자주 묻는 질문

### Q: 무기의 DamageEffectClass는 어디서 설정하나요?

무기 BP(`WxWeaponBase` 기반)의 `DamageEffectClass` 속성에 GameplayEffect를 설정합니다. 이 이펙트가 무기 충돌 시 적용됩니다.

### Q: 새로운 Gameplay Tag가 필요하면 어떻게 하나요?

Gameplay Tag는 C++ 코드에서 선언해야 합니다. 프로그래머에게 요청하세요.

### Q: 콤보 공격의 속도를 조절하려면?

AnimMontage의 PlayRate를 조절하거나, 몽타주 에디터에서 애니메이션 속도를 변경합니다. Notify 구간도 함께 조정해야 합니다.

### Q: 하나의 캐릭터에 여러 공격 어빌리티를 넣을 수 있나요?

가능하지만, 같은 ActivationInputTag를 사용하면 충돌합니다. 서로 다른 입력 태그를 사용하거나, 무기 교체 시스템과 연동해야 합니다.

### Q: 적 캐릭터의 어빌리티는 어떻게 발동시키나요?

적 캐릭터는 Behavior Tree에서 어빌리티를 발동합니다. ActivationPolicy를 `OnGiven`으로 설정하거나, BT Task에서 ASC를 통해 직접 발동할 수 있습니다.

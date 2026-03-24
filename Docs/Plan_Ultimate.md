# WxAbility_Ultimate (궁극기) 설계 문서

## 지시 사항

WxAbility_Ultimate(궁극기) 개발할건데 설계 문서를 작성해줘.

1. 궁극기는 MP 100을 cost로 사용한다.
2. 발동 시 컷신을 재생하고, 컷신 재생 동안에는 컷신 외에는 모두 시간 정지되어야한다.
3. 컷신 재생이 끝나면 캐릭터가 몽타주를 재생한다.

---

## 개요

MP 100을 소모하여 발동하는 궁극기 어빌리티. 발동 시 컷신(Level Sequence)을 재생하며, 컷신 동안 게임 월드는 시간 정지된다. 컷신 종료 후 캐릭터가 공격 몽타주를 재생한다.

---

## 동작 사양

| 항목 | 값 |
|------|------|
| MP 소모 | 100 |
| 발동 조건 | MP ≥ 100, 사망/그로기 상태가 아닐 것 |
| 발동 시 | 진행 중인 다른 어빌리티 전부 취소 |
| 컷신 중 | 게임 월드 시간 정지 (Global Time Dilation 0.001), 캐릭터 무적 |
| 컷신 종료 후 | 공격 몽타주 재생, WeaponCollision 데미지 판정 |

---

## 실행 흐름

```
입력 (Input_Ultimate)
    │
    ▼
ActivateAbility
    │  CheckCost (MP ≥ 100)
    │  CommitAbility (MP 100 차감)
    │  다른 어빌리티 취소 (CancelAbilitiesWithTag: Ability)
    │
    ▼
컷신 재생 (AbilityTask)
    │  Global Time Dilation → 0.001
    │  Level Sequence 재생 (보정된 PlayRate)
    │  캐릭터 무적 (ANS_Invincible)
    │
    ▼
컷신 종료
    │  Global Time Dilation → 1.0 복원
    │
    ▼
몽타주 재생 (AbilityTask_PlayMontageAndWait)
    │  공격 애니메이션
    │  WeaponCollision / 데미지 판정
    │
    ▼
몽타주 종료 → EndAbility
```

---

## 클래스 설계

### 1. UWxAbility_Ultimate (WxCombat)

```
UWxAbility
  └─ UWxAbility_Ultimate
```

기존 어빌리티 패턴을 따르되, 컷신 → 몽타주의 2단계 실행 구조를 갖는다.

#### 프로퍼티

| 프로퍼티 | 타입 | 설명 |
|----------|------|------|
| `CutsceneSequence` | `TSoftObjectPtr<ULevelSequence>` | 컷신 Level Sequence 에셋 |
| `UltimateMontage` | `TObjectPtr<UAnimMontage>` | 컷신 후 재생할 공격 몽타주 |

#### 베이스 클래스 설정 (생성자)

| 설정 | 값 | 비고 |
|------|------|------|
| `MPCost` | `100.f` | 기존 Cost 시스템 활용 |
| `ActivationInputTag` | `Input_Ultimate` | 신규 입력 태그 |
| `CooldownDuration` | 에셋별 설정 | 긴 쿨다운 |
| `CooldownTag` | `Cooldown_Ultimate` | 신규 쿨다운 태그 |
| `AbilityTags` | `Ability_Ultimate` | 신규 어빌리티 태그 |
| `CancelAbilitiesWithTag` | `Ability` | 발동 시 모든 어빌리티 취소 |
| `ActivationBlockedTags` | `State_Dead, State_Groggy` | 사망/그로기 중 발동 불가 |

#### 실행 로직

```
ActivateAbility()
├─ CommitAbility() 실패 시 → EndAbility(true)
├─ 캐릭터에 무적 태그 부여 (ANS_Invincible)
├─ UWxAbilityTask_PlayCutscene 생성 및 활성화
│   ├─ OnCompleted → HandleCutsceneCompleted()
│   └─ OnCancelled → HandleCutsceneCancelled()
│
HandleCutsceneCompleted()
├─ 무적 태그 제거
├─ UAbilityTask_PlayMontageAndWait 생성 및 활성화
│   ├─ OnCompleted → HandleMontageCompleted()
│   ├─ OnBlendOut → HandleMontageBlendOut()
│   ├─ OnInterrupted → HandleMontageInterrupted()
│   └─ OnCancelled → HandleMontageCancelled()
│
HandleMontageCompleted()
└─ EndAbility(false)
```

---

### 2. UWxAbilityTask_PlayCutscene (WxCombat)

```
UAbilityTask
  └─ UWxAbilityTask_PlayCutscene
```

Level Sequence 재생과 시간 정지를 관리하는 커스텀 AbilityTask.

#### 역할

1. `ALevelSequenceActor`를 월드에 스폰
2. Global Time Dilation을 `0.001`로 설정하여 게임 월드 시간 정지
3. Level Sequence Player의 PlayRate를 `1.0 / 0.001 = 1000`으로 보정하여 컷신만 정상 속도 재생
4. 시퀀스 종료 시 Time Dilation을 `1.0`으로 복원
5. 결과를 델리게이트로 브로드캐스트

#### 프로퍼티

| 프로퍼티 | 타입 | 설명 |
|----------|------|------|
| `LevelSequence` | `ULevelSequence*` | 재생할 시퀀스 에셋 |

#### 델리게이트

| 델리게이트 | 발동 조건 |
|------------|-----------|
| `OnCompleted` | 시퀀스 정상 종료 |
| `OnCancelled` | 어빌리티 취소로 인한 중단 |

#### 핵심 구현

```
Activate()
├─ SpawnActorDeferred<ALevelSequenceActor>()
├─ ULevelSequencePlayer 초기화
├─ UGameplayStatics::SetGlobalTimeDilation(0.001)
├─ LevelSequencePlayer.SetPlayRate(1.0 / 0.001)
├─ LevelSequencePlayer.OnFinished에 바인딩
└─ LevelSequencePlayer.Play()

HandleSequenceFinished()
├─ UGameplayStatics::SetGlobalTimeDilation(1.0)
├─ LevelSequenceActor 제거
└─ OnCompleted.Broadcast()

OnDestroy()  // 비정상 종료 안전장치
├─ Time Dilation이 변경된 상태라면 1.0으로 복원
├─ 캐릭터의 무적 태그가 잔류하면 제거
└─ LevelSequenceActor가 존재하면 제거
```

#### Time Dilation 방식 상세

| 항목 | 값 | 근거 |
|------|------|------|
| Global Time Dilation | `0.001` | `0`은 엔진 내부 0 나누기 문제 발생 가능. 최소값 사용 |
| Sequence PlayRate 보정 | `1.0 / GlobalTimeDilation` | 시퀀스만 원래 속도로 재생되도록 역수 적용 |
| 복원 시점 | `OnFinished` + `OnDestroy` | 이중 안전장치로 Time Dilation 미복원 방지 |

---

## 신규 Gameplay Tag

**파일**: `WxGameplayTags.h / .cpp`

```cpp
// ── Ability ──
WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Ultimate);

// ── Cooldown ──
WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Ultimate);

// ── Input ──
WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ultimate);
```

---

## 모듈 의존성

`WxCombat.Build.cs`에 `LevelSequence` 모듈 의존성 추가 필요:

```csharp
PrivateDependencyModuleNames.AddRange(new string[]
{
    // 기존 ...
    "LevelSequence",
    "MovieScene",
});
```

---

## 작업 체크리스트

### 코드 작업
| # | 작업 | 파일 |
|---|------|------|
| 1 | `Ability_Ultimate`, `Cooldown_Ultimate`, `Input_Ultimate` 태그 추가 | `WxGameplayTags.h/.cpp` |
| 2 | `UWxAbilityTask_PlayCutscene` 구현 | 신규 (WxCombat) |
| 3 | `UWxAbility_Ultimate` 구현 | 신규 (WxCombat) |
| 4 | `WxCombat.Build.cs`에 LevelSequence, MovieScene 모듈 추가 | `WxCombat.Build.cs` |

### 에디터 작업
| # | 작업 | 비고 |
|---|------|------|
| 1 | 컷신 Level Sequence 에셋 제작 | 에디터 |
| 2 | 공격 몽타주 에셋 제작 | 에디터 |
| 3 | AbilitySet에 궁극기 등록 및 입력 바인딩 | 에디터 |

---

## 고려 사항

- **멀티플레이어**: Global Time Dilation은 모든 클라이언트에 영향을 준다. 멀티플레이 환경에서는 서버 권한으로 실행하고, 다른 플레이어에게는 컷신 스킵 또는 별도 연출이 필요할 수 있다. 초기 구현은 싱글 플레이어 기준으로 진행하고, 멀티플레이 대응은 후속 작업으로 분리한다.
- **컷신 스킵**: 플레이어가 컷신을 건너뛸 수 있도록 입력 처리를 AbilityTask에 추가하는 것을 고려한다.
- **OnDestroy 안전장치**: 어빌리티가 외부 요인으로 강제 종료될 경우에도 Time Dilation과 무적 태그가 반드시 복원/제거되도록 `OnDestroy`에서 이중 처리한다.
- **PlayRate 상한**: LevelSequencePlayer의 PlayRate를 1000으로 설정하는데, 엔진 내부에 PlayRate 상한이 있을 수 있다. 구현 시 실제 시퀀스가 정상 속도로 재생되는지 검증이 필요하다.

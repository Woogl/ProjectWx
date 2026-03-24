# 퍼펙트 가드 설계 문서

## 개요

가드 몽타주 초반에 배치하는 퍼펙트 가드 구간. 이 구간 동안 피격 시 대미지를 받지 않고, 공격자에게 DP 피해를 반사한다.

---

## 동작 사양

| 조건 | 결과 |
|------|------|
| `ANS_PerfectGuard` + `ANS_Guard` 동시 보유 | 대미지 0, 공격자에게 DP 피해 (PerfectGuard 우선) |
| `ANS_Guard`만 보유 (기존) | 대미지 50% 감소 |
| 태그 없이 피격 | 대미지 100% |

> `ANS_PerfectGuard`와 `ANS_Guard`는 겹칠 수 있다. 두 태그가 동시에 존재하면 `ANS_PerfectGuard`를 우선 적용한다.

### DP 반사 공식

`WxDamageExecCalc`의 대미지 공식을 그대로 사용하되, 결과를 공격자의 DP에 적용한다.

```
ReflectDP = SourceATK × (100 / (100 + TargetDEF))
```

- 치명타 판정 없음 (반사이므로 크리티컬 미적용)
- 공격자의 DP에 `ReflectDP`만큼 가산
- 공격자의 DP가 MaxDP에 도달하면 그로기 상태 부여

---

## 클래스 설계

### UWxAnimNotifyState_PerfectGuard (WxCombat)

```
UAnimNotifyState
  └─ UWxAnimNotifyState_PerfectGuard
```

기존 `Guard`, `Invincible` ANS와 동일한 패턴. 구간 동안 `ANS_PerfectGuard` 태그를 부여/제거한다.

---

## 기존 코드 수정

### WxDamageExecCalc.cpp

`Execute_Implementation` 상단에 퍼펙트 가드 분기를 추가한다. 기존 무적 체크 직후에 배치.

```
Execute_Implementation()
│
├─ 무적 체크 (ANS_Invincible) → return
│
├─ 퍼펙트 가드 체크 (ANS_PerfectGuard)  ← 신규 (ANS_Guard보다 먼저 체크)
│   ├─ SourceATK, TargetDEF 캡처
│   ├─ ReflectDP = SourceATK × (100 / (100 + TargetDEF))
│   ├─ SourceASC의 DP에 ReflectDP 가산 (GameplayEffect)
│   ├─ Source DP ≥ MaxDP → Source에 State_Groggy 부여
│   └─ return (대미지 적용하지 않음, 가드 감소도 미적용)
│
├─ 기존 대미지 계산 (변경 없음)
│   ├─ ... (중간 계산)
│   ├─ 가드 체크 (ANS_Guard) → 50% 감소  ← 퍼펙트 가드가 아닐 때만 도달
│   ...
```

#### DP 반사 적용 방법

ExecCalc의 `OutExecutionOutput`은 **타겟**의 어트리뷰트만 수정할 수 있으므로, 공격자(Source)의 DP를 직접 수정할 수 없다. 따라서 공격자의 ASC에 별도의 Instant GameplayEffect를 적용하여 DP를 가산한다.

기존 `WxDamageExecCalc`의 MP 회복 패턴(`static UGameplayEffect*` + `AddToRoot`)을 따르되, 반사량은 매번 다르므로 `WxEffect_Cost`의 SetByCaller 패턴을 사용한다.

```cpp
// 공격자에게 DP 반사 적용
if (SourceASC)
{
    static UGameplayEffect* DPReflectEffect = nullptr;
    if (!DPReflectEffect)
    {
        DPReflectEffect = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("PerfectGuardDPReflect")));
        DPReflectEffect->AddToRoot();
        DPReflectEffect->DurationPolicy = EGameplayEffectDurationType::Instant;

        FSetByCallerFloat SetByCaller;
        SetByCaller.DataTag = WxGameplayTags::SetByCaller_ReflectDP;

        FGameplayModifierInfo ModInfo;
        ModInfo.Attribute = UWxCombatAttributeSet::GetDPAttribute();
        ModInfo.ModifierOp = EGameplayModOp::Additive;
        ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
        DPReflectEffect->Modifiers.Add(ModInfo);
    }

    FGameplayEffectSpec Spec(DPReflectEffect, SourceASC->MakeEffectContext(), 1.f);
    Spec.SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_ReflectDP, ReflectDP);
    SourceASC->ApplyGameplayEffectSpecToSelf(Spec);

    // 그로기 판정
    // (Source의 현재 DP + ReflectDP ≥ MaxDP → State_Groggy 부여)
}
```

---

## 신규 Gameplay Tag

**파일**: `WxGameplayTags.h / .cpp`

```cpp
// ── ANS ──
/** 퍼펙트 가드 판정 구간. ANS_PerfectGuard가 부여/제거 */
WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ANS_PerfectGuard);

// ── SetByCaller ──
/** DP 반사량 SetByCaller 키 */
WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_ReflectDP);
```

---

## 몽타주 배치 예시

가드 몽타주 타임라인에서 퍼펙트 가드 구간을 앞부분에 배치한다.

```
Guard Montage Timeline
|<-- PerfectGuard -->|
|<--------------------- Guard ----------------------->|
0s                 0.2s                              End
```

- `ANS_Guard`는 몽타주 전체 구간에 배치
- `ANS_PerfectGuard`는 가드 시작 직후 짧은 프레임(약 0.1~0.3초)에 겹쳐서 배치
- 두 태그가 겹치는 구간에서는 ExecCalc에서 `ANS_PerfectGuard`를 우선 체크하므로 퍼펙트 가드가 발동
- 퍼펙트 가드 구간이 지나면 `ANS_Guard`만 남아 일반 가드로 자연 전환

---

## 작업 체크리스트

### 코드 작업
| # | 작업 | 파일 |
|---|------|------|
| 1 | `ANS_PerfectGuard`, `SetByCaller_ReflectDP` 태그 추가 | `WxGameplayTags.h/.cpp` |
| 2 | `UWxAnimNotifyState_PerfectGuard` 구현 | 신규 (WxCombat) |
| 3 | `UWxDamageExecCalc`에 퍼펙트 가드 분기 추가 | `WxDamageExecCalc.cpp` |

### 에디터 작업
| # | 작업 | 비고 |
|---|------|------|
| 1 | 가드 몽타주에 PerfectGuard ANS 구간 배치 | 몽타주 에디터 |

---

## 고려 사항

- **HitReact 이벤트**: 퍼펙트 가드 성공 시 기존 `Event_HitReact` 대신 별도 이벤트(`Event_PerfectGuard`)를 발송하여, 전용 리액션 어빌리티(반격 연출, 이펙트 등)를 트리거할 수 있다. 필요 시 후속 작업으로 추가한다.
- **AI 감지**: 퍼펙트 가드 시에도 `UAISense_Damage::ReportDamageEvent`를 호출할지 결정이 필요하다. 대미지가 0이므로 호출하지 않는 것이 자연스럽다. -> 호출 안해도 되는 것으로 결정함

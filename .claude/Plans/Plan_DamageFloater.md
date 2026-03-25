# 데미지 플로터 설계 문서

## 개요

Gameplay Cue를 활용하여 피격 시 데미지 수치를 화면에 표시하는 데미지 플로터 시스템을 구현한다.

- **트리거**: `UWxDamageExecCalc`에서 데미지 확정 후 Gameplay Cue 실행
- **표시 정보**: 데미지 수치, 치명타 여부
- **연출**: 월드 스페이스 위치에서 위로 떠오르며 페이드 아웃

---

## 동작 사양

| 조건 | 표시 |
|------|------|
| 일반 대미지 | 데미지 수치 (기본 스타일) |
| 치명타 대미지 | 데미지 수치 (강조 스타일: 크기 확대, 색상 변경) |

- 대미지 0 이하일 경우 (무적, 퍼펙트 가드 등) 플로터를 표시하지 않는다
- 위젯은 위로 떠오르며 페이드 아웃 후 자동 제거된다

---

## 실행 흐름

```
WxDamageExecCalc (WxCombat)
    │
    │  GameplayCue.Damage 실행 (FGameplayCueParameters에 데미지 정보 전달)
    ▼
UWxGameplayCueNotify_Damage (C++, WxCombat)
    │
    │  HandleGameplayCue에서 DamageFloater 액터 스폰 + 나이아가라 히트 이펙트 스폰
    ▼
AWxDamageFloaterActor (C++, WxCombat)
    │
    │  WidgetComponent에 위젯 생성, IWxDamageFloaterInterface로 데이터 전달
    ▼
WBP_DamageFloater (BP 위젯, 에디터)
    │
    │  데미지 표시, 애니메이션 재생 후 액터 자기 파괴
    ▼
  제거
```

### 모듈 배치 근거

| 클래스 | 모듈 | 근거 |
|--------|------|------|
| `GameplayCue.Damage`, `Damage.Critical` 태그 | WxCore | 공용 태그 정의 |
| `IWxDamageFloaterInterface` | WxCombat | 데미지 플로터 데이터 전달 인터페이스 |
| `UWxGameplayCueNotify_Damage` | WxCombat | GameplayCue Tag 매핑, DamageFloater 액터 스폰, 나이아가라 히트 이펙트 스폰 |
| `AWxDamageFloaterActor` | WxCombat | WidgetComponent 보유, 데미지 정보 전달 |
| `BP_WxDamageFloater` | 에디터 | 액터 BP 서브클래스. FloaterWidgetClass, 애니메이션 설정 |
| `WBP_DamageFloater` | 에디터 | UMG BP 위젯. 비주얼 담당 |

---

## 클래스 설계

### IWxDamageFloaterInterface (C++, WxCombat)

데미지 플로터 위젯이 구현해야 하는 인터페이스. C++에서 위젯 생성 후 이 인터페이스를 통해 데미지 정보를 전달한다.

| 함수 | 파라미터 | 설명 |
|------|----------|------|
| `InitDamageInfo` | `float DamageAmount, bool bIsCritical` | BlueprintNativeEvent. BP 위젯이 구현하여 데미지 정보를 수신 |

### AWxDamageFloaterActor (C++, WxCombat)

```
AActor
  └─ AWxDamageFloaterActor
```

데미지 플로터 액터. 피격 위치에 스폰되어 WidgetComponent로 데미지 수치를 월드 스페이스에 표시한다. BP 서브클래스에서 위젯 클래스 설정, 떠오르기·페이드 아웃·자기 파괴를 처리한다.

#### 주요 프로퍼티

| 프로퍼티 | 타입 | 설명 |
|----------|------|------|
| `WidgetComponent` | `UWidgetComponent*` | Screen 스페이스 위젯 컴포넌트 (RootComponent) |
| `FloaterWidgetClass` | `TSubclassOf<UUserWidget>` | 데미지 플로터 위젯 클래스 (IWxDamageFloaterInterface 구현 필수) |

#### InitDamageInfo 처리 흐름

1. `FloaterWidgetClass`로 WidgetComponent에 위젯 생성
2. `IWxDamageFloaterInterface::Execute_InitDamageInfo`로 데미지/크리티컬 값 전달

### UWxGameplayCueNotify_Damage (C++, WxCombat)

```
UGameplayCueNotify_Static
  └─ UWxGameplayCueNotify_Damage
```

GameplayCue Tag 매핑, DamageFloater 액터 스폰, 나이아가라 히트 이펙트 스폰을 처리한다.

#### 주요 프로퍼티

| 프로퍼티 | 타입 | 설명 |
|----------|------|------|
| `FloaterActorClass` | `TSubclassOf<AWxDamageFloaterActor>` | 데미지 플로터 액터 클래스 |
| `HitNiagaraSystem` | `TObjectPtr<UNiagaraSystem>` | 히트 시 스폰할 나이아가라 이펙트 |

#### HandleGameplayCue 처리 흐름

1. `FloaterActorClass`를 `Parameters.Location`에 스폰
2. `Parameters.RawMagnitude`에서 데미지 수치, `Parameters.AggregatedSourceTags`에서 치명타 여부 추출
3. `AWxDamageFloaterActor::InitDamageInfo`로 데이터 전달
4. `HitNiagaraSystem`이 설정되어 있으면 `Parameters.Location`에 나이아가라 스폰

---

## 기존 코드 수정

### WxDamageExecCalc.cpp

데미지 최종 확정 후, `OutExecutionOutput` 직전 또는 직후에 GameplayCue를 실행한다.

```cpp
// 데미지 플로터 GameplayCue 실행
if (TargetASC)
{
    FGameplayCueParameters CueParams;
    CueParams.RawMagnitude = FinalDamage;
    CueParams.Location = TargetActor->GetActorLocation();
    CueParams.EffectContext = ExecutionParams.GetOwningSpec().GetEffectContext();

    // 치명타 여부를 AggregatedSourceTags로 전달
    if (bIsCritical)
    {
        FGameplayTagContainer DamageInfoTags;
        DamageInfoTags.AddTag(WxGameplayTags::Damage_Critical);
        CueParams.AggregatedSourceTags = DamageInfoTags;
    }

    TargetASC->ExecuteGameplayCue(WxGameplayTags::GameplayCue_Damage, CueParams);
}
```

**필요 변경**: 치명타 판정 결과를 `bool bIsCritical` 지역 변수로 추출하여 Cue 파라미터에 전달할 수 있도록 리팩토링한다.

---

## 신규 Gameplay Tag

**파일**: `WxGameplayTags.h / .cpp`

```cpp
// ── GameplayCue ──
WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Damage);

// ── Damage Info ──
WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Critical);
```

---

## 작업 체크리스트

### 코드 작업
| # | 작업 | 파일 |
|---|------|------|
| 1 | `GameplayCue_Damage`, `Damage_Critical` 태그 추가 | `WxGameplayTags.h/.cpp` |
| 2 | `IWxDamageFloaterInterface` 구현 | 신규 (WxCombat) |
| 3 | `AWxDamageFloaterActor` 구현 (WidgetComponent, InitDamageInfo) | 신규 (WxCombat) |
| 4 | `UWxGameplayCueNotify_Damage` 구현 (액터 스폰, HandleGameplayCue) | 신규 (WxCombat) |
| 5 | `UWxDamageExecCalc`에 치명타 플래그 추출 및 Cue 실행 코드 추가 | `WxDamageExecCalc.cpp` |

### 에디터 작업
| # | 작업 | 비고 |
|---|------|------|
| 1 | `WBP_DamageFloater` UMG 위젯 제작 (`IWxDamageFloaterInterface` 구현, 데미지 텍스트 표시) | UMG 에디터 |
| 2 | `BP_WxDamageFloater` 액터 BP 서브클래스 제작 (`FloaterWidgetClass` 설정, 떠오르기·페이드 아웃·자기 파괴) | 에디터 |
| 3 | `GC_Damage` GameplayCue BP 서브클래스에서 `FloaterActorClass`, `HitNiagaraSystem` 설정 | 에디터 |

---

## 고려 사항

- **오브젝트 풀링**: 다수의 데미지 플로터가 동시에 생성될 수 있으므로, 성능이 문제가 되면 위젯 풀링을 도입한다.
- **멀티플레이어**: `ExecuteGameplayCue`는 서버에서 호출 시 클라이언트에 자동 리플리케이션되므로, 별도 네트워크 처리가 불필요하다.
- **카메라 거리 기반 필터링**: 먼 거리의 데미지 플로터는 표시하지 않는 옵션을 고려한다.

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
UWxGameplayCueNotify_Damage (C++, WxGame)
    │
    │  BP 서브클래스에서 HandleGameplayCue 오버라이드
    ▼
BP_WxGameplayCue_Damage (BP, 에디터)
    │
    │  위젯 생성, 데이터 전달, 화면 배치
    ▼
WBP_DamageFloater (BP 위젯, 에디터)
    │
    │  애니메이션 재생 후 자동 제거
    ▼
  제거
```

### 모듈 배치 근거

| 클래스 | 모듈 | 근거 |
|--------|------|------|
| `GameplayCue.Damage` 태그 | WxCore | 공용 태그 정의 |
| `UWxGameplayCueNotify_Damage` | WxGame | C++ 베이스. GameplayCue Tag 매핑 및 확장 포인트 제공 |
| `BP_WxGameplayCue_Damage` | 에디터 | BP 서브클래스. 위젯 생성 및 데이터 전달 로직 처리 |
| `WBP_DamageFloater` | 에디터 | UMG BP 위젯. 비주얼 및 애니메이션 담당 |

---

## 클래스 설계

### UWxGameplayCueNotify_Damage (C++, WxGame)

```
UGameplayCueNotify_Static
  └─ UWxGameplayCueNotify_Damage
```

C++ 베이스 클래스. GameplayCue Tag 매핑과 BP 확장 포인트를 제공한다. 프로젝트 최초의 GameplayCue 클래스이므로, 이후 추가될 GameplayCue의 패턴 기준이 된다.

#### 주요 구현

```cpp
UCLASS(Abstract, Blueprintable)
class WXGAME_API UWxGameplayCueNotify_Damage : public UGameplayCueNotify_Static
{
    GENERATED_BODY()

public:
    UWxGameplayCueNotify_Damage();
};
```

#### 생성자

```cpp
UWxGameplayCueNotify_Damage::UWxGameplayCueNotify_Damage()
{
    GameplayCueTag = WxGameplayTags::GameplayCue_Damage;
}
```

- `Abstract`: 직접 사용 불가, BP 서브클래스 필수
- `Blueprintable`: BP에서 `HandleGameplayCue`를 오버라이드할 수 있도록 허용
- 생성자에서 `GameplayCueTag`를 설정하여 `GameplayCue.Damage` 태그와 자동 매핑

#### BP 서브클래스 (BP_WxGameplayCue_Damage)에서 처리할 내용

`HandleGameplayCue` 이벤트를 오버라이드하여 다음을 처리:

1. `Parameters.RawMagnitude`에서 데미지 수치 추출
2. `Parameters.AggregatedSourceTags`에서 `Damage_Critical` 태그 포함 여부 확인
3. `Parameters.Location`으로 월드 좌표 획득
4. 로컬 플레이어의 `PlayerController`를 통해 위젯 생성
5. `ProjectWorldLocationToScreen`으로 스크린 좌표 변환
6. 위젯(`WBP_DamageFloater`)을 뷰포트에 추가하고 데미지/크리티컬 값 전달

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
| 2 | `UWxGameplayCueNotify_Damage` 구현 | 신규 (WxGame) |
| 3 | `UWxDamageExecCalc`에 치명타 플래그 추출 및 Cue 실행 코드 추가 | `WxDamageExecCalc.cpp` |

### 에디터 작업
| # | 작업 | 비고 |
|---|------|------|
| 1 | `BP_WxGameplayCue_Damage` BP 서브클래스 제작 (HandleGameplayCue 오버라이드) | 에디터 |
| 2 | `WBP_DamageFloater` UMG 위젯 제작 (레이아웃, 애니메이션) | UMG 에디터 |

---

## 고려 사항

- **오브젝트 풀링**: 다수의 데미지 플로터가 동시에 생성될 수 있으므로, 성능이 문제가 되면 위젯 풀링을 도입한다.
- **멀티플레이어**: `ExecuteGameplayCue`는 서버에서 호출 시 클라이언트에 자동 리플리케이션되므로, 별도 네트워크 처리가 불필요하다.
- **카메라 거리 기반 필터링**: 먼 거리의 데미지 플로터는 표시하지 않는 옵션을 고려한다.

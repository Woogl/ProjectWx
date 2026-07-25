# 안 쓰는 SetByCaller 생산 경로 제거

## 계획

### 목표
`SetByCaller.*` 키 7개를 전수 감사한 결과 완전히 죽은 키는 없었다. 대신 SetByCaller를 세팅하지만 호출자가 0인 함수 2개가 남아있어 이를 제거한다. 키 선언과 GE 클래스는 그대로 두므로 동작 변화는 없다.

감사 결과 — 살아있는 생산자:

| 키 | 살아있는 생산자 |
|---|---|
| `Duration` | `WxAbility_Groggy` → `WxEffect_DrainDP` |
| `Recovery.UP` / `Recovery.MP` | `FWxDamageInfo::MakeSpecs`, 퍼펙트 가드·회피 |
| `ReflectDP` | `WxExecCalc_Damage` (퍼펙트 가드 반사) |
| `Coeff.ATK` | `FWxDamageInfo::MakeSpecs` |
| `RawDamage` | `BP_DamageZone`·`BP_LaserWall`의 `AWxEffectZone::SetByCallers` |
| `HitStop` | `AWxWeaponBase` → `UWxCombatLibrary::ApplyDamage` |

제거 대상 2곳의 유래:

* `UWxCombatLibrary::ApplyRawDamage` — 뒤잡이 이 함수로 고정 Raw 대미지를 넣던 것을 2026-06-29에 표준 대미지 테이블 경로로 옮기면서 호출자가 사라졌다. `RawDamage` 키 자체는 EffectZone 경로로 계속 쓰이므로 키는 남는다.
* `UWxEffect_NoCooldown::ApplyToASC` — 2026-07-25 `OnActivateEffects` 제거로 호출자가 사라졌다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxCombat/.../Public/WxCombatLibrary.h` | `ApplyRawDamage` 선언 제거, 이 함수만 쓰던 `GameplayTagContainer.h` include 제거, 클래스 주석의 실체 없는 "적대 판정" 문구 정정 | 수정 |
| `WxCombat/.../Private/WxCombatLibrary.cpp` | `ApplyRawDamage` 정의 제거, 이 함수만 쓰던 `WxEffect_Damage.h` include 제거 | 수정 |
| `WxCombat/.../Effect/WxEffect_NoCooldown.h` | `ApplyToASC` 선언과 `UAbilitySystemComponent` 전방선언 제거 | 수정 |
| `WxCombat/.../Effect/WxEffect_NoCooldown.cpp` | `ApplyToASC` 정의 제거, 이 함수만 쓰던 `AbilitySystemComponent.h` include 제거 | 수정 |
| `Plugins/WxCombat/README.md` | 진입점 목록에서 `ApplyRawDamage` 제거 | 수정 |

### 접근 방식
- **키 선언은 전부 유지**: `RawDamage`는 EffectZone BP가, `Duration`은 Groggy가 계속 생산한다. 죽은 것은 키가 아니라 그 키를 세팅하던 함수뿐이므로 삭제 범위를 함수와 그 함수만 쓰던 include·전방선언으로 한정한다.
- **GE 클래스 보존**: `WxEffect_NoCooldown`·`WxEffect_InfiniteMP`는 2026-07-25 결정대로 재사용 목적으로 남긴다. 생성자의 `SetByCaller.Duration` 배선도 그대로 두어 다시 쓸 때 적용 함수만 새로 쓰면 되게 한다.
- **Ultimate BP 잔여 데이터 무시**: `GA_Ultimate`·`GA_HR_Ultimate` uasset에 남은 `SetByCaller.Duration` 문자열은 제거된 `OnActivateEffects` 속성의 잔여 데이터이므로 손대지 않는다(BP 재저장 시 소멸).

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxCombat/.../Public/WxCombatLibrary.h` | `ApplyRawDamage` 선언, `GameplayTagContainer.h` include 제거. 클래스 주석에서 실체 없는 "적대 판정" 문구 삭제 | 수정 |
| `WxCombat/.../Private/WxCombatLibrary.cpp` | `ApplyRawDamage` 정의, `WxEffect_Damage.h` include 제거 | 수정 |
| `WxCombat/.../Effect/WxEffect_NoCooldown.h` | `ApplyToASC` 선언, `UAbilitySystemComponent` 전방선언 제거 | 수정 |
| `WxCombat/.../Effect/WxEffect_NoCooldown.cpp` | `ApplyToASC` 정의, `AbilitySystemComponent.h` include 제거 | 수정 |
| `Plugins/WxCombat/README.md` | 대미지 파이프라인 진입점에서 `ApplyRawDamage` 삭제 | 수정 |

### 구현·결정과 그 이유
- **키가 아니라 죽은 함수를 지웠다**: 감사 결과 7개 키 전부에 살아있는 생산자가 있었다. `RawDamage`는 두 이펙트존 BP가 데이터로 넣고 있어 C++ 호출자가 사라졌다는 사실만으로는 키를 지울 수 없다. 함수와 그 함수만 쓰던 include·전방선언까지가 안전한 최대 범위다.
- **NoCooldown 적용 함수는 남기지 않았다**: 클래스는 재사용 목적으로 보존하지만, 호출자 없는 진입점을 미리 붙여두는 것은 방어적 선언이다. 다시 쓸 때 그 시점의 필요에 맞춰 적용 함수를 쓰면 되고, 생성자의 `SetByCaller.Duration` 배선은 그대로라 재작성 비용은 몇 줄이다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- `SetByCaller.ReflectDP`와 전용 GE `UWxEffect_Reflect` 제거는 별건으로 진행한다(사용자 지시). DP 클램프·그로기 판정을 `PostGameplayEffectExecute`에서 어트리뷰트 베이스 변경 훅으로 옮기는 것이 전제다.
- `DT_Damage`의 `RecoverUP` 행 값을 실제로 쓰는지는 확인하지 못했다(에디터 미실행, DataTable 값은 바이너리라 정적 판독 불가). 전부 0이면 UP 회복 경로(`SetByCaller.Recovery.UP`, `RecoverUP` 필드, `RecoverResource`의 UP 모디파이어)가 데이터상 사문화된 것이므로 별도 판단이 필요하다.
- `GA_Ultimate`·`GA_HR_Ultimate` uasset에 제거된 `OnActivateEffects`의 잔여 데이터로 `SetByCaller.Duration`·`WxEffect_NoCooldown`·`WxEffect_InfiniteMP` 참조가 남아있다. 해당 BP를 한 번 저장하면 사라진다.

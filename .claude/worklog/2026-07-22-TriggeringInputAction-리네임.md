# TriggeringInputAction → ActivationInputAction 리네임

## 계획

### 목표
`UWxAbilityBase`의 발동 입력 필드 이름을 `TriggeringInputAction` → `ActivationInputAction`으로 바꿔, 이를 소비하는 술어(`IsActivationInput`)·프로젝트 공통 어휘(`ActivationPolicy`, 엔진 `ActivateAbility` 계열)와 정렬한다. 동작 변화 없음(순수 리네임).

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Config/DefaultEngine.ini` | `[CoreRedirects]`에 PropertyRedirect 추가(구 이름→새 이름) | 수정 |
| `WxAbilityBase.h` | 필드 선언 + 주석 2곳 | 수정 |
| `WxAbilityBase.cpp` | `IsActivationInput`/`GetInputActions` 참조 3곳 | 수정 |
| `WxAbilitySet.h` `WxAbility_Guard.h` `WxAbility_Dodge.h` `WxAbility_Attack.h` | 베이스 필드를 언급하는 주석 | 수정 |
| `Docs/Programmer/Ability_Activation_Flow.md` | 문서 내 필드명 표기 | 수정 |

### 접근 방식
- **UPROPERTY 리네임 = 직렬화 데이터 유실 위험**: 필드는 `EditDefaultsOnly`라 각 GA_*.uasset 디폴트에 InputAction이 저장돼 있다. CoreRedirect(`PropertyRedirects`)로 구 이름을 새 이름에 매핑해, 기존 BP에 지정된 값이 유실되지 않게 한다.
- **동명 엔진 필드 회피**: `UCommonButtonBase::TriggeringInputAction`(CommonUI)이 WxUI `WxButtonBase.cpp`/`WxActionWidget.h`에서 쓰인다. 이는 별개 클래스이므로 일괄 치환하지 않고 `UWxAbilityBase` 관련 위치만 정확히 바꾼다.
- **스냅샷 JSON 비수정**: `WxBlueprintSnapshot/Snapshots/*.json`의 구 이름 표기는 자동 생성물이라 BP 재저장 시 갱신되므로 수동 편집하지 않는다.
- **`EWxAbilityActivationPolicy::OnTriggered` 유지**: 여기 "Trigger"는 입력·이벤트·AI를 아우르는 상위 개념이라 입력 필드명과 층위가 달라 그대로 둔다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Config/DefaultEngine.ini` | `[CoreRedirects]`에 `+PropertyRedirects`(TriggeringInputAction→ActivationInputAction) 1줄 추가 | 수정 |
| `WxAbilityBase.h` | 필드 `ActivationInputAction`으로 리네임 + 주석 2곳 | 수정 |
| `WxAbilityBase.cpp` | `IsActivationInput`/`GetInputActions`의 필드 참조 3곳 | 수정 |
| `WxAbilitySet.h` `WxAbility_Guard.h` `WxAbility_Dodge.h` `WxAbility_Attack.h` | 베이스 필드를 언급하는 주석 표기 | 수정 |
| `Docs/Programmer/Ability_Activation_Flow.md` | 본문·표·주의점의 필드명 표기 3곳 | 수정 |

### 구현·결정과 그 이유
- **CoreRedirect 동반**: `EditDefaultsOnly` UPROPERTY라 GA_*.uasset 디폴트에 값이 직렬화돼 있다. 리다이렉트 없이 리네임하면 로드 시 지정된 InputAction이 유실되므로 `PropertyRedirects`로 매핑했다.
- **동명 엔진 필드 회피**: `UCommonButtonBase::TriggeringInputAction`(CommonUI)을 쓰는 WxUI(`WxButtonBase.cpp`·`WxActionWidget.h`)는 별개 클래스라 손대지 않았다.
- **스냅샷 JSON 비수정**: 자동 생성물이라 BP 재저장 시 새 이름으로 갱신된다. 지금은 구 이름 표기가 남아 있으나 리다이렉트로 로드는 정상.

### 계획 대비 달라진 점
- 계획대로. (빌드 전 한글 worklog 파일명발 UBT 크래시 예방으로 `git config core.quotepath false`를 선반영)

### 후속 과제
- 없음. (각 GA_ BP를 한 번씩 재저장하면 스냅샷 JSON도 새 이름으로 정리됨 — 기능엔 영향 없어 강제하지 않음)

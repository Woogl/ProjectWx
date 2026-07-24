# 처형 어빌리티 const_cast 제거

## 계획

### 목표
`UWxAbility_Finisher::ActivateAbility`가 트리거 이벤트의 대상을 꺼낼 때 쓰던 `const_cast`를 없앤다. 이 어빌리티가 대상에게 하는 일은 전부 대상 ASC를 통하고 액터 자체는 위치만 읽으므로, 비const가 실제로 필요한 게 아니라 BP 노출용 래퍼 함수의 시그니처가 const를 못 받았을 뿐이다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Finisher.h` | `TargetActor` 멤버를 `TWeakObjectPtr<const AActor>`로 | 수정 |
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp` | 캐스팅 제거, ASC 조회·이벤트 송출을 const 수용 경로로 교체, include 교체 | 수정 |

### 접근 방식
- **const를 받는 동등 경로로 교체**: 비const를 강요하던 두 호출을 엔진이 이미 제공하는 대응물로 바꾼다. 동작 변화는 없다.

| 기존 | 대체 | 근거 |
|---|---|---|
| `UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AActor*)` | `UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(const AActor*)` | BP 라이브러리 구현이 이 함수를 그대로 반환한다. 프로젝트 선례는 `WxWeaponBase` |
| `UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(AActor*, ...)` | 대상 ASC의 `HandleGameplayEvent` 직접 호출 | 엔진 구현이 유효성 검사 후 하는 일이 이것뿐. 감싸는 프리딕션 윈도 분기는 CVar 기본값에서 꺼져 있고 엔진 주석도 legacy bug로 표기 |

- **구조는 그대로**: 권위 분기·호출 순서·조기 종료 조건을 건드리지 않는다. 대상 ASC를 한 번만 조회하도록 합치는 정리도 하지 않는다. 기존에도 이벤트 송출 함수가 내부에서 따로 조회했으므로 조회 횟수는 같고, diff를 작게 유지하는 편이 낫다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Finisher.h` | 대상 멤버를 const 액터 약참조로 | 수정 |
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp` | 캐스팅 제거, 대상 조회·이벤트 송출을 const 수용 경로로, 지역 변수 const화, include 교체 | 수정 |

### 구현·결정과 그 이유
- **캐스팅 대신 const를 받는 경로로 교체**: 대상에 가하는 변경은 전부 대상 ASC를 거치고 액터 자체는 위치만 읽으므로, 비const가 필요했던 게 아니라 BP 노출용 래퍼의 시그니처가 const를 못 받았을 뿐이었다. 엔진이 같은 일을 하는 const 수용 함수를 이미 제공하므로 캐스팅 없이 대상을 const로 끝까지 유지했다.
- **이벤트 송출을 대상 ASC 직접 호출로**: BP 래퍼가 유효성 검사 뒤 하는 일이 이 호출뿐이다. 감싸던 프리딕션 윈도 분기는 CVar 기본값에서 꺼져 있고 엔진 주석도 legacy bug로 표기하므로, 기본 설정에서 동작이 같다.
- **캐스팅 사연을 적던 주석을 대체**: 왜 캐스팅하는지 변명하던 두 줄 대신, 왜 const로 충분한지를 한 줄로 남겼다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 인게임 확인 미수행. 이벤트 송출 경로만 바뀌었으므로 앞잡·뒤잡에서 짝 피격 몽타주가 동시 재생되면 동등성이 확인된다.

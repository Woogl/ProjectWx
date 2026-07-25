# WxAbilityBase OnActivateEffects 제거

## 계획

### 목표
어빌리티가 발동 시 거는 GE를 BP 데이터 배열로 받던 `UWxAbilityBase::OnActivateEffects`를 없앤다. 어빌리티가 거는 GE는 그 어빌리티의 C++ 클래스가 멤버 변수로 직접 들고 있어야 한다는 방침을 따르며, 배열을 실제로 쓰던 BP 4종의 효과는 전부 폐기하기로 결정했다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/.../Public/AbilitySystem/Ability/WxAbilityBase.h` | `FWxAbilityEffect` 구조체·`OnActivateEffects` 프로퍼티·`ActivateAbility` 오버라이드 선언 삭제 | 수정 |
| `Plugins/WxCombat/.../Private/AbilitySystem/Ability/WxAbilityBase.cpp` | `ActivateAbility` 정의 삭제 | 수정 |
| `Plugins/WxCombat/.../Public/AbilitySystem/Ability/WxAbility_Attack.h` | 클래스 주석의 `OnActivateEffects` 언급 제거 | 수정 |
| `Plugins/WxCombat/.../Public/AbilitySystem/Ability/WxAbility_Skill.h` | 클래스 주석의 `OnActivateEffects` 언급 제거 | 수정 |
| `Docs/Programmer/Ability_Activation_Flow.md` | 흐름도·본문·구성요소 표에서 `OnActivateEffects` 제거 | 수정 |

### 접근 방식
- **베이스 오버라이드 자체를 제거**: 배열 루프를 걷어내면 `UWxAbilityBase::ActivateAbility`에는 `Super` 호출만 남는다. 자식들의 `Super::ActivateAbility`는 `UGameplayAbility::ActivateAbility`로 이어지므로 BP 이벤트 경로를 포함해 동작이 그대로다.
- **폐기 대상**: `GA_Ultimate`·`GA_HR_Ultimate`의 NoCooldown/InfiniteMP(각 10초), `GA_Skill_4`의 GE_Exceed, `GA_HR_Skill_4`의 빈 엔트리. 프로퍼티가 사라지면 BP에 저장된 델타는 로드 시 버려지므로 에디터 작업은 필요 없다.
- **기준 형태는 이미 존재**: `UWxAbility_Sprint`가 `SprintEffectClass` 멤버를 생성자에서 고정해 적용하는 방식이 목표하는 형태라 스프린트는 손대지 않는다.
- **미사용이 되는 GE/큐**(`WxEffect_NoCooldown`·`WxEffect_InfiniteMP`·`WxEffect_Exceed`·`WxCueNotify_Exceed`와 관련 에셋)는 추후 재사용을 위해 남긴다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/.../Public/AbilitySystem/Ability/WxAbilityBase.h` | `FWxAbilityEffect`·`OnActivateEffects`·`ActivateAbility` 선언 삭제 | 수정 |
| `Plugins/WxCombat/.../Private/AbilitySystem/Ability/WxAbilityBase.cpp` | `ActivateAbility` 정의 삭제 | 수정 |
| `Plugins/WxCombat/.../Public/AbilitySystem/Ability/WxAbility_Attack.h` | 재발동 설명에서 배열 언급 제거 | 수정 |
| `Plugins/WxCombat/.../Public/AbilitySystem/Ability/WxAbility_Skill.h` | 재발동 설명에서 배열 언급 제거 | 수정 |
| `Docs/Programmer/Ability_Activation_Flow.md` | 흐름도·활성화 본체 설명·구성요소 표 갱신 | 수정 |

### 구현·결정과 그 이유
- **베이스의 `ActivateAbility` 오버라이드를 통째로 제거**: 배열 루프가 사라지면 부모 호출만 남아 존재 이유가 없다. 자식의 부모 호출이 엔진 순정으로 바로 이어지므로 BP 이벤트 경로도 그대로다.
- **효과 폐기 방식**: BP 델타는 프로퍼티가 사라지면 로드 시 버려지므로, 궁극기 버프와 스킬4 버프는 코드 변경만으로 적용이 끊긴다. 에디터에서 에셋을 손댈 필요가 없다.
- **스프린트는 손대지 않음**: 이미 자기 GE를 멤버로 들고 생성자에서 고정해 적용하는 형태라, 앞으로 발동 시 버프가 필요한 어빌리티가 따라야 할 기준이 된다. 문서의 활성화 본체 설명에도 이 형태를 예시로 남겼다.
- **미사용이 된 GE/큐 유지**: 쿨다운 무효화·MP 무한·Exceed 계열은 참조가 0이 되지만 재사용 여지가 있어 남겼다.

### 계획 대비 달라진 점
계획대로.

### 후속 과제
- `GA_Ultimate`·`GA_Skill_4` 스냅샷 JSON은 해당 BP를 다음에 저장할 때 갱신된다(현재는 사라진 프로퍼티가 남아 있음).
- 참조가 0이 된 GE/큐(쿨다운 무효화·MP 무한·Exceed 계열과 그 에셋)는 재사용 계획이 없다면 별도 작업으로 정리한다.

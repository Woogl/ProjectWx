# AbilitySet 재부여 차단 (재빙의 시 중복 부여 방지)

## 계획

### 목표
`GiveAbilitySet()`에 재진입 가드가 없어 재빙의(언포제스→리포제스) 때마다 어빌리티·GE가 중복 부여되고 어트리뷰트가 초기값으로 되돌아간다(전투 중 재빙의 = 풀피 회복). AbilitySet 부여를 ASC 수명 동안 최초 1회만 실효하게 만든다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/.../Public/AbilitySystem/WxAbilitySet.h` | `FWxAbilitySetGrantedHandles` 구조체 삭제, `GiveToAbilitySystem`에서 `OutHandles` 파라미터 제거, 불필요해진 핸들 include 제거 | 수정 |
| `Plugins/WxCombat/.../Private/AbilitySystem/WxAbilitySet.cpp` | `RemoveFromAbilitySystem` 정의 삭제, `OutHandles` 수집 분기 삭제 | 수정 |
| `Plugins/WxCombat/.../Public/AbilitySystem/WxAbilitySystemComponent.h` | 멤버 `AbilitySetGrantedHandles` → `bAbilitySetGranted` 플래그로 교체 | 수정 |
| `Plugins/WxCombat/.../Private/AbilitySystem/WxAbilitySystemComponent.cpp` | `GiveAbilitySet()` 진입부 조기 반환 가드 + 플래그 세팅 | 수정 |

### 접근 방식
- **부여 여부 플래그 조기 반환**: ASC는 캐릭터 서브오브젝트라 재빙의 후에도 앞서 부여한 내용을 그대로 쥐고 있으므로, 재부여(제거 후 다시 부여)가 아니라 두 번째 호출을 통째로 무시하는 쪽이 맞다.
- **핸들 저장소 제거**: `RemoveFromAbilitySystem`은 호출부가 0개였고, 조기 반환 가드가 들어가면 핸들을 보관할 이유가 사라진다.
- **핸들 비어 있음으로 가드하지 않는 이유**: 어빌리티·GE 없이 어트리뷰트 행만 지정한 세트에서는 핸들이 비어 있어 가드가 통과하고 어트리뷰트가 다시 초기화된다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/.../Public/AbilitySystem/WxAbilitySet.h` | `FWxAbilitySetGrantedHandles` 삭제, `GiveToAbilitySystem(ASC)`로 시그니처 축소, 핸들 include 2개 제거 | 수정 |
| `Plugins/WxCombat/.../Private/AbilitySystem/WxAbilitySet.cpp` | `RemoveFromAbilitySystem` 정의 삭제, 부여 반환 핸들 수집 분기 삭제 | 수정 |
| `Plugins/WxCombat/.../Public/AbilitySystem/WxAbilitySystemComponent.h` | `AbilitySetGrantedHandles` → `bAbilitySetGranted` | 수정 |
| `Plugins/WxCombat/.../Private/AbilitySystem/WxAbilitySystemComponent.cpp` | `GiveAbilitySet()` 조기 반환 가드 + 플래그 세팅 | 수정 |

### 구현·결정과 그 이유
- **재부여가 아니라 무시**: ASC가 캐릭터 서브오브젝트라 재빙의 후에도 부여 내용이 살아 있다. 제거 후 다시 부여하면 어트리뷰트 초기화가 다시 돌아 풀피 회복 문제가 그대로 남는다.
- **플래그는 비복제**: `GiveAbilitySet()`은 `HasAuthority()` 안에서만 호출되므로 서버 상태만으로 충분하다.
- **핸들 저장소 제거**: 제거 경로가 사라져 보관 이유가 없다. 어빌리티 수만큼 핸들 배열을 들고 있을 필요도 없다.

### 계획 대비 달라진 점
- 계획대로

### 후속 과제
- `InitAbilitySystem`의 어트리뷰트 변경 델리게이트는 재빙의마다 중복 구독된다(`WxCharacterBase.cpp:206`, `WxEnemyCharacter.cpp:50`). 핸들러가 무해해 이번 건과 분리했다.

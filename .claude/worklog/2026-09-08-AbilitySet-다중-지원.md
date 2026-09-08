# WxAbilitySystemComponent 다중 AbilitySet 지원

## 계획

### 목표
ASC가 `TObjectPtr<UWxAbilitySet> AbilitySet` 단일 필드만 들고 있어 공용 세트 + 캐릭터 전용 세트처럼 나눠 저작할 수 없다. 세트를 배열로 받아 여러 개를 순서대로 부여한다. 범위는 에디터 저작 배열까지이며, 부여 시점(`InitAbilitySystem`, 서버 권위)과 1회 부여 가드는 그대로 두고 런타임 추가 부여·해제 API는 만들지 않는다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/.../Public/AbilitySystem/WxAbilitySystemComponent.h` | `AbilitySet` → `TArray<TObjectPtr<UWxAbilitySet>> AbilitySets`, `bAbilitySetGranted` → `bAbilitySetsGranted`, `GiveAbilitySet()` → `GiveAbilitySets()` | 수정 |
| `Plugins/WxCombat/.../Private/AbilitySystem/WxAbilitySystemComponent.cpp` | `GiveAbilitySets()`·`GetAbilityInputActions()`를 배열 순회로 교체 | 수정 |
| `Plugins/WxCombat/.../Public/AbilitySystem/WxAbilitySet.h` | `GetInputActions()` → `AppendInputActions(TArray<const UInputAction*>& OutActions) const`, 클래스 주석을 다중 지정 기준으로 정정 | 수정 |
| `Plugins/WxCombat/.../Private/AbilitySystem/WxAbilitySet.cpp` | 위 시그니처에 맞춰 수집부 교체(호출부가 넘긴 배열에 `AddUnique`) | 수정 |
| `Source/WxGame/Character/WxCharacterBase.cpp` | `GiveAbilitySet()` 호출·주석을 새 이름으로 | 수정 |
| `Content/Character/*/BP_*.uasset` (6개) | 빌드 후 에디터에서 `AbilitySets[0]`에 동명 `ABS_*` 재지정 | 수정(데이터·사용자 수행) |

### 접근 방식
- **배열 순회 부여**: `GiveAbilitySets()`는 `bAbilitySetsGranted`로 조기 반환한 뒤 플래그를 세우고, 유효한 세트마다 기존 `GiveToAbilitySystem(this)`를 그대로 호출한다. 회수 경로가 없으므로 세트별 핸들 보관은 이번에도 필요 없다(`2026-08-25-AbilitySet-재부여-차단.md`).
- **적용 순서 = 배열 순서**: 두 세트가 모두 `AttributeInitRow`를 지정하면 뒤 세트가 앞을 덮는다. 빈 핸들은 `FDataTableRowHandle::GetRow`가 조용히 `nullptr`을 돌려줘 무시되므로, 어트리뷰트 행은 한 세트에만 두는 저작이 자연스럽게 성립한다.
- **입력 액션 수집은 out 파라미터로**: 세트마다 임시 배열을 만들어 합치면 `AddUnique` 지점이 둘로 갈린다. 세트가 호출부 배열에 직접 `AddUnique`하게 해 세트 간 중복 IA를 한 곳에서 거른다. 호출부는 `AWxPlayerCharacter::SetupPlayerInputComponent` 하나뿐이다.
- **중복 부여는 저작 규칙으로**: 서로 다른 세트가 같은 어빌리티 클래스나 GE를 담으면 두 번 부여된다. 감지 코드를 넣어봐야 어느 쪽이 이겼는지가 데이터에서 안 보이므로 세트를 겹치지 않게 저작하는 쪽으로 둔다.
- **BP 데이터 재지정이 필요한 이유**: 단일 오브젝트 프로퍼티가 배열이 되면 태그드 프로퍼티 로더가 타입 불일치로 기존 값을 버린다 — `FArrayProperty::ConvertFromType`은 비배열 태그에 `UseSerializeItem`을 돌려주고 상위 루프가 건너뛴다(UE 5.8 `PropertyArray.cpp:1407`). CoreRedirect로도 못 잇는다. 대상 6개 BP는 각각 같은 폴더의 동명 `ABS_*` 하나를 참조해 매핑이 1:1이고, 레벨 배치 인스턴스의 오버라이드는 없다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/.../Public/AbilitySystem/WxAbilitySystemComponent.h` | `AbilitySets` 배열·`bAbilitySetsGranted`로 교체, `GiveAbilitySets()` 리네임, 적용 순서 주석 추가 | 수정 |
| `Plugins/WxCombat/.../Private/AbilitySystem/WxAbilitySystemComponent.cpp` | `GiveAbilitySets()`·`GetAbilityInputActions()` 배열 순회 | 수정 |
| `Plugins/WxCombat/.../Public/AbilitySystem/WxAbilitySet.h` | `GetInputActions()` → `AppendInputActions(OutInputActions)`, 클래스 주석 정정 | 수정 |
| `Plugins/WxCombat/.../Private/AbilitySystem/WxAbilitySet.cpp` | 받은 배열에 `AddUnique`로 덧붙이도록 교체 | 수정 |
| `Source/WxGame/Character/WxCharacterBase.cpp` | 호출·주석을 `GiveAbilitySets`로 | 수정 |

### 구현·결정과 그 이유
- **빈 배열에서도 플래그를 세운다**: 이전 가드는 세트가 없으면 플래그를 세우지 않았지만, 부여할 것이 없다는 결과가 같아 조건을 나눌 이유가 없다. 진입부가 `bAbilitySetsGranted` 하나만 보므로 재빙의 차단 근거도 그대로다.
- **입력 액션을 out 파라미터로 모은다**: 세트별로 배열을 만들어 합치면 `AddUnique`가 세트 안과 병합부 두 곳에 생긴다. 호출부 배열에 직접 덧붙이면 세트 간 중복 IA까지 한 지점에서 걸러진다.
- **어트리뷰트 행 충돌은 순서로 규정**: 뒤 세트가 앞을 덮는다는 사실만 프로퍼티 주석에 남겼다. 병합 규칙을 코드로 정하면 데이터만 봐서는 결과를 못 읽는다.

### 계획 대비 달라진 점
- 계획대로

### 후속 과제
- `BP_Boss`·`BP_Enemy`·`BP_Minion`·`BP_Player`·`BP_Sandbag`·`BP_Soldier` 6개의 `AbilitySets`에 동명 `ABS_*`를 다시 지정해야 한다. 프로퍼티 타입이 바뀌어 기존 값이 로드 시 폐기되므로, 지정 전에는 어트리뷰트가 0이라 캐릭터가 사망 상태로 뜬다.

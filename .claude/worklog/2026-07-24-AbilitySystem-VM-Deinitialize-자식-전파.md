# AbilitySystem VM Deinitialize 자식 전파

## 계획

### 목표
`UWxViewModel_AbilitySystem::Deinitialize`가 자식 VM 배열을 `Empty()`로 떼기만 하고 각 자식 `Deinitialize()`를 부르지 않아, 배열에서 떨어진 자식(Effect/Ability)이 GC의 `BeginDestroy→Deinitialize`까지 FTSTicker 티커·ASC 구독을 유지하던 문제를 수정한다. `Empty()` 직전에 자식 `Deinitialize()`를 호출해 teardown을 결정적으로 만든다. `module_review_WxUI.md` 발견 #3. 동작 결과는 불변.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp` | `Deinitialize`에서 세 배열 `Empty()` 직전에 원소 `Deinitialize()` 전파 순회 추가 | 수정 |

### 접근 방식
- **자식 전파**: `UWxViewModel_Character::Deinitialize`(`WxViewModel_Character.cpp:27`)가 자식 `AbilitySystem->Deinitialize()`를 부르는 패턴을 동일하게 적용. `AttributeViewModels`/`AbilityViewModels`/`ActiveEffectViewModels` 각각을 range-for로 순회하며 원소 `Deinitialize()` 호출 후 기존 `Empty()`.
- **명시적 3중 루프**: 원소 타입이 서로 달라(Attribute/Ability/Effect) 헬퍼 추출 없이 풀어쓴다(프로젝트 방침: 작은 헬퍼·과한 추상화 지양). null 원소는 파일 내 기존 관례(`if (VM && ...)`)대로 건너뛴다.
- **안전성**: 자식 `Deinitialize()`는 모두 idempotent(`TickerHandle.IsValid()`·`CachedASC.Get()` 가드, `RemoveAll` 재호출 무해)라 GC BeginDestroy의 재호출과 겹쳐도 안전. 자식 Deinitialize는 부모로 되돌아오는 브로드캐스트가 없어 재진입 없음.
- 부모 자신의 ASC 델리게이트 해제·`OwnedTags.Reset()`·`Super::Deinitialize()`는 현행 위치 유지.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp` | `Deinitialize`에서 세 배열 `Empty()` 직전에 원소 `Deinitialize()` 전파 순회 3개 추가 | 수정 |

### 구현·결정과 그 이유
- **자식 전파로 결정적 teardown**: `Empty()`는 참조만 끊을 뿐 자식의 티커·구독을 멈추지 않아, 부모 파괴 후에도 자식이 GC까지 살아 티킹했다. `Character VM`이 자식을 명시 Deinitialize하는 것과 동일하게 세 자식 배열도 Deinitialize를 전파해 티커·ASC 구독을 즉시 정리한다.
- **3중 명시 루프**: 원소 타입(Attribute/Ability/Effect)이 달라 공통 헬퍼가 이득 없음. 프로젝트 방침(작은 헬퍼·과한 추상화 지양)대로 풀어썼고, null 원소는 파일 내 기존 관례대로 건너뛴다.
- **이중 호출 안전**: 자식 Deinitialize는 idempotent라 GC BeginDestroy의 재호출과 겹쳐도 무해. 부모로 되돌아오는 브로드캐스트가 없어 재진입도 없다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 런타임 확인 미수행(컴파일만 검증): VM 재초기화·파괴 시 자식 티커가 즉시 멈추는지, 표시·수치 동작이 불변인지는 인게임 확인 권장.
- 리뷰 문서 `module_review_WxUI.md` 발견 #3은 수정됐으므로 다음 `/module-review` 재실행 시 해소 반영됨.

# WxViewModel IsInitialized 초기화 상태 추적 기능 제거

## 계획

### 목표
`UWxViewModel`(모든 뷰모델의 추상 베이스)의 초기화 상태 추적 기능(`IsInitialized`/`SetInitialized`/`bInitialized`)을 전부 제거한다. getter만 떼면 `SetInitialized` 내부 FieldNotify 브로드캐스트가 컴파일되지 않으므로 trio와 모든 호출부를 함께 들어낸다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` | `IsInitialized()`/`SetInitialized()` 선언+주석, `bInitialized` 멤버 제거. 비게 되는 `protected:`/`private:` 섹션 제거 | 삭제 |
| `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp` | `IsInitialized()`/`SetInitialized()` 정의 제거 | 삭제 |
| `Source/WxGame/MVVM/WxViewModel_Item.cpp` | `SetInitialized(...)` 호출 3곳 제거 | 수정 |
| `Source/WxGame/MVVM/WxViewModel_Inventory.cpp` | `SetInitialized(...)` 호출 2곳 제거 | 수정 |
| `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_InteractionList.cpp` | `SetInitialized(...)` 호출 2곳 제거 | 수정 |
| `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp` | `SetInitialized(...)` 호출 2곳 제거 | 수정 |
| `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp` | `SetInitialized(...)` 호출 2곳 제거 | 수정 |
| `Plugins/WxUI/README.md` | 30행 `IsInitialized` 규약 문구, 36행 새 뷰모델 호출 규약 문장 제거 | 수정 |

### 접근 방식
- **기능 전체 제거**: 베이스의 trio를 제거하고, 파생 VM 5종(총 9곳)의 `Initialize`/`Deinitialize` 내 `SetInitialized` 호출을 제거. 주변 빈 줄 정리.
- **BP 바인딩**: `WBP_Nameplate_Boss`가 `bIsVisible`를 `IsInitialized` FieldNotify에 바인딩 중 → 본 C++ 작업에서는 .uasset/스냅샷을 건드리지 않고, 완료 후 에디터 수동 정리를 안내.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` | `IsInitialized()`/`SetInitialized()`/`bInitialized` 제거, 빈 `protected:`/`private:` 섹션 제거 | 삭제 |
| `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp` | `IsInitialized()`/`SetInitialized()` 정의 제거 | 삭제 |
| `Source/WxGame/MVVM/WxViewModel_Item.cpp` | `SetInitialized` 호출 3곳 제거 | 수정 |
| `Source/WxGame/MVVM/WxViewModel_Inventory.cpp` | `SetInitialized` 호출 2곳 제거 | 수정 |
| `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_InteractionList.cpp` | `SetInitialized` 호출 2곳 제거 | 수정 |
| `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp` | `SetInitialized` 호출 2곳 제거 | 수정 |
| `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp` | `SetInitialized` 호출 2곳 제거 | 수정 |
| `Plugins/WxUI/README.md` | 핵심 타입 표 + 새 뷰모델 규약에서 `IsInitialized`/`SetInitialized` 문구 갱신 | 수정 |

### 구현·결정과 그 이유
- **trio + 호출부 동시 제거**: `SetInitialized` 내부의 `UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsInitialized)`는 getter FieldNotify에 의존하므로, getter만 떼면 컴파일이 깨진다. 기능 전체를 들어내는 것이 유일한 정합적 제거.
- **README 규약 재작성**: 기존 "새 뷰모델" 규약이 `SetInitialized` 호출을 의무화했으므로, 기능 제거에 맞춰 `Deinitialize`/재초기화 규약만 남도록 다시 썼다.

### 계획 대비 달라진 점
- 계획대로. (README 30행은 단순 문구 삭제 대신 베이스의 실제 역할로 교체)

### 후속 과제
- **`WBP_Nameplate_Boss` 바인딩 수동 정리**: 위젯 `bIsVisible`가 `IsInitialized` FieldNotify에 MVVM 바인딩되어 있었으므로, 에디터에서 해당 바인딩을 제거/대체해야 한다. (C++ 빌드는 통과하나 BP는 컴파일 경고/끊긴 바인딩이 남을 수 있음.) 정리 후 BP 저장 시 스냅샷 JSON은 자동 갱신.

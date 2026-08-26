# Patrol 태스크의 BlackboardKey 편집 잠금

## 계획

### 목표
`UWxBTTask_Patrol` 은 이동 목표를 하드코딩된 `PatrolTargetLocation` 에 쓰지만, 실제 이동은 엔진이 `EditAnywhere` 로 노출한 `BlackboardKey` 를 읽어 수행한다. 디자이너가 그 키를 바꾸면 경고 없이 정찰이 실패하거나 폰이 엉뚱한 곳으로 걸어간다. 이 노드는 `UWxPatrolComponent` 와 고정된 계약을 가지므로 키를 저작 항목에서 잠근다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Public/WxBTTask_Patrol.h` | `UCLASS(HideCategories = (Blackboard))` 로 변경, `InitializeFromAsset` override 선언, 클래스 주석 정정 | 수정 |
| `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp` | `InitializeFromAsset` 구현(`Super::` 전에 `SelectedKeyName` 고정), 생성자 주석 정정 | 수정 |

### 접근 방식
- **저작 잠금(`HideCategories`)**: `BlackboardKey` 는 `Category = Blackboard` 다. 클래스에 `HideCategories` 를 주면 디테일 패널이 그 카테고리를 통째로 걸러낸다(`FObjectPropertyNode::GetCategoryProperties` → `FEditorCategoryUtils::IsCategoryHiddenFromClass`). 회색 처리는 엔진 `FBlackboardSelectorDetails` 가 행 활성 상태를 자기 기준으로만 정해 `CanEditChange` 를 무시하므로 통하지 않고, 그러려면 에디터 모듈에 디테일 커스터마이징을 새로 붙여야 해 과하다.
- **기존 직렬화 값 무력화(`InitializeFromAsset`)**: 카테고리를 숨기기만 하면 이미 저장된 키는 보이지도 고칠 수도 없는 상태로 남는다. `UBTTask_BlackboardBase::InitializeFromAsset` 이 `Super::` 뒤에 `ResolveSelectedKey` 를 부르므로, 그 앞에서 `SelectedKeyName` 을 덮어써 해석 결과를 고정한다.
- 함께 숨겨지는 `ObservedBlackboardValueTolerance` 는 EditCondition 인 `bObserveBlackboardValue` 가 UE 5.8 AIModule 어디서도 true 로 설정되지 않아 어차피 항상 비활성이라 손실이 없다. 키 이름은 `GetStaticDescription` 덕에 노드 그래프에 계속 보인다.
- `WxBTTask_ReturnHome` 은 키를 읽기만 해 같은 위험이 없으므로 건드리지 않는다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Public/WxBTTask_Patrol.h` | `UCLASS(HideCategories = (Blackboard))`, `InitializeFromAsset` override 선언, 클래스 주석에 키 고정 규약 명시 | 수정 |
| `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp` | `InitializeFromAsset` 추가(`Super::` 전에 `SelectedKeyName` 고정), 생성자에서 기본 키 대입 제거 | 수정 |

### 구현·결정과 그 이유
- **생성자의 기본 키 대입을 지웠다**: `InitializeFromAsset` 이 해석 직전에 무조건 덮어쓰므로 CDO 기본값을 따로 맞춰 둘 이유가 없어졌다. 두 곳에 같은 키를 적어 두면 나중에 한쪽만 바뀔 여지가 생긴다.
- **`HideCategories` 만으로 끝내지 않았다**: 카테고리를 숨기는 것은 앞으로의 저작만 막는다. 이미 다른 키가 직렬화된 에셋이 있으면 값이 살아 있는 채로 패널에서 사라져 오히려 손댈 수 없게 된다.
- **회색 처리 대신 숨김을 택했다**: 엔진 `FBlackboardSelectorDetails` 가 헤더 행 활성 상태를 자기 기준으로만 정해 `CanEditChange` 를 무시한다. 회색 처리하려면 에디터 모듈에 디테일 커스터마이징을 새로 붙여야 해 잠금 목적에 비해 과하다.

### 계획 대비 달라진 점
- 계획엔 없던 **생성자 기본 키 대입 제거**를 함께 했다(위 사유).

### 후속 과제
- 에디터에서 `Content/AI/BT_Enemy.uasset` 을 열어 Patrol 노드 디테일에 Blackboard 카테고리가 없고 노드 제목이 `Patrol: PatrolTargetLocation` 으로 유지되는지 눈으로 확인. 컴파일(WxEditor Development)까지만 검증했다.

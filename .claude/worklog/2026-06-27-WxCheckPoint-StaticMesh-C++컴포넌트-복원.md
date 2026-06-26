# WxCheckPoint StaticMesh를 C++ MeshComponent로 복원

## 계획

### 목표
커밋 `ef1c7fe9`에서 제거됐던 `AWxCheckPoint`의 C++ `MeshComponent`(UStaticMeshComponent)를 되살린다. ef1c7fe9은 이를 BP-native `StaticMesh` 컴포넌트로 대체했으나, 메시를 다시 C++ 컴포넌트가 소유하도록 되돌린다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Source/WxGame/WorldObject/WxCheckPoint.h` | `MeshComponent` UPROPERTY 복원 (전방 선언은 이미 존재) | 수정 |
| `Source/WxGame/WorldObject/WxCheckPoint.cpp` | 생성자에서 `MeshComponent` 생성·RootComponent 부착 복원 (include는 이미 존재) | 수정 |

### 접근 방식
- **빈 컴포넌트만 C++로**: C++은 `MeshComponent`만 생성하고, SM_TorchBase 메시·트랜스폼은 BP 디폴트에서 지정(기존 방식·컨벤션, 예제 콘텐츠 경로를 코드에 박지 않음).
- **InteractionComponent는 루트 유지**: ef1c7fe9의 "InteractionComponent를 루트에 직접 부착" 의도 보존. MeshComponent 밑으로 재중첩하지 않음 — BP override `X=-90`이 메시 오프셋과 이중 적용되는 것을 회피.
- **bReplicates 미변경**: 같은 커밋에서 함께 빠졌으나 StaticMesh와 무관해 이번 범위 밖.
- **BP 후속(에디터, 사용자)**: 상속된 C++ MeshComponent에 SM_TorchBase·트랜스폼(X=-90,Z=-90) 지정, 중복 BP-native `StaticMesh` 컴포넌트 삭제, 저장 시 스냅샷 자동 갱신.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Source/WxGame/WorldObject/WxCheckPoint.h` | `MeshComponent` UPROPERTY 복원 | 수정 |
| `Source/WxGame/WorldObject/WxCheckPoint.cpp` | 생성자에서 `MeshComponent` 생성·RootComponent 부착 복원, `InteractionComponent`를 `MeshComponent` 밑으로 재중첩, `bReplicates = true` 복원(+사유 주석) | 수정 |

### 구현·결정과 그 이유
- **메시 에셋은 BP에 위임**: C++은 빈 `MeshComponent`만 생성. SM_TorchBase는 예제/플레이스홀더 콘텐츠라 경로를 코드에 박지 않고 BP 디폴트에 맡겨, 에셋 교체·이동에 코드가 영향받지 않게 했다.
- **ef1c7fe9 이전 구조로 완전 복원**: 최종적으로 `bReplicates`·`MeshComponent`·`InteractionComponent`(MeshComponent 자식) 모두 ef1c7fe9 이전 원형으로 되돌아갔다.

### 계획 대비 달라진 점
- **`bReplicates = true` 함께 복원**: 계획에선 범위 밖으로 뒀으나, 구현 중 사용자가 직접 복원. OnInteracted가 서버 Multicast RPC라 액터 복제가 필요하다는 원래 사유 주석도 같이 되살리고 trailing whitespace를 정리했다.
- **InteractionComponent 재중첩**: 계획은 "루트 유지(이중 오프셋 회피)"였으나, 사용자가 `MeshComponent` 자식으로 되돌려 원래 계층을 복원. BP override `X=-90`은 본래 이 계층에서 보정된 값이라 원형과 일치한다.

### 후속 과제
- **BP 에디터 작업(사용자)**: BP_CheckPoint에서 상속된 C++ `MeshComponent`에 SM_TorchBase·트랜스폼(X=-90,Z=-90) 지정 후, 중복되는 BP-native `StaticMesh` 컴포넌트 삭제. 저장 시 스냅샷 자동 갱신.
- **오프셋 주의**: `InteractionComponent`가 다시 `MeshComponent` 자식이므로, BP에서 메시 트랜스폼을 정하면 상호작용 볼륨이 그만큼 따라 이동한다(InteractionComponent의 BP `X=-90`에 메시 오프셋이 가산됨). 위치가 어긋나면 BP에서 재보정.
- 직전 턴의 `git checkout`으로 BP-native `StaticMesh`가 되살아나 있어, 위 작업 전까지 `.uasset`/스냅샷은 그 컴포넌트를 포함한 상태다.

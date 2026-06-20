# WxElevator 완전 StateTree 전환 — 플랫폼 위치 C++ 잔재 제거

## 계획

### 목표
`WxElevator` 의 마지막 C++ 위치 제어(플랫폼 초기/복원 스냅: `SetPlatformDistance` + `CachedSplineLength` + BeginPlay 스냅)를 없애 완전히 StateTree 구동으로 만든다. 원인은 공용 태스크 `ComponentSplineMove` 가 "가장 가까운 포인트의 다음으로 전진" 모델이라 초기 진입에서 State 의 목표 끝점을 모르기 때문. 이 태스크를 "명시한 목표 스플라인 포인트로 이동" 모델로 바꿔 초기 진입에서도 끝점으로 스냅하게 한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Public/Gimmick/WxGimmickStateTreeNodes.h` | `ComponentSplineMoveInstanceData` 에 `int32 TargetPointIndex` 추가, 구조체/개요 주석을 "목표 포인트로 이동" 으로 정정 | 수정 |
| `Plugins/WxWorld/.../Private/Gimmick/WxGimmickStateTreeNodes.cpp` | `ComponentSplineMove::EnterState` 재작성(초기=목표 포인트 스냅, 라이브=nearest→목표 슬라이드), `GetDescription` 갱신, `Tick` 불변 | 수정 |
| `Plugins/WxWorld/.../Public/Gimmick/WxElevator.h` | `SetPlatformDistance` 선언·`CachedSplineLength` 멤버 제거, "위치 정보" doc 정정 | 수정 |
| `Plugins/WxWorld/.../Private/Gimmick/WxElevator.cpp` | BeginPlay 의 캐시·스냅 제거(인터랙션 바인딩 + StartLogic 유지), `SetPlatformDistance` 정의 제거, 닫힌 루프 주석 정정 | 수정 |

### 접근 방식
- **명시-목표 모델**: 각 State 의 Spline Move 가 `TargetPointIndex` 로 자기 끝점을 직접 선언. 초기 진입(`!SourceStateID.IsValid()`)이면 그 끝점 거리로 즉시 스냅(복원 정확), 라이브 전이면 현재(가장 가까운) 포인트에서 목표까지 일정 속도 슬라이드.
- **파급 최소화**: 소비자가 WxElevator 단독이라 모델 변경 파급은 ST_Elevator 재오서링뿐. USTRUCT 이름·DisplayName 유지로 에셋의 태스크 참조 보존(파라미터만 추가). `SetClosedLoop(true)` 는 유지(새 로직은 끝점 거리만 목표, 폐합 구간 무관).

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxGimmickStateTreeNodes.h` | `ComponentSplineMoveInstanceData` 에 `TargetPointIndex` 추가, 구조체·개요·런타임 필드 주석을 "목표 포인트로 이동" 모델로 정정 | 수정 |
| `WxGimmickStateTreeNodes.cpp` | `ComponentSplineMove::EnterState` 재작성(초기=목표 포인트 스냅, 라이브=nearest→목표 슬라이드), `GetDescription` 에 목표 포인트 표시 | 수정 |
| `WxElevator.h` | `SetPlatformDistance` 선언·`CachedSplineLength` 멤버 제거, "위치 정보" doc 정정 | 수정 |
| `WxElevator.cpp` | BeginPlay 캐시·스냅 제거, `SetPlatformDistance` 정의 제거, 닫힌 루프 주석 정정 | 수정 |

### 구현·결정과 그 이유
- **명시-목표로 모델 전환**: 기존 "가장 가까운 포인트의 다음으로 전진" 은 초기 진입에서 목적지를 알 수 없어 C++ 스냅을 강제했다. 각 상태가 `TargetPointIndex` 로 자기 끝점을 직접 선언하게 바꾸니, 초기 진입에서도 그 끝점으로 스냅할 수 있어 플랫폼 위치를 전적으로 StateTree 가 소유하게 됐다. 라이브 전이의 시작점은 여전히 "가장 가까운 포인트"(정지 시 끝점에 주차됨)라 한 끝점에서 다른 끝점으로 자연스럽게 슬라이드한다.
- **태스크 식별자 보존**: USTRUCT 이름·DisplayName 을 유지해 ST_Elevator 의 기존 노드 참조가 끊기지 않게 했다. 파라미터만 추가됐으므로 에셋은 `TargetPointIndex` 만 채우면 된다.
- **닫힌 루프 유지**: 새 로직은 끝점 포인트 거리만 목표하고 폐합 구간을 건드리지 않아, 스플라인을 열어 형상/길이를 바꿀 필요가 없다. 에셋 재검증 회피 차 `SetClosedLoop(true)` 를 그대로 뒀다(주석만 정정).
- **검증**: WxEditor(Development) 빌드 `Result: Succeeded`. WxWorld·WxGame 컴파일·링크 완료, 경고는 변경과 무관한 엔진 C4996 뿐.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- **에디터 재오서링(사용자)**: `Content/WorldObject/Gimmick/ST_Elevator.uasset` 의 Start/End 끝점별 부모 상태 Wx Component Spline Move 노드에 `TargetPointIndex` 설정(Start 부모=0, End 부모=끝점 인덱스). 채우기 전엔 플랫폼이 0번 포인트로만 스냅된다.
- **PIE 검증 미완**: AtEnd 저장→복원 시 End 끝점 복원(C++ 스냅 없이), 라이브 Start↔End 슬라이드·같은 끝점 내 문만 개폐, 리슨 서버 2인 추종 확인.
- **다른 기믹(별도 작업)**: LaserCorridor/CheckPoint/CutsceneTrigger 의 StateTree 전환 여부 검토, 베이스 StartLogic 누락 가드.

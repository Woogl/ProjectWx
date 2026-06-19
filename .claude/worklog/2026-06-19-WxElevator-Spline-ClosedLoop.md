# WxElevator SplineComponent 닫힌 루프 기본값

## 계획

### 목표
AWxElevator의 SplineComponent를 생성자에서 닫힌 루프(closed loop)로 기본 설정한다. 에디터에서 별도 토글 없이 닫힌 형태의 경로가 기본이 되도록 한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxElevator.cpp` | 생성자 SplineComponent 생성 직후 `SetClosedLoop(true)` 한 줄 추가 | 수정 |

### 접근 방식
- **생성자 기본값**: `CreateDefaultSubobject` 직후 `SplineComponent->SetClosedLoop(true)` 호출. 이동/문 로직은 손대지 않는다.
- **알려진 한계(사용자 인지·승인)**: 닫힌 루프에서는 거리 0(Start)과 SplineLength(End)가 같은 지점이 되어 두 끝점 모델의 왕복 의미가 약해진다. 끝점/거리 로직(`SnapVisualsToState`, `BeginMoveSequence` 등)은 이번 범위에서 건드리지 않는다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxElevator.cpp` | 생성자에서 SplineComponent 생성 직후 `SetClosedLoop(true)` 추가 | 수정 |

### 구현·결정과 그 이유
- **생성자 기본값으로만 설정**: 사용자 요청이 "스플라인을 루프로"였고, 의도 확인 결과 이동 로직 변경 없이 생성자 기본값만 원함. 최소 변경으로 끝점/거리 로직은 그대로 두었다.

### 계획 대비 달라진 점
- 계획대로

### 후속 과제
- 끝점 일치(Start==End) 문제: 닫힌 루프 의미를 실제 이동에 살리려면 끝점/거리 모델 재설계 필요. 현 범위에서는 보류(사용자 인지).

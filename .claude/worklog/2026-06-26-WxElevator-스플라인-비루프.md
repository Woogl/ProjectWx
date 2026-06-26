# WxElevator 스플라인 비루프(Closed Loop 해제)

## 계획

### 목표
`AWxElevator` 의 SplineComponent 를 닫힌 루프가 아니게(열린 경로) 바꾼다. 2026-06-21 작업에서 에셋 재검증 회피 차 유지하던 `SetClosedLoop(true)` 결정을 되돌린다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Private/Gimmick/WxElevator.cpp` | 생성자 `SetClosedLoop(true)` → `false`, 주석을 비루프로 정정 | 수정 |
| `Plugins/WxWorld/.../Public/Gimmick/WxElevator.h` | 클래스 doc 의 "닫힌 루프" 언급 2곳 정정 | 수정 |

### 접근 방식
- **CDO 한 줄 변경으로 충분**: BP_Elevator 스냅샷의 SplineComponent delta 에 `bClosedLoop` 오버라이드가 없어 BP 는 플래그를 C++ CDO 에서 상속한다. 생성자 `true`→`false` 면 BP 인스턴스까지 비루프가 되고, 스플라인 포인트 형상(SplineCurves delta)은 보존된다.
- **기능 무영향**: 두 끝점 거리(0, 400)는 폐합 세그먼트와 무관해 동일. `Wx Component Spline Move`(끝점 거리만 목표)·`Wx At Spline Point`(컨트롤 포인트 nearest) 모두 폐합 구간 미사용이라 이동·판정 불변. End↔Start 는 단일 세그먼트를 정/역방향 주파.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxElevator.cpp` | 생성자 `SetClosedLoop(true)` → `false`, 주석을 "열린 경로"로 정정 | 수정 |
| `WxElevator.h` | 클래스 doc 의 "닫힌 루프" 언급 2곳(개요·플랫폼 이동 설명)을 열린 경로 기준으로 정정 | 수정 |
| `WxGimmickStateTreeNodes.cpp` | `EnableInteraction::GetDescription` 의 빌드 차단 수정(원시 `TEXT()`→`INVTEXT()`) | 수정(부수) |

### 구현·결정과 그 이유
- **CDO 한 줄로 충분**: BP delta 에 `bClosedLoop` 오버라이드가 없어 플래그를 CDO 에서 상속하므로, 생성자 변경만으로 BP 인스턴스까지 비루프가 된다. 스플라인 포인트 형상은 BP delta 가 보존.
- **기능 무영향 확인**: 끝점 거리(0, 400)는 폐합 세그먼트와 무관해 그대로라, Spline Move 이동·AtSplinePoint 판정 모두 불변. doc 만 실제(열린 경로)에 맞게 정정.
- **부수 빌드 수정**: 직전 외부 편집이 `FText::Format` 에 원시 `const TCHAR*` 를 넘겨 컴파일을 막고 있었다. "true"/"false" 표시 의도를 보존하며 `INVTEXT` 로 감싸 해소(의도 유지, 단순 유효화).
- **검증**: WxEditor(Development) 빌드 `Result: Succeeded`. 경고는 변경과 무관한 엔진 C4996 뿐.

### 계획 대비 달라진 점
- 계획 외로 `WxGimmickStateTreeNodes.cpp` 의 기존 빌드 차단 1건을 함께 고쳤다(검증을 위한 그린 빌드 확보).

### 후속 과제
- **에디터 확인(사용자, 선택)**: BP_Elevator/배치 인스턴스에서 스플라인이 열린 경로로 보이는지, BP 저장 시 스냅샷 갱신 확인. 레벨 인스턴스가 `bClosedLoop` 를 수동 오버라이드했다면 개별 해제(통상 없음).

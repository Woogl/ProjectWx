# InputData_KMB — 마우스 입력 추가

## 계획

### 목표
`InputData_KMB`의 `InputBrushDataMap`에 누락된 마우스 입력 11종을 추가한다(키보드 101종은 그대로 유지). 앞선 키보드 작업과 동일 방식·동일 32×32.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Content/UI/InputData/InputData_KMB.uasset` | `InputBrushDataMap`에 마우스 키 11종 추가(101→112, 전부 32×32) | 수정 |

### 범위 — 마우스 키 11종 (EKeys → 텍스처)
- 버튼 5: `LeftMouseButton`·`RightMouseButton`·`MiddleMouseButton`·`ThumbMouseButton`·`ThumbMouseButton2` (동명 텍스처)
- 휠 3: `MouseWheelAxis`(동명), `MouseScrollUp`→`MouseWheelUp`, `MouseScrollDown`→`MouseWheelDown`
- 축 3: `MouseX`·`MouseY`(동명), `Mouse2D`→`MouseXY2D-Axis`

키 이름은 UE 5.8 `InputCoreTypes.cpp` `EKeys`와 대조, 텍스처는 `MouseAndKeyboard/` 실제 파일로 확인 완료(11종 모두 존재).

### 접근 방식
- MCP `ProgrammaticToolset`으로 현재 배열을 읽어 기존 101종은 그대로 두고 마우스 11종을 append(이미 있으면 스킵), `set_properties`로 clear→fill 후 `save_assets`.
- 기존 키보드 항목을 재생성하지 않고 읽은 값 그대로 보존해 수동 변경분이 있어도 안전.

### 검증
되읽어 112개·마우스 11종 존재·해당 `resourceObject` non-null·`imageSize` 32×32 확인, `is_dirty` false 확인.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Content/UI/InputData/InputData_KMB.uasset` | `InputBrushDataMap`에 마우스 11종 추가(101→112, 전부 32×32) | 수정 |
| `Plugins/WxBlueprintSnapshot/Snapshots/Game/UI/InputData/InputData_KMB.json` | 저장 시 자동 갱신(112종) | 수정(자동) |

### 구현·결정과 그 이유
- **기존 101종 보존 append**: 현재 배열을 읽어 그대로 두고 마우스 11종만 이어붙임(이미 있으면 스킵). 키보드 항목 재생성 없이 수동 변경분까지 안전.
- **배열 세터 제약 회피**: 기존이 있어 clear→fill(비우기→채우기)로 처리.
- **키↔텍스처 대응**: 대부분 동명, 예외는 `MouseScrollUp/Down`→`MouseWheelUp/Down`, `Mouse2D`→`MouseXY2D-Axis`. 전 텍스처 실제 파일 확인.

### 계획 대비 달라진 점
- 계획대로.

### 검증 결과
- 라운드트립: before=101, added=11, after=112, 마우스 11종 존재, 텍스처 해석 실패 0, `imageSize` 전부 32×32.
- `is_dirty=false`(저장 확인), 스냅샷 112종·마우스 11종 반영 확인.

### 후속 과제
- 없음.

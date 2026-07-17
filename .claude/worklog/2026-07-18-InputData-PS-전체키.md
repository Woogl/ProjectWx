# InputData_PS — 플레이스테이션 키맵핑 채우기

## 계획

### 목표
`InputData_PS`(`UCommonInputBaseControllerData`)를 PlayStation 스타일 아이콘으로 채운다. 현재는 `InputData_Gamepad`(Xbox)의 복제본이라 스타일 종속 16개가 `Gamepad_X`/`T_X_` 텍스처를 가리킨다. 이를 `Gamepad_P`/`T_P_`로 교체해 PS 아이콘으로 만든다. 앞선 게임패드 작업과 동일 32키·동일 32×32.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Content/UI/InputData/InputData_PS.uasset` | `InputBrushDataMap` 32키를 PS 아이콘으로 재설정(스타일 종속 16 → `Gamepad_P`) | 수정 |

### 접근 방식
- **스타일 종속 16 → PS**: 페이스버튼 4·숄더 2·트리거 4·D-패드 4·스페셜 2를 `Gamepad_P`의 `T_P_*` 텍스처로. (PS 페이스버튼 텍스처가 ✕/○/□/△를 Bottom/Right/Left/Top 위치에 이미 담고 있어 키→텍스처 구조는 Xbox와 동일, 폴더·프리픽스만 치환.)
- **썸스틱 16(공용) 유지**: `GamepadThumbsticks`는 스타일 무관이라 그대로.
- **동일 파이프라인**: MCP `ProgrammaticToolset`으로 32키 배열 생성 → `set_properties`(clear→fill) → `save_assets`.
- **GamepadName**: 현재 "None". CommonInput이 PS 패드에 이 데이터를 실제로 선택하려면 적절한 GamepadName + `DefaultGame.ini` 등록이 필요하지만, 이번 요청 범위(아이콘 채우기) 밖이라 건드리지 않고 후속 과제로 남김.

### 검증
`get_properties` 라운드트립으로 32개·`resourceObject` non-null·`imageSize` 32×32 확인, 스타일 종속 16개가 전부 `Gamepad_P` 경로인지 확인, `is_dirty` false 확인.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Content/UI/InputData/InputData_PS.uasset` | `InputBrushDataMap` 32키를 PS 아이콘으로 재설정(스타일 종속 16 = `Gamepad_P`, 썸스틱 16 = 공용) | 수정 |

### 구현·결정과 그 이유
- **복제본 위에 전체 재설정**: PS 애셋은 Xbox 데이터의 복제본이었음(스타일 종속 16개가 `T_X_`를 가리킴). 부분 치환 대신 32키 전체를 재구성해 clear→fill로 덮어써 잔존 Xbox 참조 가능성을 원천 제거.
- **PS 페이스버튼은 위치 매핑 유지**: `Gamepad_P`의 Bottom/Right/Left/Top 텍스처가 이미 ✕/○/□/△를 담고 있어 키→텍스처 구조는 Xbox와 동일, 폴더·프리픽스만 치환.
- **GamepadName 미변경**: 요청 범위(아이콘 채우기) 밖. 후속 과제 참조.

### 계획 대비 달라진 점
- 계획대로.

### 검증 결과
- 라운드트립: count=32, PS 텍스처 16, Xbox 잔존 0, 텍스처 해석 실패 0, `imageSize` 전부 32×32, 중복 0, 기대 키 집합 일치.
- `is_dirty=false`(저장 확인).

### 후속 과제
- **CommonInput 연결**: 이 애셋이 실제 PS 패드에 선택되려면 (1) `GamepadName`을 PS 패드 식별자로 설정, (2) `Config/DefaultGame.ini`의 `[CommonInputPlatformSettings...]`에 `+ControllerData=.../InputData_PS.InputData_PS_C` 등록 필요. 현재는 `InputData_KMB`·`InputData_Gamepad`만 등록됨. `InputData_XBOX`도 같은 등록 필요.

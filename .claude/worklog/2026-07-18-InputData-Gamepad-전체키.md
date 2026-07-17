# InputData_Gamepad — 게임패드 전체 키 채우기

## 계획

### 목표
`InputData_Gamepad`(`UCommonInputBaseControllerData`, 현재 `InputBrushDataMap` 비어 있음, `GamepadName`="None" 범용 패드)를 게임패드 키 전체로 채운다. 앞선 `InputData_KMB`와 동일 방식·동일 32×32.

### 스타일 결정
`GamepadName`="None"(범용) 슬롯의 표준 관례는 Xbox 레이아웃 → **Xbox(`Gamepad_X`)** 아이콘 사용. 썸스틱은 스타일 무관 공용 폴더(`GamepadThumbsticks`) 사용. (P/S 스타일로 교체 시 텍스처 폴더/프리픽스만 치환하면 됨.)

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Content/UI/InputData/InputData_Gamepad.uasset` | `InputBrushDataMap`을 게임패드 키 32종으로 채움(모두 32×32) | 수정 |

### 범위 — 게임패드 키 32종
- **Xbox 스타일 16**: 페이스버튼 4(`Gamepad_FaceButton_Bottom/Right/Left/Top`), 숄더 2, 트리거 4(`LeftTrigger`+`LeftTriggerAxis`, Right 동일 → 트리거 텍스처 공용), D-패드 4, 스페셜 2(`Special_Left/Right`).
- **썸스틱(공용) 16**: 스틱 클릭 2(`Left/RightThumbstick`), 2D축 2(`Left2D/Right2D`), X/Y축 4(`LeftX/LeftY/RightX/RightY`), 방향 8(`Left/RightStick_Up/Down/Left/Right`).
- **제외**: `Gamepad_Special_Left_X/Y/Touched`(PS 터치패드, 텍스처 없음).

### 접근 방식
`InputData_KMB`와 동일: 실행 중 에디터 MCP `ProgrammaticToolset`으로 정상 브러시 템플릿(32×32) 복제해 32개 원소 생성 → `ObjectTools.set_properties`로 `InputBrushDataMap` 설정(현재 빈 배열 → 순수 추가라 2단계 불필요) → `AssetTools.save_assets` 저장. 키 이름은 UE 5.8 `EKeys`(`Gamepad_*`)와 실제 텍스처 파일명을 전수 대조.

### 검증
`get_properties` 라운드트립으로 32개·`resourceObject` non-null·`imageSize` 32×32 확인, `is_dirty` false 확인. (`PreSave`가 이미지 없는 항목을 삭제하므로 저장 후 32개 유지 = 전 텍스처 정상 해석의 재확인.)

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Content/UI/InputData/InputData_Gamepad.uasset` | `InputBrushDataMap`을 게임패드 키 32종으로 채움(전 항목 32×32) | 수정 |

### 구현·결정과 그 이유
- **Xbox 스타일 채택**: 범용 패드(`GamepadName`="None") 슬롯이라 UE 관례대로 Xbox 레이아웃 아이콘 사용. 페이스버튼 위치(Bottom=A/Right=B/Left=X/Top=Y)가 이 스타일과 일치.
- **트리거 텍스처 공용**: `Gamepad_LeftTrigger`(디지털)와 `Gamepad_LeftTriggerAxis`(아날로그) 둘 다 같은 트리거 텍스처에 매핑(Right 동일). EasyInputPrompts엔 트리거 아이콘이 하나뿐.
- **썸스틱은 공용 폴더**: X/P/S 스타일 무관하게 `GamepadThumbsticks` 사용(스틱 클릭·2D·X/Y축·8방향).
- **KMB와 동일 파이프라인**: MCP `set_properties` 일괄 주입 + `save_assets`. 빈 배열이었지만 안전하게 비우기→채우기 2단계로 처리.

### 계획 대비 달라진 점
- 계획대로.

### 검증 결과
- 라운드트립: count=32, 텍스처 해석 실패 0, `imageSize` 전부 32×32, 중복 0, 기대 키 집합과 정확히 일치.
- `is_dirty=false`(저장 확인), `.uasset` 5.5KB→58KB.

### 후속 과제
- 없음. PS/Switch용 표시가 필요하면 별도 `InputData_Gamepad_PS/_Switch` 애셋에 `Gamepad_P`/`Gamepad_S` 텍스처로 같은 매핑을 적용하면 된다(썸스틱은 공용 재사용).

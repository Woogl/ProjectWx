# InputData 게임패드 3종 CommonInput 배선 (Xbox 기본, PS/XBOX 등록)

## 계획

### 목표
게임패드 InputData 3종(Gamepad·XBOX·PS)을 CommonInput에 배선한다. Xbox를 기본 표시로 유지하고 PS·XBOX를 구분 이름으로 등록한다. 현재 3종 모두 `GamepadName`="None"이라, 런타임 요청 이름("Generic")과 안 맞아 게임패드 아이콘이 해석되지 않던 문제도 함께 해결한다.

### 배경(조사 결과)
- 매칭(`GetControllerDataForInputType`)은 게임패드의 경우 `GamepadName` **정확 일치** 요구.
- 런타임 아이콘 조회는 스톡 `UCommonActionWidget::GetIcon`(→ `GetCurrentGamepadName()`, 기본 "Generic") 경로. `WxActionWidget`의 커스텀 분기는 **디자인타임 프리뷰 전용**(`GetCurrentPlatformDefaults`=Generic)이라 런타임 무관.
- 따라서 기본 표시 애셋은 `GamepadName`="Generic"이어야 하고, 스타일 전환은 향후 `SetGamepadInputType` 호출로 요청 이름을 바꾸면 된다(코드 변경 불필요).

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Content/UI/InputData/InputData_Gamepad.uasset` | `GamepadName` None→"Generic" (기본/폴백, Xbox 아트) | 수정 |
| `Content/UI/InputData/InputData_XBOX.uasset` | `GamepadName` None→"Xbox" (등록, 대기) | 수정 |
| `Content/UI/InputData/InputData_PS.uasset` | `GamepadName` None→"PS" (등록, 대기) | 수정 |
| `Config/DefaultGame.ini` | Windows 플랫폼 섹션에 `+ControllerData` 2줄(XBOX·PS) 추가 | 수정 |

### 접근 방식
- MCP `set_properties`로 3종 `GamepadName` 지정 후 `save_assets`.
- `DefaultGame.ini`의 `[CommonInputPlatformSettings_Windows ...]` 기존 `+ControllerData`(KMB·Gamepad) 아래에 `InputData_XBOX_C`·`InputData_PS_C` 2줄 추가.
- 코드 변경 없음(스톡 런타임 경로가 이미 런타임 게임패드 이름을 존중).

### 검증
3종 `GamepadName` 되읽기 확인, `DefaultGame.ini` 2줄 추가 확인. (ini는 에디터 재시작 후 적용.)

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Content/UI/InputData/InputData_Xbox.uasset` | `GamepadName` → "Generic" (기본/폴백 게임패드, Xbox 아트) | 수정 |
| `Content/UI/InputData/InputData_PS.uasset` | `GamepadName` → "PS" (등록, 대기) | 수정 |
| `Config/DefaultGame.ini` | Windows 섹션 `+ControllerData`를 KMB + `InputData_Xbox_C` + `InputData_PS_C`로 (리다이렉터 참조 제거) | 수정 |

### 계획 대비 달라진 점 (중요)
- **`InputData_Gamepad`는 실제 애셋이 아니라 리다이렉터였음** → 대상은 `InputData_Xbox`(정식명, 클래스 `InputData_Xbox_C`). 유저가 Gamepad를 Xbox로 리네임한 결과. 그래서:
  - 계획의 "Gamepad=Generic, XBOX=Xbox"를 접고, **실제 Xbox 애셋에 "Generic"** 을 줘 기본 게임패드로 삼음(라벨은 Xbox지만 GamepadName=Generic = "기본 게임패드 룩은 Xbox").
  - ini의 `InputData_Gamepad.InputData_Gamepad_C`(리다이렉터) 참조를 **실제 `InputData_Xbox.InputData_Xbox_C`로 교체**(향후 Fix Up Redirectors로 리다이렉터가 정리돼도 config가 안 깨지게).
- `DefaultGamepadName=Generic`은 유지("기본 유지").

### 구현·결정과 그 이유
- **기본 표시 애셋은 GamepadName="Generic"이어야**: 런타임 스톡 경로가 요청하는 이름이 기본 "Generic"이라, 여기에 맞춰야 게임패드 아이콘이 해석됨(3종 모두 "None"이던 기존 버그 해소).
- **PS는 "PS"로 대기 등록**: 코드 변경 없이 등록만. 실제 전환은 스톡 경로가 존중하는 `UCommonInputSubsystem::SetGamepadInputType("PS")` 호출로 이후 연결.

### 검증 결과
- `GamepadName` 되읽기: Xbox="Generic", PS="PS". 애셋 저장 완료.
- `DefaultGame.ini` 최종: `+ControllerData` = KMB, InputData_Xbox_C, InputData_PS_C (리다이렉터 줄 제거).

### 후속 과제
- **런타임 PS 전환**: 옵션 설정/패드 감지에서 `SetGamepadInputType("PS")` 호출(별도 작업, 옵션3에 해당).
- **리다이렉터 정리**: 완료(유저가 Fix Up Redirectors 실행) — `InputData_Gamepad` 리다이렉터 삭제됨. ini는 이미 실제 `InputData_Xbox`를 참조하므로 영향 없음(재검증: `get_class`가 `InputData_Xbox.InputData_Xbox_C`·`InputData_PS.InputData_PS_C` 반환, ini 값과 일치).
- **ini 반영**: 실행 중 에디터엔 재시작 후 적용(애셋 변경은 즉시 반영됨).

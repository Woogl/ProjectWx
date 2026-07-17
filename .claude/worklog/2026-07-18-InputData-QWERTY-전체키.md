# InputData_KMB — QWERTY 키보드 전체 키 채우기

## 계획

### 목표
`InputData_KMB`(`UCommonInputBaseControllerData`)의 `InputBrushDataMap`을 표준 QWERTY 키보드 전체 키(101종)로 채운다. 각 키를 EasyInputPrompts `T_Keyboard_*` 텍스처에 매핑하고 모든 브러시 `ImageSize`를 32×32로 통일한다. 현재는 A~N 일부만 있고 오류(D 중복·E 오배정, J만 128×128)가 섞여 있어 전체 교체로 정리한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Content/UI/InputData/InputData_KMB.uasset` | `InputBrushDataMap`을 101개 키 매핑으로 전체 교체(모두 32×32) | 수정 |

### 접근 방식
- **실행 중 에디터 MCP 브리지 사용**: 소스/바이너리 직접 편집 대신 실행 중 에디터(PID 8036)의 MCP 툴로 데이터 에셋을 조작한다.
- **키→텍스처 매핑**: UE 5.8 `InputCoreTypes.cpp`의 `EKeys` 이름과 실제 텍스처 파일 목록을 전수 대조해 확정. 넘패드/수정자 등 텍스처 이름이 키 이름과 다른 항목은 별도 매핑(예: `Zero→_0`, `LeftControl→_LeftCtrl`, `LeftCommand→_WinKey`, `Divide→_NumDivide`, `NumPadZero→_Num0`).
- **일괄 교체**: `ProgrammaticToolset.execute_tool_script`로 정상 브러시 하나를 템플릿 삼아 101개 원소를 만들고(`key`·`resourceObject`만 치환) `ObjectTools.set_properties`로 배열 전체를 한 번에 대체 → `AssetTools.save_assets`로 저장.
- **제외**: PrintScreen·메뉴키·넘패드 전용 Enter는 `EKeys`에 대응 키가 없어 제외.

### 검증
`ObjectTools.get_properties`로 되읽어 101개·모든 `resourceObject` non-null·`imageSize` 32×32 확인, `is_dirty` false 확인. C++ 변경이 없어 빌드 불필요.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Content/UI/InputData/InputData_KMB.uasset` | `InputBrushDataMap`을 QWERTY 전체 키 101종으로 교체(전 항목 32×32) | 수정 |
| `Plugins/WxBlueprintSnapshot/Snapshots/Game/UI/InputData/InputData_KMB.json` | 저장 시 자동 갱신(101종) | 수정(자동) |

### 구현·결정과 그 이유
- **실행 중 에디터 MCP로 처리**: `.uasset` 바이너리를 손대지 않고, 실행 중 에디터의 `ProgrammaticToolset`으로 정상 브러시 1개를 템플릿 삼아 101개 원소를 생성해 `ObjectTools.set_properties`로 일괄 주입 → `AssetTools.save_assets` 저장.
- **배열 세터가 diff 기반이라 2단계 교체**: 크기와 원소가 동시에 바뀌면 삽입 지점이 모호하다며 거부됨. 먼저 `[]`로 비우고(순수 삭제) 101개를 채워(순수 추가) 회피.
- **키 이름은 엔진 원본 대조**: UE 5.8 `InputCoreTypes.cpp`의 `EKeys` 이름과 실제 텍스처 파일을 전수 대조. 잘못된 FKey 이름은 라운드트립으로 잡히지 않으므로(FName으로 그냥 저장) 사전 대조가 필수였다.
- **32×32는 `FSlateBrush` 기본값**: `SlateBrushDefs::DefaultImageSize = 32.0f`. 유효 크기는 전 항목 32×32지만 델타 직렬화라 스냅샷엔 `ImageSize`가 생략됨(정상). 기존 J의 128×128 이상값과 D 중복·E 오배정도 전체 교체로 정리됨.

### 계획 대비 달라진 점
- 배열 세터의 diff 제약 때문에 「비우기 → 채우기」 2단계로 나눈 것 외 계획대로.

### 검증 결과
- `get_properties` 라운드트립: count=101, 텍스처 해석 실패 0, `imageSize` 전부 32×32, 키 중복 0, 기대 키 집합과 정확히 일치.
- `is_dirty=false`(저장 확인), 스냅샷 JSON 101종으로 재생성 확인.

### 후속 과제
- 없음. (필요 시 마우스 버튼·휠 등 KMB의 마우스 항목도 같은 방식으로 추가 가능.)

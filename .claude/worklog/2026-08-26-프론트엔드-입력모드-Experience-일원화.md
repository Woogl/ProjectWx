# 프론트엔드 입력 모드 처리를 Experience 모듈 경로로 일원화

## 계획

### 목표
`WBP_FrontEnd`가 Construct/Destruct에서 직접 `SetInputModeUIOnly`/`SetInputModeGameAndUI`를 호출하던 처리를 걷어내고, 입력 모드를 WxExperience → HUD 컴포넌트 → CommonUI 경로로 완전히 모듈식으로 처리되게 한다. 레벨 전환 시 남던 InputMode 찌꺼기를 이 경로가 스스로 해소한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Content/UI/Widget/WBP_FrontEnd.uasset` | `Event Construct`·`Event Destruct` 이벤트 체인 제거 (PreConstruct·OnClicked 2개는 유지) | 수정 |

### 접근 방식
- **이미 서 있는 모듈식 경로에 위임**: `WBP_FrontEnd`는 부모가 `WxHUDLayout`이고 `InputMode=Menu`·`bSupportsActivationFocus=true`라, CommonUI가 이 위젯을 레이어에 올리면 `GetDesiredInputConfig()`가 `FUIInputConfig(Menu, NoCapture)`를 자동 적용한다. 이 위젯을 올리는 주체는 `EXP_FrontEnd → WAS_FrontEnd`가 주입한 `WxHUDComponent`이며, Experience가 발행한 `gameHUDClass=WBP_FrontEnd_C`를 push한다. 따라서 WBP의 `SetInputMode` 호출은 이미 중복이자 CommonUI 우회이므로 제거한다.
- **찌꺼기 근본 원인은 해소 상태**: 과거 찌꺼기는 HUD의 `bSupportsActivationFocus=false`로 CommonUI 설정이 조용히 미적용되던 문제였고, 현재 두 HUD 모두 `true`라 라이브 PIE 전환 로그에서 Menu↔Game 전환·`Previous(None)` 리셋이 확인된다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Content/UI/Widget/WBP_FrontEnd.uasset` | EventGraph에서 `Event Construct`·`Event Destruct` 체인(SetInputModeUIOnly/GameAndUI + GetPlayerController + Self) 6개 노드 삭제, 컴파일·저장 | 수정 |

### 구현·결정과 그 이유
- **중복 제거로 끝난 이유**: 입력 모드는 이미 CommonUI 액티베이터블 경로가 발행한다. `WBP_FrontEnd`가 `WxHUDLayout(InputMode=Menu)`라, Experience가 `WxHUDComponent`로 이 위젯을 레이어에 올리면 `GetDesiredInputConfig()`가 `Menu, NoCapture`를 자동 적용한다. WBP의 `SetInputMode` 호출은 CommonUI의 `ActiveInputConfig`를 우회하는 중복이었을 뿐이라, 지우기만 하면 모듈식 경로만 남는다.
- **찌꺼기가 안 남는 근거**: 세계가 내려갈 때 CommonUI가 `ActiveInputConfig`를 스스로 리셋하고, 새 세계에서 HUD를 다시 push하며 config를 새로 적용한다. 검증 로그에서 전환마다 `Previous (None)`으로 리셋된 뒤 새 모드가 실림을 확인했다.
- **삭제 수단**: `write_graph_dsl`(그래프 전체 재기입) 대신 대상이 명확한 `delete_node`로 6개 노드만 제거했다. 두 체인이 `GetPlayerController` 노드를 공유하고 있어 함께 지웠다.

### 검증
라이브 PIE(LV_DevCombat 시작)에서 `LogUIActionRouter` 로그로 확인:
- `open LV_FrontEnd` → `WBP_FrontEnd_C_0` leaf 기준 `New (ECommonInputMode::Menu)` 적용(블루프린트 노드 없이 CommonUI 경로).
- 프론트엔드 `LV_DevCombat` 버튼 **마우스 클릭** 성공(→ OpenLevel 발동) → `WBP_GameHUD_C_0` 기준 `New (ECommonInputMode::Game)` 적용.
- 두 전환 모두 `Previous (None)` 리셋으로 찌꺼기 없음. 마우스 클릭이 되는 것으로 Menu 설정(NoCapture·커서 표시)이 버튼 조작을 허용함도 확인.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 게임패드/키보드 메뉴 내비게이션이 필요해지면, Construct 훅이 아니라 `WxHUDLayout`에 `GetDesiredFocusTarget` 오버라이드로 포커스를 주는 것이 모듈식 방향(이번 범위 밖).

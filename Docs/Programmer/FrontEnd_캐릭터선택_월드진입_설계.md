# FrontEnd 캐릭터 선택과 레벨 진입

작성일: 2026-09-05. 싱글플레이 우선. 인벤토리와 로딩 UI는 이번 작업 범위에서 제외한다.

## 현재 구조

기존 WBP_FrontEnd가 목록과 임시 선택을 관리하고, 선택한 Pawn 클래스와 맵을 라이브러리에 직접 전달한다. 별도 캐릭터·목적지 Definition이나 카탈로그는 사용하지 않는다.

| 구성 | 역할 |
| --- | --- |
| WBP_FrontEnd | 목록 기본값, 선택 인덱스, WidgetSwitcher 페이지, 확인 팝업 결과 처리 |
| UWxFrontEndLibrary | RequestNewGame, GetTravelStatus 두 Blueprint 진입점 |
| UWxGameFlowSubsystem | 요청 검증, Pawn 클래스 비동기 준비, 맵 전환, 도착 준비, 실패 복구 |
| AWxGameMode | 도착 맵 Experience 해석과 선택 Pawn 생성 |

새 ScreenWidget, FrontEnd 컴포넌트, 전용 ViewModel, Resolver는 없다. 기존 UWxHUDLayout 부모와 WBP_FrontEnd 에셋을 사용한다.

## 입장 API

```cpp
static bool RequestNewGame(
    const UObject* WorldContextObject,
    TSoftClassPtr<APawn> PawnClass,
    TSoftObjectPtr<UWorld> Level);
```

반환값 true는 요청 접수이며 도착 완료를 뜻하지 않는다. 라이브러리는 목록이나 현재 선택을 소유하지 않는다. GetTravelStatus는 전환 중 여부와 오류 문구를 제공한다.

WBP의 CharacterOptions와 DestinationOptions 기본값에 표시 정보와 소프트 참조를 작성한다. FWxFrontEndOption은 Title, Description, PawnClass, Level만 있는 목록용 값 구조체다. 캐릭터 항목에는 PawnClass를, 레벨 항목에는 Level을 지정한다. 별도 데이터 에셋은 만들지 않는다.

현재 목록은 BP_Player 1종, LV_DevCombat·LV_OpenWorld 2개다. UI 이름은 Player, Combat Test, Open World이며 메뉴와 확인 문구는 영어다. 목록을 열 때 Pawn 클래스나 맵 전체를 준비하지 않고, 확정한 Pawn 클래스만 입장 전에 비동기 로드한다.

## 선택과 전환

- WBP의 PageSwitcher는 Main, SelectCharacter, SelectLevel 순서의 세 페이지다. 기존 ProjectTitle과 WBP_Button을 유지한다.
- Main의 NewGame은 ShowCharacters로 이동하고, QuitGame은 엔진 QuitGame을 호출한다. 캐릭터 클릭 후 ShowLevels로 이동한다.
- 레벨 클릭 시 기존 ShowConfirmationPopup(OkCancel)에 선택한 두 항목의 Title과 시작 질문을 전달한다. PopupOpen 동안 버튼과 중복 요청을 차단한다.
- HandleStartResult에서 확인한 경우에만 RequestNewGame을 호출한다. 취소하면 ShowMain이 두 인덱스를 -1로 초기화하고 메인으로 복귀한다.
- 각 페이지에서 FocusTarget을 갱신하고 사용자 포커스를 지정한다. BP_GetDesiredFocusTarget도 이를 반환하므로 팝업 종료 후 현재 페이지로 포커스가 복귀한다.
- 입장 요청 직후 중복 입력을 막는다. 전환 중에만 HandleRefreshStatus를 0.1초 단발 타이머로 재호출하며, Destruct에서 타이머를 해제한다.
- GameFlow는 싱글플레이 요청인지, 참조가 유효한지, 맵 패키지가 있는지 확인한다. 현재 맵으로의 신규 입장은 거부한다. 별도의 FrontEnd 맵 이름 제한은 없다.
- 요청 당시 출발 맵의 패키지 이름을 기억해 실패 시 복귀한다. PIE 접두사는 제거하며, FrontEndSettings나 ini의 복귀 맵 지정은 사용하지 않는다. 전환 제한 시간은 GameFlow 내부 상수 120초다.
- 선택 Pawn 클래스 준비 후 OpenLevel을 실행한다. Experience URL 옵션을 덮어쓰지 않고 도착 맵의 기존 WorldSettings 설정을 사용한다.
- 도착 맵에 유효한 선택 Pawn이 있으면 Experience 기본 Pawn보다 우선한다. 별도의 선택 허용 플래그는 없다. 맵·Pawn·Experience의 잘못된 구성은 실패로 처리한다.
- Experience·Pawn·HUD·World Partition 준비를 확인한 뒤 입력을 복원한다. 지형 준비 전에 Pawn이 떨어지지 않도록 기존 도착 준비 처리를 유지한다.
- PlayerStart는 엔진의 기존 선택 경로를 사용한다. 별도 진입점 태그 기능은 현재 제공하지 않는다.

두 게임플레이 맵의 WorldSettings에는 기존 EXP_Combat이 지정되어 있다. 직접 PIE로 맵을 실행해 선택 정보가 없는 경우에는 기존 Experience.DefaultPawnClass를 사용한다.

## 후속 확장

확정된 Pawn 클래스와 맵 참조는 GameInstance 수명의 RunState에 보관한다. 이는 디스크 저장 기능이 아니다. SaveGame을 도입할 때 식별자·버전·상태 수집과 복원을 설계한다. 현재 메뉴의 배열 인덱스를 저장 식별자로 사용하지 않는다.

게임 중 다른 맵 이동, 같은 오픈월드의 빠른 이동, 특정 진입점과 저장 위치 복원은 후속 작업이다. 필요할 때 전환 요청에 진입 위치와 복원 데이터를 추가한다. 일반적인 오픈월드 이동은 World Partition 스트리밍을 사용한다.

로딩 UI, 인벤토리, 멀티플레이 기능은 추가하지 않는다.

## 검증 범위

UE 5.8 WxEditor Win64 Development 빌드와 WBP 컴파일을 확인했다. RenderOffscreen PIE에서 세 페이지 순서, 팝업 이름·질문, 중복 차단, 취소 시 Main 복귀와 선택 초기화, 확인 후 두 맵 입장·입력 복원, QuitGame의 PIE 종료를 확인했다. 영어 팝업 화면을 캡처해 표시도 검토했다. 테스트는 CommonUI HandleButtonClicked 경로를 호출하며 물리 마우스와 패키지 Cook는 별도다.

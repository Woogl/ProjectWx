# FrontEnd 프레젠테이션 로직 WBP 이전

## 계획

- 사용자 승인 및 부모 클래스 수정 요청에 따라 WBP_FrontEnd의 부모를 WxActivatableWidget으로 변경한다.
- 버튼·스위처·포커스·확인 팝업·상태 표시를 WBP 이벤트 그래프로 이전하고 디자인과 선택 데이터를 보존한다.
- WBP는 FrontEndLibrary의 RequestNewGame과 GetTravelStatus API를 사용한다. 게임 시작 검증·선택 폰 보존·맵 이동은 기존 비즈니스 경로를 유지한다.
- WxFrontEndWidget을 삭제하고 GameFlowSubsystem의 불필요해진 화면 제어 상태·함수를 제거한다.
- 부모 변경에 필요한 WxPlayerLayoutComponent의 LayoutClass 타입을 WxActivatableWidget으로 넓혀 기존 HUD와 프론트엔드 메뉴를 모두 수용한다.
- 블루프린트 컴파일, 선택·취소·게임 시작·오류 복구 흐름과 UE 5.8 WxEditor Development 빌드를 검증한다.

## 완료

- 후속 사용자 지시에 따라 LayoutClass 타입 확장은 취소했다. 최종 구조는 WxHUDLayout 전용 컴포넌트와 BP_FrontEndPlayerController의 직접 Menu Push이며, 후속 기록은 2026-09-13-FrontEnd-컨트롤러-메뉴-Push.md를 참조한다.
- WxFrontEndWidget.h/.cpp를 삭제했다. 새 엔진 프로세스에서 /Script/WxGame.WxFrontEndWidget 객체가 존재하지 않음을 확인했다.
- WBP_FrontEnd의 부모를 WxActivatableWidget으로 변경했다. 이전 이벤트 그래프를 복원하고 버튼·페이지·포커스·팝업·문구·활성화를 WBP가 담당하도록 했다. EventGraph 154개 노드와 BP_GetDesiredFocusTarget 오버라이드를 사용한다.
- WBP는 FrontEndLibrary.RequestNewGame으로 선택한 PawnClass/Level을 전달하고 GetTravelStatus로 이동 상태를 조회한다. 이동 중에만 0.1초 단발 타이머로 상태를 재확인하고 Destruct에서 타이머와 팝업 대기 상태를 정리한다. 선택 인덱스 범위 검사와 중복 입력·늦은 팝업 콜백 방지를 검증했다.
- GameFlowSubsystem에서 화면 단계, 임시 선택 옵션, 확인 문구와 화면 변경 이벤트를 제거했다. 요청 검증, 체크포인트 초기화, 맵 이동, 도착 폰 보존, 이동 실패 처리는 유지했다.
- WxPlayerLayoutComponent.LayoutClass를 TSoftClassPtr<UWxActivatableWidget>으로 변경했다. 프론트엔드 컨트롤러 BP 컴파일과 실제 화면 생성에 성공했다.
- 디자인·옵션·입력 설정 보존: 이전/이후 위젯 및 슬롯의 읽기 가능한 속성, 캐릭터 2개·목적지 2개 옵션(FText 식별자 포함), InputMode 등 설정의 동일성을 확인했다. Saved/FrontEndWbpBefore.json, Saved/FrontEndWbpAfter.json, Saved/FrontEndWbpStructureResults.json.
- UE 5.8.2 WxEditor Win64 Development 빌드 성공(종료 코드 0): Saved/Logs/BuildDoctor/build_2026-09-13_191808_638_3320.log.
- 저장된 WBP와 BP_FrontEndPlayerController를 새 프로세스에서 경고를 오류로 취급하여 컴파일했다. Saved/Logs/FrontEndWbpStructure.log의 WX_FRONTEND_WBP_STRUCTURE_SUCCESS.
- NullRHI PIE에서 실제 CommonUI 버튼 클릭으로 페이지·포커스, 팝업 취소, 늦은 확인 콜백 무시, 잘못된 인덱스 무시, 요청 거절 즉시 문구 표시와 재시도, 이동 중 중복 입력 차단, Template Player → LV_DevCombat, HGTest → LV_OpenWorld, 메뉴 복귀 후 상태 갱신 및 Quit 종료를 검증했다. Saved/FrontEndWbpRuntimeResults.json = SUCCESS.
- 요청 거절 검증은 메모리에서만 DestinationOptions의 인스턴스 편집 플래그를 열고 잘못된 옵션을 주입했다. 테스트용 변경은 저장하지 않았다. 실제 엔진 TravelFailure 강제 재현은 하지 않았다.
- PIE 중 기존 보스 네임플레이트 VM_BossCharacter_Character_AbilitySystem 초기화 오류를 관찰했다. 프론트엔드 검증은 완료됐으며 해당 코드는 이번 작업에서 수정하지 않았다.
- 변경 전 에셋 백업: Saved/WBP_FrontEnd-before-removal.uasset. 이전·구조·PIE 검증 스크립트는 Saved/MigrateFrontEndToWbp.py, Saved/VerifyFrontEndWbpStructure.py, Saved/VerifyFrontEndWbpRuntime.py에 보관했다.

# FrontEnd 컨트롤러에서 메뉴 Push

## 계획

- 사용자의 후속 수정 지시에 따라 WxPlayerLayoutComponent.LayoutClass를 WxHUDLayout 전용으로 복원한다.
- BP_FrontEndPlayerController의 LayoutClass를 비우고 로컬 BeginPlay에서 기존 PushWidgetToLayer API로 WBP_FrontEnd를 UI.Layer.Menu에 Push한다.
- WBP_FrontEnd의 WxActivatableWidget 부모와 이벤트 그래프 프레젠테이션 로직을 유지한다.
- UI 레이아웃 생성 시점과 컨트롤러 교체 시 수명 처리를 확인하고, UE 5.8 WxEditor Development 빌드·BP 컴파일·PIE 메뉴 표시와 맵 왕복을 검증한다.

## 완료

- WxPlayerLayoutComponent.LayoutClass를 TSoftClassPtr<UWxHUDLayout>으로 복원했다. cpp의 include도 원복했으며 컴포넌트는 전투 HUD 전용으로 유지한다.
- BP_FrontEndPlayerController의 PlayerLayoutComponent.LayoutClass를 None으로 저장했다. BeginPlay → IsLocalController → PushWidgetToLayer(WBP_FrontEnd, UI.Layer.Menu)로 연결했다. 새 C++ 컨트롤러·위젯 클래스는 추가하지 않았다.
- WBP_FrontEnd의 WxActivatableWidget 부모·이벤트 그래프·디자인·옵션은 유지했다. 공용 컨트롤러 및 레이아웃 컴포넌트의 주석만 최종 역할에 맞게 정정했다.
- 컨트롤러 교체 시 UIManager가 기존 PrimaryGameLayout을 제거하고 새 컨트롤러용 레이아웃을 만든다. 비동기 Push는 기존 API의 TargetLayout 및 WorldContextObject 유효성 검사를 사용한다.
- UE 5.8.2 WxEditor Win64 Development 빌드 성공(종료 코드 0): Saved/Logs/BuildDoctor/build_2026-09-13_193429_184_12892.log.
- 새 프로세스에서 프론트엔드 컨트롤러와 WBP를 경고를 오류로 취급해 컴파일했고, 저장된 LayoutClass가 None인지 확인했다.
- NullRHI PIE에서 활성 FrontEnd 메뉴가 하나이고 Menu 레이어에 있으며 Game 레이어는 비어 있음을 검증했다. 선택·포커스·취소·늦은 콜백·요청 거절 후 복구·중복 입력 차단, Template Player → LV_DevCombat, HGTest → LV_OpenWorld, 두 차례 메뉴 복귀와 Quit 종료를 통과했다. Saved/FrontEndControllerPushRuntimeResults.json = SUCCESS.
- WBP_FrontEnd 클래스가 PIE 시작 전에 로드되지 않은 새 프로세스에서도 비동기 Push가 로컬 컨트롤러 소유의 메뉴 하나를 Menu 레이어에 생성했고 NewGame 버튼이 작동했다. Saved/FrontEndControllerColdPushResults.json = SUCCESS.
- 기존 보스 네임플레이트 VM_BossCharacter_Character_AbilitySystem 초기화 오류는 이번 PIE에서도 관찰됐다. 해당 병행 작업 코드는 수정하지 않았다.
- 백업: Saved/BP_FrontEndPlayerController-before-menu-push.uasset. 이전 및 검증 스크립트: Saved/MigrateFrontEndControllerPush.py, Saved/VerifyFrontEndControllerPushRuntime.py, Saved/VerifyFrontEndControllerColdPush.py.

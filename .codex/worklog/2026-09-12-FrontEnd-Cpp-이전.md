# FrontEnd 블루프린트 로직 C++ 이전

## 계획

- WxGameFlowSubsystem으로 선택 상태, 단계 검증, 시작 확정·취소, 이동 상태 통지를 이전한다.
- WxHUDLayout을 상속한 WxFrontEndWidget에 버튼 바인딩, 페이지·포커스, 팝업, 문구·활성화 처리를 구현한다.
- WBP_FrontEnd의 부모와 이벤트·변수를 정리하고 디자인과 옵션 데이터를 보존한다.
- 선택·확인·취소·종료·이동 실패 흐름, 블루프린트 컴파일, UE 5.8 WxEditor Development 빌드를 검증한다.

## 완료

- WxGameFlowSubsystem이 선택 단계·옵션, 확정·취소, 중복 요청 검증과 상태 변경 통지를 관리한다. 기존 RequestNewGame과 Pawn 선택·맵 이동 경로는 유지한다.
- WxFrontEndWidget을 WxHUDLayout 하위에 추가해 6개 버튼 바인딩, 페이지·포커스, 확인 팝업, 오류 문구·버튼 활성화 및 구독 수명을 C++로 이전했다.
- WBP_FrontEnd 부모를 WxFrontEndWidget으로 변경하고 EventGraph 노드 132개와 BP 포커스 오버라이드·런타임 변수를 제거했다. 위젯 디자인과 캐릭터 2개·목적지 2개 옵션(FText 식별자 포함)의 변경 전후 동일성을 검증했다.
- UE 5.8 WxEditor Win64 Development 빌드 성공: Saved/Logs/BuildDoctor/build_2026-09-12_065512_072_12240.log.
- Wx.FrontEnd.SelectionFlow 자동화 테스트 성공: 단계 순서, 누락된 선택, 중복 입력, 취소, 늦은 콜백, 요청 실패 후 재시도와 상태 통지 검증. Saved/Logs/FrontEndCppTests.log.
- 저장된 에셋을 새 UE 프로세스로 다시 로드하고 Blueprint 경고를 오류로 취급한 컴파일을 통과했다. Saved/FrontEndCppMigrationResult.json, Saved/FrontEndBeforeCpp.json.
- 별도 NullRHI PIE에서 실제 CommonUI 버튼을 호출해 페이지 전환, 팝업 내용, 취소, 요청 거절 즉시 오류 표시, 이동 중 버튼 잠금, Template Player → LV_DevCombat, HGTest → LV_OpenWorld, FrontEnd 복귀, Quit 종료를 검증했다. Saved/FrontEndCppRuntimeResults.json = SUCCESS.
- 실제 엔진 TravelFailure는 강제로 재현하지 않았다. 요청 거절 복구는 자동화·PIE 양쪽으로 검증했다. PIE 중 별도 보스 네임플레이트 MVVM 초기화 오류와 MCP 포트 점유 로그가 있었으나 이번 범위의 검증은 완료됐다.
- 에셋 변경 전 백업: Saved/WBP_FrontEnd-before-cpp.uasset. 마이그레이션 및 PIE 검증 스크립트는 Saved/에 보관했다.


# FrontEndSettings 제거

## 계획

- 승인에 따라 FrontEndSettings 클래스와 ini 설정을 제거한다.
- 요청 당시 출발 맵의 PIE 접두사를 제거한 패키지 이름을 기억해 실패 복귀에 사용하고, 전환 제한 시간은 기존 120초를 내부 상수로 유지한다.
- 기존 Experience에 메뉴 전용 표식이 없으므로 맵 이름 기반 신규 게임 제한을 제거한다. 싱글플레이·중복 요청·맵·Pawn 검증은 유지한다.
- 사용처가 사라진 WxGame의 DeveloperSettings 의존성을 제거한다.
- UE 5.8 WxEditor Development 빌드, 기존 왕복 진입 및 출발 맵 복귀를 검증한다.

## 완료

- FrontEndSettings 클래스·ini 섹션·WxGame의 DeveloperSettings 의존성을 제거했다.
- 유효한 요청을 접수할 때 출발 맵의 PIE 접두사를 제거한 패키지 이름을 ReturnLevelPackage에 기억한다. 실패 복구와 복구 완료 판정에서 이 이름을 사용한다. 기존 120초 제한은 cpp 내부 상수로 유지한다.
- 기존 Experience에 메뉴 전용 표식이 없어 맵 이름 기반 제한을 제거했다. 싱글플레이·중복 요청·참조·같은 맵 진입 검사는 유지했다. 새로운 정책 변수는 추가하지 않았다.
- UE 5.8 WxEditor Win64 Development 빌드 성공(종료 코드 0, Result: Succeeded): C:/Wx/Saved/Logs/BuildDoctor/build_2026-09-05_210112_016_15980.log.
- 고정 FrontEnd와 다른 임시 LV_Source에서 Experience가 없는 임시 맵으로 입장해, 실패 후 캡처한 LV_Source로 돌아오고 메뉴 버튼이 다시 활성화됨을 검증했다. PIE 패키지 접두사 제거도 확인했다. 로그: C:/Wx/Saved/Logs/CapturedReturnMap.log. 새로 만든 임시 맵 두 개는 검증 종료 후 경로·파일 목록을 확인해 제거했다.
- 기존 메뉴의 선택 유지·중복 방지·두 맵 진입·FrontEnd 왕복·입력 복원 PIE 통과: C:/Wx/Saved/Logs/FrontEndWithoutSettings.log.
- 삭제 타입·함수의 소스/설정 참조 없음 및 diff 공백 검사 통과. 현재 설계 문서를 정정했다. 로딩·인벤토리 변경 없음. 실제 렌더링과 패키지는 검증하지 않았다.

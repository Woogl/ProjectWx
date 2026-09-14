# 전체 테스트 코드 제거

## 계획

- 사용자의 명시적 삭제 요청에 따라 오늘 추가한 Source/WxGame/Tests/WxHitWrapperTest.cpp 전체를 제거한다.
- 추가 확인에 따라 기존 Source/WxGame/Tests/WxNameplateViewModelTest.cpp도 제거한다. 게임 코드와 기존 검증 이력은 유지한다.
- UE 5.8 WxEditor Development 빌드로 삭제 후 컴파일을 확인한다.

## 완료

- WxHitWrapperTest.cpp와 WxNameplateViewModelTest.cpp를 모두 삭제했다. Source 및 Plugins의 C++ 소스에서 자동화 테스트 선언·헤더·WITH_DEV_AUTOMATION_TESTS 잔존 항목이 없음을 확인했다.
- UE 5.8.2 WxEditor Win64 Development 최종 빌드 성공(종료 코드 0). 로그: C:/Wx/Saved/Logs/BuildDoctor/build_2026-09-14_224341_229_27756.log.
- git diff --check 통과. 이전 실행 결과와 작업 이력은 보존했다. 커밋·푸시는 실행하지 않았다.

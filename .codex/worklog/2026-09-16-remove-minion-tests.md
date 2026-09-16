# 제출용 미니언 테스트 코드 제거

## 계획

- 사용자 요청대로 이번 작업에서 추가한 미니언 자동화 테스트와 테스트용 어빌리티 세 파일을 제거한다.
- 테스트 전용으로 추가한 WxEditor의 AIModule/WxAI/WxCombat 의존성 및 README 테스트 실행 안내를 제거한다.
- 런타임 변경은 유지하고 WxEditor Development 빌드로 제출 구성을 검증한다.

## 완료

- 이번 작업의 자동화 테스트 및 테스트 어빌리티 세 파일을 제거했다. Source/Plugins에서 해당 테스트 코드 참조가 남지 않음을 확인했다.
- 테스트 전용 모듈 의존성과 README 실행 안내를 제거했다. WxEditor.Build.cs는 HEAD 대비 차이가 없다.
- UE 5.8 WxEditor Win64 Development 빌드 성공. 로그: Saved/Logs/BuildDoctor/build_2026-09-16_230011_734_5868.log. git diff --check 통과.

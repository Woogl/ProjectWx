# 플레이어 레이아웃 컴포넌트 이름 정리

## 계획

- 승인받은 UWxPlayerLayoutComponent, LayoutClass, LayoutWidget 이름과 관련 함수·멤버·코드 및 README 참조를 정리한다.
- BP 호환성을 위해 클래스·프로퍼티 CoreRedirects를 추가하고 기본 서브오브젝트 식별자는 유지한다. 화면 표시 동작과 순서는 유지한다.
- 잔여 참조 및 diff를 확인하고 UE 5.8 WxEditor Win64 Development 빌드로 검증한다.

## 완료

- 클래스·파일을 UWxPlayerLayoutComponent로 변경하고 LayoutClass, LayoutWidget, 관련 함수·멤버와 Controller/GameFlow/README 참조를 정리했다.
- 클래스·프로퍼티 리다이렉트를 추가하고 BP 컴포넌트 식별자 HUDComponent는 호환성을 위해 유지했다.
- UE 5.8 WxEditor Win64 Development 빌드 성공(exit 0). 로그: Saved/Logs/BuildDoctor/build_2026-09-07_215610_132_24808.log.
- Python 커맨드릿에서 두 Controller BP의 새 컴포넌트 타입 및 WBP_GameHUD/WBP_FrontEnd LayoutClass 보존을 확인했다. 최초 검증에서 누락된 프로퍼티를 발견해 클래스 변환 후 경로 리다이렉트를 보완한 뒤 두 검증 모두 통과했다.
- git diff --check 통과. 실제 화면 플레이 검증은 실행하지 않았다. 기존 사용자 변경은 보존했다.

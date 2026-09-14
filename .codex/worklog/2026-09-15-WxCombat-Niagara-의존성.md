# WxCombat Niagara 플러그인 의존성 선언

## 계획

- 승인된 범위: WxCombat.uplugin의 Plugins 배열에 Niagara를 Enabled: true로 추가한다.
- JSON 유효성과 UE 5.8 WxEditor Win64 Development 빌드를 확인한다.
- 검증 후 이 문서에 완료 결과를 기록한다.

## 완료

- WxCombat.uplugin의 Plugins 배열에 Niagara(Enabled: true)를 추가했다.
- JSON 파싱 및 Niagara 선언이 정확히 한 개이고 활성화되어 있는지 검증했다.
- git diff --check를 통과했다.
- UE 5.8 WxEditor Win64 Development 빌드 성공(종료 코드 0). Niagara 의존성 경고 없음.
- 빌드 로그: C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-15_015840_993_27656.log

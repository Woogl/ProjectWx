# BossComponent 호환 레이어 제거

## 계획

- 사용자가 `BP_Boss`, `BP_Enemy`를 새 `EnemyRank` 구조로 리세이브하고 동작을 확인한 것을 호환 종료 조건으로 삼는다.
- 저장소에서 `UWxBossComponent`와 `InitializeLegacyBossRank`의 잔존 참조를 확인한다.
- 상태 없는 임시 `UWxBossComponent` 소스와 `UWxEnemyComponent::InitializeLegacyBossRank` 진입점을 제거한다.
- Boss 등급 판정, `IsEngaged`, 보스 HUD 관찰 경로는 `UWxEnemyComponent`에 유지해 런타임 동작을 보존한다.
- 관련 심볼 검색과 `git diff --check`를 수행하고, UE 5.8 `WxEditor` Win64 Development 빌드로 검증한다.

## 완료

- 리세이브된 `Content/Character/Boss/BP_Boss.uasset`, `Content/Character/Enemy/BP_Enemy.uasset`에 `WxBossComponent`, `BossComponent`, `InitializeLegacyBossRank` 문자열이 남지 않은 것을 확인했다.
- `UWxBossComponent` 헤더와 구현 파일을 삭제했다.
- `UWxEnemyComponent::InitializeLegacyBossRank` 선언과 구현을 제거했다.
- 실행 코드·플러그인·설정에서 삭제된 호환 심볼 참조가 0건임을 확인했다. 과거 설계 문서와 작업 기록은 변경 이력으로 유지했다.
- Build Doctor의 첫 실행은 Windows PowerShell 5.1이 `utf8NoBOM`을 지원하지 않아 빌드 시작 전 중단됐고, PowerShell 7에서 같은 스크립트를 재실행했다.
- UE 5.8 `WxEditor` Win64 Development 빌드 성공: `C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-04_220657_085_9024.log`.
- `git diff --check`와 최종 심볼 검색으로 공백 오류와 런타임 잔존 참조가 없음을 확인했다.

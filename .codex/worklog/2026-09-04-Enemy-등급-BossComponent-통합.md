# Enemy 등급과 BossComponent 통합

## 계획

- `EWxEnemyRank`를 `Normal`, `Elite`, `Boss` 값으로 추가하고 `UWxEnemyComponent`의 단일 등급 데이터로 사용한다.
- `UWxEnemyComponent`에 `GetEnemyRank`, `IsBoss`, `IsEngaged`와 보스 HUD 생명주기·교전 변경 델리게이트를 이관한다.
- 기존 네임플레이트 갱신에 사용하던 AI 대상·사망 이벤트에서 교전 상태도 함께 갱신해 중복 구독을 제거한다.
- 보스 ViewModel이 `UWxBossComponent` 대신 Boss 등급의 `UWxEnemyComponent`를 탐색·관찰하도록 변경한다.
- `UWxBossComponent`는 독립 상태 없이 기존 BP의 EnemyComponent를 Boss 등급으로 초기화하고 조회를 전달하는 deprecated 호환 어댑터로 축소한다.
- 에디터 에셋은 수정하지 않는다. 사용자가 BP의 EnemyRank를 Boss로 저장하고 BossComponent를 제거한 뒤 호환 어댑터 삭제를 별도 축소 단계로 진행한다.
- UE 5.8 `WxEditor` Win64 Development 빌드와 잔존 참조·포맷을 검증한다.

## 완료

- `EWxEnemyRank`를 추가하고 기본값을 `Normal`로 두어 `Normal`, `Elite`, `Boss` 등급을 `UWxEnemyComponent` 한 곳에서 관리하도록 구성했다.
- `UWxEnemyComponent`가 `IsBoss`, `IsEngaged`와 보스 준비·종료·교전 변경 이벤트를 소유하도록 이관했다. 기존 대상 변경·사망 처리 경로에서 교전 상태를 함께 갱신한다.
- 보스 ViewModel이 `AWxCharacterBase`의 Boss 등급 `UWxEnemyComponent`를 탐색하고 직접 관찰하도록 변경했다.
- `UWxBossComponent`의 상태와 이벤트 구독을 제거하고, 기존 BP가 남아 있는 동안 Enemy 등급을 Boss로 승격하는 임시 호환 어댑터로 축소했다.
- 에디터 에셋은 수정하지 않았다. BP별 `EnemyRank` 저장과 기존 `BossComponent` 제거는 사용자가 진행한 뒤 호환 어댑터를 별도 단계에서 삭제할 수 있다.
- 최초 빌드에서 Unreal의 deprecated 클래스 접두사 규칙 때문에 UHT 오류가 발생해, 클래스 이름 유지 요구에 맞춰 `Deprecated` 지정자는 제거하고 deprecation 메시지만 유지했다.
- UE 5.8 `WxEditor` Win64 Development 빌드 성공: `Saved/Logs/BuildDoctor/build_2026-09-04_215517_707_19312.log`.
- `git diff --check`와 관련 심볼 검색으로 공백 오류, ViewModel의 `UWxBossComponent` 의존, BossComponent의 중복 상태가 없음을 확인했다.

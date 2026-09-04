# EnemyCharacter EnemyComponent 전체 통합

## 계획

- `AWxEnemyCharacter`가 보유하던 AI 조립과 `UWxEnemyComponent`의 네임플레이트·피니셔·보상·스포너·보스 상태 책임을 하나의 액터로 합친다.
- 파티원 AI와의 의미 혼동을 피하기 위해 최종 액터 이름은 `AWxEnemyCharacter`로 유지한다. 아군 소환은 `Team`을 바꾸고, 미니언 관계는 Spawn `Instigator`, 보스 여부는 `bIsBoss`로 독립 판정한다.
- 보스 ViewModel과 전역 보스 준비·종료·교전 이벤트가 `UWxEnemyComponent` 대신 `AWxEnemyCharacter`를 관찰하도록 변경한다.
- 기존 BP의 `EnemyComponent` 템플릿에 저장된 `bIsBoss`, 피니셔 각도, 보상 행, 발사 속도를 잃지 않도록 확장 단계에서 액터 프로퍼티로 이관하고 관련 캐릭터 BP를 리세이브한다.
- 에셋 이관 검증 뒤 `UWxEnemyComponent`, `EWxEnemyRank`, `WxEnemyTypes`를 제거하고 `AWxEnemyCharacter` 클래스는 보존한다.
- 단계별 참조·포맷 검사를 수행하고 최종 UE 5.8 `WxEditor` Win64 Development 빌드로 검증한다.

## 완료

- `AWxEnemyCharacter` 클래스 이름은 유지하고 `UWxEnemyComponent`가 담당하던 초기화, 네임플레이트, 피니셔, 스포너 연결, 처치 보상, 보스·교전 상태를 액터로 흡수했다.
- 보스 ViewModel이 컴포넌트 대신 `AWxEnemyCharacter`의 준비·종료·교전 이벤트를 직접 관찰하도록 변경했다.
- `BP_Boss`, `BP_Enemy`, `BP_Minion`, `BP_Sandbag`을 단계적으로 재컴파일·리세이브해 구 네이티브 컴포넌트 서브오브젝트를 제거했다.
- 에셋 기본값은 `BP_Boss: bIsBoss=true, Gold500`, `BP_Enemy: bIsBoss=false, Gold100`, `BP_Minion/BP_Sandbag: bIsBoss=false, 보상 없음`으로 보존했다.
- 구 타입 참조와 에셋 직렬화 흔적이 사라진 뒤 `UWxEnemyComponent`, `EWxEnemyRank`, `WxEnemyTypes` 및 일회성 이관 코드를 제거했다.
- 구 컴포넌트 타입이 없는 최종 실행 환경에서 네 블루프린트의 로드와 보스 판정을 모두 확인했다: `C:\Wx\Saved\Logs\Wx.log`.
- `git diff --check`에서 공백 오류가 없음을 확인했다(기존 줄바꿈 변환 경고만 출력).
- UE 5.8 `WxEditor` Win64 Development 최종 빌드 성공: `C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-05_010426_514_11428.log`.

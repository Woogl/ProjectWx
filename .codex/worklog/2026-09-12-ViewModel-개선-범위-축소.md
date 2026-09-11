# ViewModel 개선 범위 축소

## 계획

- 사용자가 승인한 대로 구독 누수·종료·초기화 알림 수정만 남긴다.
- Ability와 Effect의 독립 관찰 및 자식 티커 추가를 되돌리고 기존 AbilitySystem의 갱신·제거 관리를 복원한다.
- Inventory 직접 주입, 파생 Resolver 및 고정 타입 검증 확장을 되돌린다.
- AbilitySystem의 팩토리 내부 초기화 계약, 재연결 전 구독 해제, 종료 시 대기 취소·표시 초기화와 알림은 유지한다.
- 테스트와 리뷰 후속 기록도 축소된 범위에 맞춘다. UE 5.8 WxEditor Development 빌드 및 관련 자동화 테스트로 검증한다.

## 완료

- Ability의 Spec 관찰과 Effect 제거를 기존 부모 AbilitySystem 관리로 복원했다. 자식 Spec 콜백·티커, Effect 제거 콜백, 부모의 콜백 순서 방어 코드를 제거했다.
- Inventory 직접 주입 및 관찰 상태, Resolver 파생 타입 생성·타입 검증을 되돌렸다. 공유본 재개 지원도 제거하고 기존 ASC별 생성 경로를 유지한다.
- AbilitySystem 초기화를 private로 제한한 계약, Dialogue·Quest의 재연결 전 구독 해제, InteractionList 종료 시 Ready 대기 취소, 표시 초기화/FieldNotify와 개별 Resolver 종료 정리를 유지했다.
- 런타임 소스 변경은 최초 확장 19파일(228줄 추가/93줄 삭제)에서 12파일(109줄 추가/37줄 삭제)로 줄었다. 회귀 테스트의 직접 주입·독립 관찰·공유본 재개 항목도 제거했다.
- UE 5.8 WxEditor Win64 Development 빌드 성공, 종료 코드 0. 로그: `Saved/Logs/BuildDoctor/build_2026-09-12_072054_615_20444.log`.
- `Wx.ViewModel` 회귀 테스트 3개 성공, 경고/실패/미실행 0개. 결과: `Saved/Automation/ViewModelSimplified/index.json`, 로그: `Saved/Logs/ViewModelSimplifiedTests.log`.
- 변경 소스의 `git diff --check` 통과. 리뷰 두 문서와 최초 작업 기록에 최종 축소 범위를 반영했다. 다른 작업의 미커밋 변경은 보존했다.
- 실제 WBP 화면·네트워크 PIE는 검증하지 않았다. 독립 사용 및 순정 ASC 목록 변경 이벤트 제약은 기존 구조의 사용 조건으로 유지한다.

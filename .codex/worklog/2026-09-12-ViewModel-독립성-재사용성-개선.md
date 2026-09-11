# ViewModel 독립성·재사용성 개선

> 이후 사용자의 범위 축소 승인으로 독립 관찰·직접 주입·Resolver 확장을 되돌렸다. 아래는 최초 구현 기록이며, 최종 상태는 [범위 축소 기록](2026-09-12-ViewModel-개선-범위-축소.md)을 따른다.

## 계획

- 사용자가 앞선 리뷰의 개선사항 적용을 승인했다. 기존 미커밋 작업은 보존한다.
- 공유 AbilitySystem은 ASC별 팩토리 생성으로 초기화 계약을 제한하고, 종료 시 목록·태그 초기화를 통지한다.
- Ability의 Spec 변경 관찰과 Effect 제거 관찰을 각 VM이 수행하게 한다. 재초기화·종료에서 표시 필드를 초기화하고 FieldNotify를 발행하되 GC 중에는 통지하지 않는다. Attribute의 Max 생략 의미를 팩토리와 일치시킨다.
- Dialogue·Quest의 소스 교체 전 구독을 해제하고, InteractionList의 대기 취소와 종료를 통일한다. 개별 Resolver의 종료도 연결 정리를 호출한다.
- Inventory에 컴포넌트 직접 주입을 추가한다. 위젯별 Resolver와 PlayerCharacter는 요청 파생 타입을 지원하고, 고정 타입 공유 Resolver는 호환되지 않는 요청을 거절한다.
- 재연결·빈 상태 통지·독립 관찰·직접 주입의 회귀 테스트를 작성한다. 런처 정보에서 UE 5.8 경로를 찾아 WxEditor Win64 Development 빌드 및 가능한 자동화 테스트를 실행한다.

## 완료

- AbilitySystem 초기화를 private 팩토리 경로로 제한하고 반복 조회를 멱등 처리했다. 공유본 종료 시 표시 목록/태그를 통지하며 이미 외부에서 보유한 자식의 관찰은 유지한다.
- Ability는 SpecDirtied를 직접 구독하고 다음 틱에 재매칭한다. 종료 시 예약/구독/이미지를 정리하고 빈 표시를 통지한다. 자기 태그 참조를 재초기화 인자로 넘기는 경우도 보존한다.
- Attribute는 Max 생략을 Current로 정규화하고 종료 수치를 초기화한다. Effect는 단독 제거 관찰과 유한→무한 전환 초기화를 지원하며, 부모 목록 제거는 콜백 순서에 영향받지 않게 처리했다.
- Dialogue·Quest의 이전 소스 구독을 해제하고 빈 상태를 통지한다. InteractionList는 재연결/종료 시 Ready 대기를 해제한다. 개별 Resolver는 종료 시 Deinitialize를 호출한다.
- Inventory 직접 주입은 임의 Actor의 컴포넌트와 BeginPlay 전 준비 대기/EndPlay 정리를 지원한다. 위젯별 Resolver 및 PlayerCharacter는 요청 파생 타입을 생성하고, 고정 타입 공유 Resolver는 호환되지 않는 타입을 거절한다.
- `Source/WxGame/Tests/WxViewModelReuseTest.cpp`에 SourceReuse, ObservationLifetime, GASReuse 회귀 테스트를 추가했다. 최종 결과 3개 성공, 경고/실패/미실행 0개. 결과: `Saved/Automation/ViewModelReuseFinal/index.json`, 로그: `Saved/Logs/ViewModelReuseTestsFinal.log`.
- UE 5.8 WxEditor Win64 Development 최종 빌드 성공(종료 코드 0). 로그: `Saved/Logs/BuildDoctor/build_2026-09-12_071421_108_28676.log`. 최초 샌드박스 실행 뒤 build-doctor의 샌드박스 외 실행으로 검증했다. `git diff --check`도 변경 소스에서 통과했다.
- 두 ViewModel 리뷰 문서에 후속 적용 상태와 사용 계약을 덧붙였다. 기존 모듈 리뷰 문서 및 다른 작업의 미커밋 소스·에셋은 수정하지 않았다.
- 한계: UE 5.8 순정 ASC의 단순 ClearAbility와 클라이언트 복제는 SpecDirtied를 발행하지 않는다. 이 경로는 목록 변경 완료 후 공개 RefreshBoundAbility를 호출해야 하며 헤더/리뷰에 명시했다. 게임 비용·충전/보스/자막 정책을 일반화하거나 도메인 이벤트를 신설하지 않았다. 실제 WBP 화면과 네트워크 PIE는 실행하지 않았다.

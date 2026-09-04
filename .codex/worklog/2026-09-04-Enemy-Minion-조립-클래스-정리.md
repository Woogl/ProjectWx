# Enemy·Minion 조립 클래스 정리

## 계획

- `AWxEnemyCharacter`와 `AWxMinion`은 필수 역할 컴포넌트와 기본값을 제공하는 얇은 조립 클래스로 유지한다.
- 에디터에서 컴포넌트로 이관된 Behavior Tree와 적 전용 설정의 구형 액터 프로퍼티·런타임 복사 경로를 제거한다.
- `AWxEnemyCharacter`의 사용되지 않는 전달 API를 참조 확인 후 제거하되, 컴포넌트 구성과 팀·이동 기본값은 보존한다.
- `AWxCharacterBase::PreInitializeComponents`의 Spawn Context 전달 시점은 유지해 Deferred Spawn 동작 순서를 보존한다.
- 관련 심볼 검색과 `git diff --check`를 수행하고, UE 5.8 `WxEditor` Win64 Development 빌드로 검증한다.

## 완료

- `AWxEnemyCharacter`와 `AWxMinion`을 유지하고, 역할별 필수 컴포넌트를 생성하는 얇은 조립 클래스 경계를 확정했다.
- `AWxCharacterBase`의 deprecated `BehaviorTreeAsset`과 `UWxAIBehaviorComponent::InitializeLegacyBehaviorTree` 복사 경로를 제거해 Behavior Tree의 단일 소유자를 `UWxAIBehaviorComponent`로 정리했다.
- `AWxEnemyCharacter`의 구형 처형·보상 프로퍼티와 `PreInitializeComponents` 복사 경로를 제거해 해당 설정의 단일 소유자를 `UWxEnemyComponent`로 정리했다.
- 외부 호출이 없던 `AWxEnemyCharacter`의 `HasAITarget`, `GetOwningSpawner`, `GetEnemyComponent` 전달 API를 제거했다.
- `AWxCharacterBase::PreInitializeComponents` 자체와 Spawn Context 전달 순서는 유지했다.
- `AWxEnemyCharacter`에는 Enemy 팀, 기본 이동 속도, `AIBehaviorComponent`, `EnemyComponent`, `NameplateComponent`, `LockOnPoint` 구성만 남겼다.
- UE 5.8 `WxEditor` Win64 Development 빌드 성공: `C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-04_223042_017_25176.log`.
- 관련 legacy 심볼 검색과 `git diff --check`로 잔존 참조와 공백 오류가 없음을 확인했다.

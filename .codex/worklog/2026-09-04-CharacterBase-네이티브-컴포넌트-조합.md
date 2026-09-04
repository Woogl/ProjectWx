# CharacterBase 네이티브 컴포넌트 조합

## 계획

- `AWxAICharacter`를 제거하고 `AWxEnemyCharacter`·`AWxMinion`을 다시 `AWxCharacterBase` 기반 호환 클래스로 연결한다. 아직 에디터 에셋은 리패런팅하지 않으며 구형 클래스는 삭제하지 않는다.
- 공통 락온 상태 질의인 `HasCombatTarget`을 `AWxCharacterBase`로 옮긴다.
- `UWxAIBehaviorComponent`를 추가해 Behavior Tree 설정을 네이티브 컴포넌트가 소유하게 하고, `AWxAIController`가 컴포넌트를 통해 트리를 실행하도록 변경한다. 기존 BP의 `BehaviorTreeAsset`은 호환 프로퍼티로 보존하고 런타임에 빈 컴포넌트 설정으로 이관한다.
- `AWxCharacterBase`가 액터 전용 `IWxInteractable`·`IWxSpawnable` 계약을 구현하되, 구체 역할 컴포넌트를 직접 알지 않도록 네이티브 Interaction Provider·Spawn Context Receiver 인터페이스로 라우팅한다.
- `UWxEnemyComponent`가 Interaction Provider와 Spawn Context Receiver를 구현하도록 변경하고, 역할 컴포넌트의 Owner 계약을 `AWxCharacterBase`로 일반화한다.
- `UWxBossComponent`, `UWxMinionComponent`, 보스 ViewModel도 `AWxCharacterBase`를 관찰하도록 변경한다.
- 단계별로 UE 5.8 `WxEditor` Win64 Development 빌드와 소스 참조를 검증한다. BP 조립·저장, 호환 프로퍼티와 구형 클래스 제거, 선택적 매니저를 `AWxCharacterBase`에서 제거하는 축소 단계는 사용자의 에디터 마이그레이션 뒤로 남긴다.

## 완료

- `AWxAICharacter`를 제거하고 `AWxEnemyCharacter`·`AWxMinion`을 `AWxCharacterBase`의 BP 마이그레이션용 호환 클래스로 유지했다.
- `HasCombatTarget`을 공통 락온 상태를 소유하는 `AWxCharacterBase`로 옮겼다.
- `UWxAIBehaviorComponent`를 추가하고 `AWxAIController`가 이 컴포넌트의 Behavior Tree를 실행하도록 변경했다. 기존 BP의 `BehaviorTreeAsset`은 deprecated 호환 프로퍼티로 남겨 빈 컴포넌트 설정에 전달한다.
- `AWxCharacterBase`가 `IWxInteractable`·`IWxSpawnable` 액터 계약을 구현하고, `IWxInteractionProvider`·`IWxSpawnContextReceiver`를 구현한 네이티브 컴포넌트로 요청을 전달하도록 구성했다.
- `UWxEnemyComponent`가 상호작용과 Spawn 문맥 Provider를 구현하도록 변경했다. Deferred Spawn 문맥은 BP SCS 컴포넌트가 구성된 뒤이면서 AI 빙의 전인 `PreInitializeComponents`에서 전달한다.
- Enemy·Minion·Boss 역할 컴포넌트와 보스 ViewModel의 Owner 계약을 모두 `AWxCharacterBase`로 일반화했다.
- 기존 `AWxEnemyCharacter`·`AWxMinion`에는 `UWxAIBehaviorComponent`와 역할 컴포넌트를 기본 서브오브젝트로 두어 현행 BP 동작을 보존했다.
- 에디터 에셋은 수정하지 않았다. 직접 `AWxCharacterBase`를 부모로 쓰는 AI BP에서는 `AIBehaviorComponent`와 역할 컴포넌트를 조립하고 `AIControllerClass`·`AutoPossessAI`를 설정해야 한다.
- 관련 Character 에셋과 소스·설정에서 `WxAICharacter` 참조가 없음을 확인했다.
- UE 5.8 `WxEditor` Win64 Development 빌드 성공: `C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-04_213540_349_21028.log`.
- `git diff --check`에서 공백 오류가 없음을 확인했다(기존 줄바꿈 변환 경고만 출력).
- 호환 프로퍼티·구형 클래스 제거와 `AWxCharacterBase`의 선택적 Manager 컴포넌트 분리는 BP 마이그레이션 이후 별도 축소 단계로 남겼다.

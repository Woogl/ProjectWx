# Minion 소유권의 Instigator 단일화

## 계획

- 미니언의 Master와 Unreal `Instigator`가 같은 대상을 중복 저장하는 구조를 제거하고, 복제되는 `AActor::Instigator`를 단일 소유권 데이터로 사용한다.
- `AWxEnemyCharacter`에서 `UWxMinionComponent` 기본 서브오브젝트를 제거한다.
- `AWxAIController`가 미니언 Master를 역할 컴포넌트 없이 Pawn의 Instigator에서 직접 해석하도록 단순화한다.
- `UWxMinionManagerComponent`가 Deferred Spawn 시 Instigator를 전달하고 생성 전에 팀을 복사하는 기존 순서를 유지한다.
- 이번 단계에서는 리세이브 전 에셋 호환을 위해 `UWxMinionComponent`와 `AWxMinion` 타입을 삭제하지 않는다.
- 관련 심볼과 포맷을 검사하고 UE 5.8 `WxEditor` Win64 Development 빌드로 검증한다. 사용자가 `BP_Minion`을 `AWxEnemyCharacter`로 리패런팅·리세이브한 뒤 두 호환 타입을 별도 축소 단계에서 삭제한다.

## 완료

- `AWxEnemyCharacter`에서 `UWxMinionComponent` 멤버와 기본 서브오브젝트 생성을 제거했다.
- `AWxAIController::ResolveMinionMaster`가 Pawn의 복제되는 Instigator만 읽도록 단순화하고 역할 컴포넌트 의존을 제거했다.
- `UWxMinionComponent`는 Master 상태, 자체 복제, 팀 복사, 초기화 API를 모두 제거한 상태 없는 비추가형 호환 타입으로 축소했다.
- `UWxMinionManagerComponent`의 Deferred Spawn Instigator 전달과 FinishSpawning 전 팀 복사 순서는 유지했다.
- `AWxMinion`은 `BP_Minion` 리패런팅 전까지 `AWxEnemyCharacter`를 상속하는 상태 없는 호환 클래스로 유지했다.
- UE 5.8 `AActor::Instigator`가 `ReplicatedUsing=OnRep_Instigator`로 선언된 것을 엔진 소스에서 확인했다.
- UE 5.8 `WxEditor` Win64 Development 빌드 성공: `C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-04_225252_118_30168.log`.
- 관련 심볼 검색과 `git diff --check`로 호환 선언 외 런타임 컴포넌트 참조와 공백 오류가 없음을 확인했다.

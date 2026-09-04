# Minion 역할의 EnemyCharacter 통합 확장

## 계획

- `AWxEnemyCharacter`에 `UWxMinionComponent`를 `MinionRoleComponent`라는 기본 서브오브젝트로 추가해 모든 전투 AI가 선택적으로 소환자의 Master·팀 관계를 받을 수 있게 한다.
- `AWxMinion`은 `AWxEnemyCharacter`를 상속하는 상태 없는 임시 호환 클래스로 축소하고, 중복 `AIBehaviorComponent`와 `MinionRoleComponent` 구성을 제거한다.
- 일반 배치 및 `AWxSpawner` 생성 경로는 Instigator가 없으므로 Enemy 팀을 유지하고, `UWxMinionManagerComponent` 생성 경로만 Instigator를 통해 소환자의 팀을 상속하는 기존 동작을 보존한다.
- 소스 참조, 기본 서브오브젝트 이름, 포맷을 확인하고 UE 5.8 `WxEditor` Win64 Development 빌드로 확장 단계를 검증한다.
- 이번 단계에서는 `AWxMinion` 클래스를 삭제하지 않는다. 사용자가 `BP_Minion`의 부모를 `AWxEnemyCharacter`로 변경해 리세이브하고 PIE 검증을 완료한 뒤 별도 축소 단계로 삭제한다.

## 완료

- `AWxEnemyCharacter`에 `UWxMinionComponent` 타입의 `MinionRoleComponent` 기본 서브오브젝트를 추가했다.
- `AWxMinion`을 `AWxEnemyCharacter`를 상속하는 상태 없는 호환 클래스로 축소하고, 중복 컴포넌트 멤버와 생성자 구현을 제거했다.
- 일반 `AWxSpawner`는 Instigator를 `nullptr`로 전달하고 Minion Manager만 소환자를 Instigator로 전달하므로, 일반 적은 Enemy 팀을 유지하고 소환된 전투 AI만 Master의 팀을 상속하는 경계를 확인했다.
- `MinionRoleComponent`의 이름과 타입을 기존 `AWxMinion` 구성과 동일하게 유지해 `BP_Minion` 리패런팅 시 컴포넌트 템플릿 이관 위험을 줄였다.
- `AWxMinion` 클래스와 `BP_Minion` 부모 참조는 사용자의 에디터 리패런팅·PIE 검증 전까지 유지했다.
- UE 5.8 `WxEditor` Win64 Development 빌드 성공: `C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-04_224349_487_21528.log`.
- `git diff --check`와 관련 심볼 검색으로 공백 오류와 중복 `MinionRoleComponent` 생성이 없음을 확인했다.

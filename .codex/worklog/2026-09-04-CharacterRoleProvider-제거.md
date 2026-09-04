# CharacterRoleProvider 제거

## 계획

- 현재 Git 기준본에 있던 `AWxEnemyCharacter`의 액터 인터페이스 구현을 참고해 `IWxInteractable`과 `IWxSpawnable`의 소유를 `AWxCharacterBase`에서 역할별 조립 클래스로 내린다.
- `AWxEnemyCharacter`는 보유가 보장된 `UWxEnemyComponent`에 상호작용과 스포너 문맥을 직접 전달하고, 기능 상태와 로직은 컴포넌트에 유지한다.
- `AWxNpc`는 이미 `IWxInteractable`인 `AWxDialogueActor` 파생임을 확인해 기존 상호작용 자격에 영향이 없도록 유지한다.
- `AWxCharacterBase`의 Provider 검색, 지연 스폰 문맥 전달 상태를 제거하고 `UWxEnemyComponent`의 내부 Provider 인터페이스 상속을 제거한다.
- 더 이상 참조되지 않는 `WxCharacterRoleProvider.h`를 삭제한다.
- 관련 참조, 공백 오류와 변경 전후 계약을 확인하고 UE 5.8 `WxEditor` Win64 Development 빌드로 검증한다.

## 완료

- `AWxCharacterBase`에서 `IWxInteractable`, `IWxSpawnable`과 Provider 검색·지연 전달 상태를 제거해 모든 캐릭터가 역할 계약으로 노출되던 구조를 정리했다.
- `AWxEnemyCharacter`가 두 액터 인터페이스를 명시적으로 구현하고, 생성자에서 보유가 보장되는 `UWxEnemyComponent`에 직접 전달하도록 변경했다.
- `UWxEnemyComponent`의 기능과 실패 동작은 유지하면서 내부 Provider 인터페이스 상속만 제거했다.
- `AWxNpc`는 이미 `IWxInteractable`을 구현한 `AWxDialogueActor` 파생이라 변경하지 않았다.
- `WxCharacterRoleProvider.h`를 삭제했고 관련 타입과 헬퍼의 잔여 참조가 없음을 확인했다.
- `git diff --check`를 통과했다. 줄 끝 변환 예정 경고만 있었고 공백 오류는 없었다.
- UE 5.8 `WxEditor Win64 Development` 빌드에 성공했다. 로그: `Saved/Logs/BuildDoctor/build_2026-09-04_233602_634_14612.log`

# WxGame Team 복제

## 계획

- `AWxCharacterBase::Team`을 `ReplicatedUsing = OnRep_Team`으로 변경한다.
- `AWxCharacterBase::GetLifetimeReplicatedProps()`에서 `Team`을 복제 등록한다.
- 이전 값을 받는 `OnRep_Team(EWxTeam PreviousTeam)` RepNotify 진입점을 추가하되, 현재 소비자는 `GetGenericTeamId()`로 값을 직접 조회하므로 별도 캐시나 델리게이트는 추가하지 않는다.
- UE 5.8 `WxEditor` Development 타겟을 빌드하고, 관련 파일 diff와 복제 등록을 확인한다.

## 완료

- `AWxCharacterBase::Team`을 `ReplicatedUsing = OnRep_Team`으로 변경했다.
- `GetLifetimeReplicatedProps()`를 추가하고 `DOREPLIFETIME(AWxCharacterBase, Team)`으로 런타임 팀 변경을 클라이언트에 복제하도록 등록했다.
- `OnRep_Team(EWxTeam PreviousTeam)` RepNotify 진입점을 추가했다. 현재 팀 소비자는 복제된 프로퍼티를 `GetGenericTeamId()`로 직접 읽으므로 추가 캐시나 델리게이트는 넣지 않았다.
- `WxPlayerCharacter` 파일에 diff가 없음을 확인해 Enhanced Input MappingContext 이슈는 수정 범위에서 제외했다.
- UE 5.8 `WxEditor` Win64 Development 빌드가 UHT, `WxCharacterBase.cpp` 컴파일, `UnrealEditor-WxGame.dll` 링크까지 성공했다.

# 미니언 최신 Master 발동 우선 반응

## 계획

- 승인된 대로 반응 실행 중에도 새 Master 발동을 받아 현재 분기를 중단하고 최신 태그의 분기를 다시 선택한다.
- 같은 태그도 재시작하며 BT 좌우 우선순위와 무관하게 다른 지원 스킬로 전환한다. 중단 중 새 발동은 태그 하나에 최신 값으로 보관한다.
- 추가 BB 키/번호/bool 없이 Service·데코레이터 범위에서 처리하고 ActivateAbility Task는 원본 유지한다.
- 취소 불가 및 GAS 발동 제한은 그대로 유지한다. 동일 스킬 재시작, 양방향 스킬 전환, 최신 발동 선택 및 기존 실제 에셋 테스트를 검증한다.
- UE 5.8 WxEditor Development 빌드와 작업 기록을 완료한다.

## 완료

- Service의 실행 중 감지 무시를 제거했다. 일치하는 반응이 있으면 부모 Composite에 ChildIndex=-1, Aborted로 재선택을 요청해 동일 스킬 재시작과 우선순위 무관 전환을 처리한다.
- 태그 하나의 최신 값 전달과 데코레이터의 진입 시 소비를 유지했다. 추가 BB 키/번호/실행 bool 없음. ActivateAbility Task는 HEAD 대비 차이 없음.
- 에디터 테스트에 실제 발동 횟수와 Skill 2 어빌리티를 추가해 같은 스킬이 1회→2회로 재시작되는 것, Skill 1→2 및 2→1 전환, 다음 BT 틱 전 최신 태그 선택, 취소 불가능한 기존 실행에 대한 중복 발동 거부를 검증했다.
- UE 5.8 WxEditor Development 빌드 성공. MasterAbilityReaction 및 MinionGameplaySmoke 모두 성공. 로그: Saved/Logs/LatestMasterAbilityTests.log. git diff --check 통과.
- 취소 불가 테스트 및 실제 Death 종료 시 원본 Task의 경고는 유지된다. 기존 Task를 수정하거나 GAS 취소/발동 제한을 우회하지 않았다. 수동 PIE는 수행하지 않았다.

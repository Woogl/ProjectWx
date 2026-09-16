# BP_Doppelganger 소환 및 Master 행동 복제

## 계획

- BP_Minion의 메시·재질·무기·기본 능력치를 참고해 BP_Doppelganger를 만들고 독립 전투 AI 대신 Master 행동을 복제한다.
- BP_HGTest 궁극기 몽타주에 기존 WxAnimNotify_SpawnMinion을 사용한다. MinionClass는 BP_Doppelganger, LocalSpawnOffset은 (0, 100, 0)으로 설정한다.
- 이동·회전·점프·낙하·회피를 동기화하고 충돌을 고려해 Master 우측 100cm 위치를 유지한다.
- 사용자 보충: 우측 100cm는 이동 목표이며 상대좌표 고정이 아니다. 위치·속도를 매 프레임 덮어쓰지 않고 자체 CharacterMovement의 이동 입력·가속·감속·충돌로 추종하며, 점프 시점을 복제한다.
- 추가 보충: 지상 추종으로 목표에 도달하지 못한 시간이 1초 이상이면 목표로 텔레포트한다. 도달 시 타이머를 초기화하고 공중에서는 지상 추종 시간을 누적하지 않는다.
- Master의 실제 어빌리티 발동·연속 공격·종료·취소를 복제한다. 분신의 별도 비용·쿨다운으로 누락되지 않게 하고 재귀 소환을 방지한다.
- 기존 소환수 수명·개수 관리 경로를 재사용한다.
- UE 5.8 WxEditor Development 빌드 및 BP 컴파일, 소환·추종·점프·연속 공격·재소환·Master 제거 동작을 검증한다.

## 완료

> 아래는 최초 구현 당시의 기록이다. 사용자 요청에 따라 전용 캐릭터·컴포넌트와 기존 어빌리티 수정은 제거하고 BT로 전환했다. 현재 구현·검증 결과는 `2026-09-16-doppelganger-bt.md`를 따른다.

- `/Game/Character/Doppelganger/BP_Doppelganger`와 `ABS_Doppelganger` 생성. BP_Minion의 메시·재질·애니메이션·무기와 Minion 어트리뷰트 행을 재사용하고, 독립 BT 및 퍼셉션 대신 일반 AIController와 전용 추종 컴포넌트를 사용한다.
- HGTest 궁극기 몽타주에 WxAnimNotify_SpawnMinion을 연결했다. 소환 오프셋은 (0, 100, 0)이며 분신의 소환 노티파이는 재귀 실행되지 않는다.
- 우측 100cm를 목표로 이동 입력을 주며 가속·감속·충돌은 CharacterMovement가 처리한다. 목표 15cm 이내를 도달로 판정하고, 지상 접근이 1초 이상 걸리면 TeleportTo로 복귀한다. 목적지가 겹치면 엔진의 충돌 보정을 적용하며 유효한 지점을 찾지 못하면 다음 틱에 재시도한다. 공중에서는 접근 시간을 누적하지 않는다.
- Master의 성공한 어빌리티 커밋과 종료·취소를 구독해 원본 어빌리티를 분신 ASC에서 실행한다. 연속 공격 단계, 입력 상태, 회피 방향, 타깃, 점프 시점 및 앉기를 따라가며 분신의 별도 자원·쿨다운은 소비하지 않는다. 궁극기 컷신과 처형의 대상 몽타주는 중복 실행하지 않는다.
- UE 5.8.2 WxEditor / Win64 / Development 빌드 성공. 로그: `Saved/Logs/BuildDoctor/build_2026-09-16_233604_586_15908.log`.
- 저장된 BP를 새 에디터 프로세스에서 컴파일하고 부모 클래스·참조·기본 데이터·소환 노티파이를 검증했다. 결과: `Saved/DoppelgangerAssets.json`.
- 자동 테스트 `Wx.Combat.Doppelganger.MasterReplay` 성공. 실제 BP_HGTest의 궁극기→몽타주→소환 경로, 상대좌표 고정 없는 추종, 이동·점프·착지·앉기, 무자원 연속 공격, 가드·질주·회피·Skill.2, 취소, 벽 충돌과 1초 후 텔레포트, 재귀 소환 방지, 재소환·Master 제거 정리를 검증했다. 로그: `Saved/Logs/DoppelgangerRuntimeTest.log`, 보고서: `Saved/Automation/Doppelganger/index.json`.
- 자동 테스트는 카메라 없는 독립 게임 월드에서 기존 궁극기 컷신을 생략했다. 멀티플레이 연결과 화면상의 연출은 별도 검증하지 않았다.
- 기존 GA_HGTest_Attack_Heavy_2 데이터의 쿨다운 GE 누락 로그 1건은 명시적 예상 오류로 분리했다. Skill.1은 State.Minion.Active로 발동이 금지되는 기존 소환 스킬이어서 소환 상태에서 허용되는 Skill.2를 검증했다. 두 기존 설정은 변경하지 않았다.

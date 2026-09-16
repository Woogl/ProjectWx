# 미니언 Master 어빌리티 감지와 BT 반응 전환

## 계획

- Master ASC의 기존 AbilityActivatedCallbacks를 최상위 BT Service에서 구독하고 감지 태그와 요청 번호만 전달한다. 새 GameplayEvent나 폴링, WxAI에서 WxCombat 의존성은 추가하지 않는다.
- Blackboard Master 변경과 BT 활성 수명에 맞춰 서버에서 구독·해제한다. BT 시작 후 Master가 설정되는 현재 초기화 순서를 지원한다.
- 미니언 BT가 대응 스킬과 우선순위를 결정하고 기존 ActivateAbility Task로 실행한다. 별도 소비 Task로 요청을 한 번만 처리한다.
- 최신 요청 한 건만 유지하며, 실행 중인 반응은 새 요청으로 중단하지 않는다. 반응 불가·발동 실패는 소비하고 자동 재시도하지 않는다. Master 종료에 따라 미니언 스킬을 자동 취소하지 않는다.
- 실제 Master/미니언 에셋과 Command 페이로드 의존성을 확인하고 BP·몽타주·BT를 전환한 뒤 Command 전용 코드·태그를 제거한다. 소환·소유·수명 관리는 유지한다.
- 감지·소비·발동 결과를 요청 번호와 BT 런타임 정보로 추적한다.
- UE 5.8 WxEditor Development 빌드와 변경 에셋 로드·컴파일, BP_TestHG 플레이 검증으로 연속 발동·실행 중 요청·실패·구독 수명·기존 적 BT 회귀를 확인한다.
- 기존 사용자 변경 에셋을 보존한다.

## 완료

- 완료: ObserveMasterAbility 서비스가 서버에서 Master ASC의 기존 알림을 구독하고, MasterAbility 데코레이터 → ConsumeMasterAbility → 기존 ActivateAbility 태스크로 반응한다. WxAI에서 WxCombat에 대한 참조는 추가하지 않았다.
- 추가 사용자 확인 반영: Skill 1 도중 소환된 미니언은 진행 중 어빌리티를 한 번 따라잡는다. Skill 2·강공격 후 퇴장은 미니언 BT가 담당한다.
- BB_Minion을 BB_Shared의 자식으로 추가하고 BT_Minion에 세 반응 분기·미대응 요청 폐기·대기를 연결했다. 기존 리시 이탈 퇴장을 보존하고 ROOT의 에디터 블랙보드 참조도 동기화했다.
- HGTest 몽타주 세 개의 Command 노티파이 5개, 전용 C++ 클래스·함수·이벤트 태그를 제거했다. 미니언 어빌리티의 Command 페이로드 의존성은 없었다.
- 요청 번호별 Verbose 로그와 서비스 런타임 표시를 추가했다. Death 노드만 ActivateAbility의 bWaitForAbilityEnd=false를 사용하며 다른 노드의 기본 대기 동작은 유지한다.
- 일회성 에셋 전환 도구는 Saved로 옮겨 최종 모듈에서 제거했다. 재현 가능한 에디터 회귀 테스트만 남겼다. WxEditor의 WxAI/WxCombat 의존성은 테스트용이다.
- UE 5.8.2 WxEditor Win64 Development 빌드 성공: Saved/Logs/BuildDoctor/build_2026-09-16_211900_134_28396.log.
- Wx.AI.MasterAbilityReaction 성공: 소환 시 따라잡기, 요청 소비, 같은 태그 연속 발동, 실행 중 요청 폐기, Master 종료와 미니언 실행 분리, 발동 차단 후 자동 재시도 없음, Master 교체/BT 종료 시 구독 해제.
- Wx.AI.MinionGameplaySmoke 성공: 실제 BP_HGTest Skill 1 몽타주로 BP_Minion 소환·반응, Skill 2·강공격의 미니언 스킬 정상 종료 후 BT Death 퇴장, Master 소환물 태그 해제. 헤드리스 게임 월드에서 실제 몽타주와 BT를 틱했다. 화면 연출 및 멀티플레이 네트워크 검증은 별도다.
- 최종 테스트 로그: Saved/Logs/FinalMinionReactionTests.log — 두 테스트 Success, Death 취소 경고 없음.
- 변경 에셋·관련 BP 컴파일 및 기존 BT_Template/BT_Soldier 로드 확인: Saved/Logs/VerifyMinionReactionAssets.log. 캐릭터 에셋의 Command 참조 제거 및 git diff --check 통과.
- 기존 데이터 경고: DT_Ability에 HGTest 강공격이 참조하는 GA_Attack_Heavy 행이 없다. 사용자 수정 중인 데이터테이블과 관련 어빌리티는 건드리지 않았다.
- 에디터 SourceControl 플러그인의 기존 Content/Character/Player/ 경로 경고 때문에 일부 저장 명령줄은 종료 코드 1을 냈다. Python 저장 성공 표시와 후속 에셋 로드·런타임 테스트로 저장 결과를 확인했다.
- 기존 사용자 수정 에셋과 작업 중 외부에서 삭제된 스킬 파일 상태를 보존했다.

# 미니언 BT 구조 단순화

## 계획

- 승인된 재설계: Service는 Master ASC 감지, 데코레이터는 반응 조건과 요청 소비·종료 정리, 기존 ActivateAbility Task는 실행을 담당한다.
- ConsumeMasterAbility 클래스와 요청 폐기 분기를 제거한다. 분기 실제 진입 시 데코레이터가 요청을 한 번 소비한다.
- BT_Minion은 최상위 Selector/감지 Service 아래 Skill 1 단일 태스크, Skill 2·강공격의 공격→퇴장 Sequence, 리시 이탈 퇴장, 대기만 남긴다. 연결되지 않은 기존 적 AI 노드와 불필요한 중간 Composite를 정리한다.
- 요청 번호는 디버깅용으로 유지하며 소환 시 따라잡기, 실행 중 요청 폐기, 실패 시 자동 재시도 없음, Master 종료와 미니언 실행 분리 정책을 보존한다.
- UE 5.8 WxEditor Development 빌드와 기존 MasterAbilityReaction/MinionGameplaySmoke 테스트, 최종 에셋 구조 확인으로 검증한다.
- 기존 사용자 변경은 보존한다.

## 완료

- 데코레이터의 OnNodeActivation에서 요청을 소비하도록 통합하고 ConsumeMasterAbility 클래스와 에셋 참조를 제거했다.
- BT_Minion을 다섯 분기로 정리했다. Skill 1은 단일 ActivateAbility, Skill 2·강공격은 공격→Death Sequence, 리시 이탈은 단일 Death, 나머지는 Wait다. 불필요한 Composite와 연결되지 않은 노드를 제거하고 루트 배치를 정리했다.
- 요청 번호와 기존 반응 정책을 유지하고 WxAI README를 갱신했다. 에셋 변환용 임시 소스와 의존성은 최종 소스에서 제거했다.
- UE 5.8 WxEditor Win64 Development 최종 빌드 성공.
- Wx.AI.MasterAbilityReaction 및 Wx.AI.MinionGameplaySmoke 성공: 소환 중 스킬 따라잡기, 실행 중 재요청 폐기, 실패 후 재시도 방지, Master 교체/구독 해제, 실제 HGTest Skill 2·강공격 후 퇴장 확인. 로그: Saved/Logs/SimplifiedMinionTests.log.
- 최종 에셋 로드·블루프린트 컴파일·다섯 분기 검증 성공(종료 코드 0): Saved/Logs/FinalSimplifiedAssets.log. Consume 참조 없음, git diff --check 통과.
- 검증은 헤드리스 자동화로 수행했다. 화면에서의 수동 PIE 확인은 수행하지 않았다.

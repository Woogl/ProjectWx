# Master 발동 감지의 BB 의존 제거

## 계획

- 승인된 대로 Service 인스턴스의 FGameplayTag 하나에 감지 결과를 임시 저장한다. BB MasterAbility 및 감지/실행 번호는 제거한다.
- MasterAbility 데코레이터를 일반 UBTDecorator로 바꾸고 태그 비교와 분기 진입 시 소비만 담당하게 한다.
- Service는 BT의 반응 분기 실행 여부를 확인해 실행 중 감지를 무시하며, 유효한 감지 시 해당 데코레이터의 BT 재평가를 요청한다. 별도 실행 bool은 추가하지 않는다.
- 기존 Master 키, ASC 구독/해제, 소환 시 진행 중 스킬 따라잡기, 기존 ActivateAbility Task는 유지한다.
- BB_Shared 및 BT_Minion의 저장 데이터를 정리하고 빌드와 두 자동화 테스트로 검증한다.

## 완료

- Service에 PendingAbility 태그 하나만 남기고 감지 번호/실행 번호를 제거했다. 반응 분기의 실행 여부는 IsExecutingBranch로 판단하며 실행 중 감지는 저장하지 않는다.
- MasterAbility 데코레이터를 UBTDecorator로 변경했다. 태그 비교와 진입 시 태그 소비만 수행하며 Service가 해당 데코레이터의 재평가를 요청한다.
- BB_Shared에서 이번 작업의 MasterAbility 키를 제거하고 BT_Minion의 옛 BlackboardKey 속성을 재저장으로 제거했다. 기존 BB 키 목록은 유지됐다. BB_Minion은 추가하지 않았다.
- 공유 BB 삭제 작업이 자동 검토에서 영향 범위 미확인을 이유로 거절됐으나, 원본에 해당 키가 없고 참조하는 세 BT 중 BT_Minion만 사용하는 것을 읽기 전용으로 확인한 후 재실행이 승인됐다.
- 기존 ActivateAbility Task와 WxBlackboardKeys 소스는 HEAD 대비 변경 없음 확인. 추가 bool과 요청 번호 없음.
- UE 5.8 WxEditor Development 빌드 성공. MasterAbilityReaction 및 MinionGameplaySmoke 성공: BB 키 부재, 초기 감지, 실행 중 감지 무시, 반복 발동, 실패 시 미재시도, Master 교체/구독 해제 및 실제 공격/퇴장 확인. 로그: Saved/Logs/NoAbilityBBTests.log.
- 관련 에셋 로드/블루프린트 컴파일 및 최종 BB 키 검증 성공. 로그: Saved/Logs/NoAbilityBBAssets.log. 저장 명령 종료 코드 1은 기존 SourceControl 디렉터리 누락 오류이며 Python 검증은 성공했다. 원본 Task의 Death 취소 불가 경고는 이전과 동일하다.

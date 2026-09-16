# 미니언 반응 블랙보드 키 정리

## 계획

- 승인된 범위대로 MasterAbility만 블랙보드에 남기고 감지 번호와 실행 중 번호를 인스턴스화된 감지 Service 내부로 이동한다.
- 데코레이터와 발동 Task의 내부 상태 접근을 변경하고 BB_Minion에서 두 Int 키 및 코드 accessor를 제거한다.
- 소환 시 따라잡기, 실행 중 요청 폐기, 실패 후 재시도 방지, Master 교체와 BT 중단 시 구독 해제를 유지한다.
- 기존 두 자동화 테스트를 내부 상태 접근으로 갱신하고 UE 5.8 WxEditor Development 빌드, 에셋 검증, 동일 테스트로 확인한다.
- 추가 지시 반영: BB_Minion을 제거하고 기존 BB_Shared에 MasterAbility 하나를 추가해 BT_Minion에서 사용한다.

## 완료

- 감지 번호와 현재 반응 번호를 AI별 ObserveMasterAbility Service 인스턴스로 이동했다. 데코레이터 및 Task가 실행용 BT 노드에서 Service 인스턴스를 찾아 접근한다. 서비스 디버그 표시와 로그의 번호 추적은 유지했다.
- 두 Int 블랙보드 키의 선언·accessor·필수 타입 검사와 사용을 제거했다.
- BB_Shared에 MasterAbility(Name)만 추가하고 BT_Minion의 런타임 및 에디터 루트 참조를 변경했다. BB_Minion 에셋을 제거했다.
- UE 5.8 WxEditor Win64 Development 빌드 성공. 기존 MasterAbilityReaction, MinionGameplaySmoke 모두 성공(두 키 부재 확인 포함). 로그: Saved/Logs/SingleKeyMinionTests.log.
- 관련 에셋 로드·블루프린트 컴파일 및 최종 BT 구조 검증 성공. 로그: Saved/Logs/SingleKeyAssets.log. 수동 PIE 검증은 수행하지 않았다.

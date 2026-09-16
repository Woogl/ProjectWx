# 기존 ActivateAbility Task 복원

## 계획

- 사용자 지시대로 BTTask_ActivateAbility의 이번 작업 추가분(감지 상태 접근, 추적 로그, 종료 대기 옵션)을 제거하고 기존 코드로 복원한다.
- Master 감지 및 반응 선택은 Service와 데코레이터에만 둔다. 별도 Task나 우회 발동 경로를 추가하지 않는다.
- 제거한 속성에 대한 BT 에셋 저장 데이터를 정리하고 문서를 갱신한다.
- UE 5.8 WxEditor Development 빌드 및 기존 반응/실제 미니언 테스트로 검증한다.

## 완료

- BTTask_ActivateAbility.h/.cpp를 HEAD 원본으로 복원했고 git diff --exit-code로 차이가 없음을 확인했다. Master 상태 접근, 로그, 종료 대기 옵션을 모두 제거했다.
- BT_Minion을 재저장해 제거한 옵션 데이터를 정리했다. 감지와 반응 선택은 Service/데코레이터에만 남겼으며 별도 발동 Task를 추가하지 않았다.
- UE 5.8 WxEditor Development 빌드 성공. MasterAbilityReaction 및 MinionGameplaySmoke 모두 성공. 로그: Saved/Logs/OriginalTaskMinionTests.log.
- 관련 에셋 로드/블루프린트 컴파일 검증 성공. 에셋 저장 명령의 종료 코드 1은 기존 SourceControl의 Content/Character/Player 누락 오류이며 Python 검증은 성공했다.
- 실제 퇴장은 성공하나, 기존 Task로 Death를 실행하면 BT 중단 시 취소 불가능한 Death 어빌리티에 대한 기존 경고가 발생한다. 사용자 지시대로 Task 수정이나 별도 발동 경로를 추가하지 않았다.

# 체크포인트 SaveGame 전환

- 사용자 요청: UWxCheckpointSubsystem을 제거하고 SaveGame으로 처리.
- 상태: 구현 및 Editor Development 빌드 검증 완료. 인게임 확인 대기.
- 설계·구현: UWxCheckpointSaveGame에 레벨 패키지·Transform을 직렬화한다. 기록 태스크·부활·새 게임 호출부를 전환하고 기존 서브시스템 파일을 삭제했다. 기존 Standalone/동일 맵 제한을 유지하며 사용자 후속 결정에 따라 PIE와 일반 플레이 모두 WxCheckpoint 슬롯을 사용한다. 세분화는 추후 진행한다.
- 실패 처리: 기록 실패는 StateTree Failed, 조회 불가 시 PlayerStart 부활, 새 게임 삭제 실패 시 이동 중단.
- 정적 검증: Source/Plugins/Config의 기존 클래스 참조 없음. git diff --check 통과. UHT 통과.
- 지식 반영: .wiki/raw/notes/2026-09-25-checkpoint-savegame.md, world/game 기사.
- 남은 검증: 체크포인트 활성화 후 사망·부활, 프로세스 재시작 후 같은 맵에서 부활, 새 게임 초기화, 저장 실패 및 PIE와 일반 플레이의 슬롯 공유를 인게임에서 확인해야 한다. 사람 리뷰·인게임 수용은 아직 받지 않았다.

- 빌드: WxEditor Win64 Development 성공(종료 코드 0, 248.87초). [빌드 로그](../../../Saved/Logs/BuildDoctor/build_2026-09-25_180627_941_4324.log). 변경 문서 상대 링크 검사와 Wiki 뷰어 생성도 통과했다.

- 후속 요청: RecordCheckpoint를 SaveCheckpoint로 변경. SaveGame API, StateTree 태스크·InstanceData·파일명·표시명을 변경하고 기존 StateTree 에셋을 위한 두 StructRedirects를 추가했다. WxEditor Win64 Development 재빌드 성공(종료 코드 0, 73.10초). 로그: Saved/Logs/BuildDoctor/build_2026-09-25_181539_326_29416.log. 기존 에셋의 런타임 로드는 미검증.

- 후속 결정: 저장 슬롯을 WxCheckpoint 하나로 통일. GetSlotName의 World 인자와 PIE 분기를 제거했다. 기존 WxCheckpoint_PIE 파일은 자동 이관·삭제하지 않는다.
- 단일 슬롯 변경 검증: WxEditor Win64 Development 빌드 성공(종료 코드 0, 47.10초). 로그: Saved/Logs/BuildDoctor/build_2026-09-25_183146_475_27060.log. 인게임 저장·조회는 미검증.

# Rush IgnoreCollisions

## 계획

- 승인한 설계대로 IgnoreTargetCollision을 무시할 오브젝트 종류 목록 IgnoreCollisions로 변경한다.
- 돌진 시작 시 캡슐 충돌 응답을 저장하고 선택 채널만 Ignore로 바꾼다. 정상 종료·취소 시 원래 응답을 복원한다.
- HGTest 스킬2와 분신 스킬2 몽타주의 돌진 구간에는 WorldStatic을 제외한 프로젝트의 모든 오브젝트 채널을 지정한다. 벽·지형은 WorldStatic 기준이다.
- 해당 구간에서 비대상 Pawn/동적 물체 통과, 벽 차단과 종료/취소 복원을 테스트하고 UE 5.8 WxEditor Development 빌드를 수행한다.
- 기존의 미커밋 코드·에셋 변경은 보존하며 이번 옵션과 충돌 처리만 수정한다.

## 완료

- bIgnoreTargetCollision을 `TArray<TEnumAsByte<EObjectTypeQuery>> IgnoreCollisions`로 변경했다. 빈 목록은 기존 충돌을 유지하며 선택 채널만 ECR_Ignore로 변경한다.
- 캐릭터별 Rush modifier가 시작 시 캡슐의 FCollisionResponseContainer를 저장하고 정상 완료·취소 시 복원한다. 기존 대상 액터 IgnoreActorWhenMoving 처리를 제거했다.
- 플레이어/분신 스킬2 몽타주에 WorldDynamic, Pawn, PhysicsBody, Vehicle, Destructible, WxAttack 6개를 저장했다. WorldStatic은 제외했다. 백업: `Saved/Backups/RushIgnoreCollisions/`. 저장 설정: `Saved/RushIgnoreCollisionsConfiguration.json`.
- 에셋 저장 스크립트는 성공 마커와 설정 결과를 기록했다. 커맨드릿 종료 코드는 HTTP 8000 포트 점유·기존 SourceControl 경로 경고로 1이었으나, 후속 별도 프로세스에서 두 몽타주를 로드하여 저장된 6개 채널이 프로젝트 채널 목록과 일치함을 검증했다.
- 새 CollisionChannels 테스트로 비대상 오브젝트 6종을 실제 스윕으로 통과하고 WorldStatic 벽에 차단됨을 확인했다. 정상 완료·취소·반복 취소 후 정확한 응답 복원, 빈 목록과 선택하지 않은 Trace 응답 유지도 확인했다.
- 기존 실제 몽타주 테스트에서 적 캡슐의 강제 Overlap 우회 설정을 제거했다. 이동·피해·취소 및 양쪽 충돌 복원 테스트 통과. 현재 돌진 종료 시각(약 0.333초)을 반영하도록 고정 프레임 검사를 저작된 구간 종료 기준으로 수정했다.
- UE 5.8 WxEditor Win64 Development 빌드 성공. 최종 로그: `Saved/Logs/BuildDoctor/build_2026-09-12_074905_108_10144.log`.
- 최종 자동화 4개 성공(1개 경고 없음, 3개 AI Blackboard/이름표 MVVM 초기화 경고, 실패 0). 결과: `Saved/Automation/RushIgnoreCollisionsFinal/index.json`, 로그: `Saved/Logs/RushIgnoreCollisionsFinal.log`. 변경 소스 `git diff --check` 통과.
- 벽·지형의 판단은 승인한 WorldStatic 오브젝트 채널 기준이다. 실제 플레이 UI와 네트워크 지연 환경은 별도로 실행하지 않았다.

# State.LockedOn 태그 제거

## 계획

- `State.LockedOn` 네이티브 태그 선언과 정의를 제거한다.
- 락온 어빌리티 태스크의 loose 태그 추가·제거 코드를 삭제하고 별도 대체 상태는 만들지 않는다.
- 적 네임플레이트의 `State.LockedOn` 감시와 판정을 제거해 표시 조건을 `생존 && bHasAITarget`로 단순화한다.
- 락온 상태의 단일 원본은 기존 `UWxLockOnManagerComponent::LockOnTarget`으로 유지한다.
- 코드·에셋 잔여 참조를 검색하고 WxEditor(Development) 타겟을 빌드해 검증한다.

## 완료

- `State.LockedOn` 네이티브 태그와 락온 태스크의 loose 태그 발행을 제거했다.
- 적 네임플레이트 표시 조건을 `생존 && bHasAITarget`로 단순화했으며, 별도 대체 상태는 추가하지 않았다.
- 코드·접근 가능한 게임 에셋에서 `State.LockedOn` 잔여 참조가 없음을 확인했다.
- UE 5.8.2 `WxEditor Development` 빌드에서 변경 파일(`WxGameplayTags.cpp`, `WxAbilityTask_LockOnCamera.cpp`, `WxEnemyCharacter.cpp`) 컴파일에 성공했다.
- 실행 중인 UnrealEditor가 기존 모듈 DLL을 점유해 표준 타겟의 최종 링크는 실패했다. 에디터는 임의로 종료하지 않았다. 로그: `.Codex/skills/build-doctor/logs/build_2026-08-31_024209.log`.

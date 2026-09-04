# SpawnMinion 클래스 계약 강화

## 계획

- `UWxAnimNotify_SpawnMinion::MinionClass`는 `TSubclassOf<APawn>`을 유지하고 `UGenericTeamAgentInterface` 구현 클래스로 에디터 선택을 제한한다.
- `GetMinionClass` 반환형을 실제 프로퍼티 타입인 `TSubclassOf<APawn>`으로 일치시킨다.
- `UWxMinionManagerComponent`에서 런타임에도 팀 인터페이스 구현 여부를 검증해 기존·오염된 에셋 값을 방어한다.
- ASC가 없는 소환물은 기존처럼 생성할 수 있고 명령 전달에서만 제외되는 동작을 유지한다.
- 관련 참조와 포맷을 검사하고 UE 5.8 `WxEditor` Win64 Development 빌드로 검증한다.

## 완료

- `MinionClass`의 `TSubclassOf<APawn>` 타입은 유지하면서 `MustImplement` 메타데이터로 `UGenericTeamAgentInterface` 구현 클래스만 에디터에서 선택할 수 있게 했다.
- `GetMinionClass` 반환형을 `TSubclassOf<APawn>`으로 맞추고, 지연 생성 결과도 `APawn*`으로 명확히 했다.
- 미니언 매니저가 생성과 활성 목록 변경 전에 팀 인터페이스 구현 여부를 검사하도록 런타임 방어를 추가했다.
- ASC가 없는 Pawn도 생성은 허용하되 명령 전달에서 제외하는 기존 동작은 유지했다.
- `git diff --check`를 통과했다. 줄 끝 변환 예정 경고만 있었고 공백 오류는 없었다.
- UE 5.8 `WxEditor Win64 Development` 빌드에 성공했다. 로그: `Saved/Logs/BuildDoctor/build_2026-09-04_232053_048_9964.log`

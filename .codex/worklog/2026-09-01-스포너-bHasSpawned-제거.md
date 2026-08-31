# 스포너 bHasSpawned 제거

## 계획

- `bIsKilled`만 스포너의 영속 상태로 유지하고 `bHasSpawned` 필드와 LSP 설정을 제거한다.
- 새 복원 훅이나 별도 채택 함수를 추가하지 않는다. 일반 생성 인스턴스는 비영속 `SpawnedActor` 약참조로 추적하고, LSP 복원 적은 이미 존재하는 attachment 관계로 찾는다. Pawn `Owner`는 Controller 빙의가 덮어쓰므로 수명 추적에 사용하지 않는다.
- 기존 `SpawnTarget()`에 생성 시도 판정과 진단 로그를 모으고, 기존 `OnSpawnedBy()`에서 LSP 복원 인스턴스가 처치된 스포너에 붙는 것을 거부하며 기존에 붙은 중복 인스턴스를 정리한다.
- 기존 `ShouldPersistRuntimeActor()`에서 HP 0 또는 사망 태그가 있는 적을 제외해 죽은 적의 런타임 재생성 레코드 생성을 차단한다.
- 관련 설정과 참조 잔존 여부를 정적으로 점검하고 UE 5.8 `WxEditor Development Win64` 빌드로 컴파일을 검증한다.

## 완료

- `AWxSpawner::bHasSpawned` 필드와 LSP 속성 설정을 제거하고 `bIsKilled`만 영속 상태로 남겼다.
- LSP에 없는 `AdoptSpawnedActor()`를 제거했다. Pawn `Owner`는 Controller 빙의가 덮어쓰므로 사용하지 않고, 일반 생성 인스턴스는 비영속 `SpawnedActor` 약참조로, LSP 복원 인스턴스는 기존 attachment 관계로 추적한다.
- 모든 일반 생성 시도를 기존 `SpawnTarget()`에서 판정해 처치 상태와 기존 인스턴스 때문에 건너뛴 경우를 Verbose 로그로 남긴다.
- 기존 `AWxEnemyCharacter::OnSpawnedBy()`에서 처치된 스포너의 복원 인스턴스를 제거하고, 같은 스포너의 기존 인스턴스가 있으면 경고 로그와 함께 교체하도록 했다.
- `ShouldPersistRuntimeActor()`가 HP 0 또는 `Ability.Death` 상태인 적을 거부하도록 해 죽은 적이 LSP 런타임 재생성 레코드에 들어가지 않게 했다.
- `bHasSpawned`, `AdoptSpawnedActor`, `SetOwner(Spawner)`, `Children` 잔존 참조와 변경 파일의 whitespace 오류가 없음을 정적으로 확인했다.
- UE 5.8 `WxEditor Development Win64` 빌드 성공: `C:\Wx\.Codex\skills\build-doctor\logs\build_2026-09-01_004501.log`.

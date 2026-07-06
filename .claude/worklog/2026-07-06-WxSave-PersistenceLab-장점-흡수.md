# WxSave에 PersistenceLab 샘플 장점 흡수

## 계획

### 목표

Unreal Fest 2026 PersistenceLab 샘플 분석(`Docs/Programmer/Sample_PersistenceLab_Save_System.md`)에서 확인한 장점 5개를 WxSave에 흡수한다: ① 저장 맵 기록+로드 트래블 ② 맵 이탈 메모리 플러시+복원 가드 ③ 바이트 블롭 버전 헤더 ④ PIE 자동 슬롯 ⑤ 폰 위치 저장. 이 과정에서 로드 트래블 중 스트리밍-아웃 캡처가 막 로드한 세이브를 라이브 상태로 덮어쓰는 잠재 버그도 고친다. OnPreSave 확장점·LSP 플러그인 도입·Mass/IA·크로스 세션 액터 참조는 범위에서 제외한다.

### 수정 범위

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxSave/.../Public/WxSaveGame.h` | `FWxActorRecord::VersionHeader`, `UWxSaveGame`에 저장 맵·폰 트랜스폼·컨트롤 로테이션 필드 추가 | 수정 |
| `Plugins/WxSave/.../Public/WxSaveGameSubsystem.h` | 기본 슬롯 상수, `GetSavedPawnTransform`, `LoadSlotIntoMemory`, teardown 핸들러, 로드 트래블 가드 멤버 선언 | 수정 |
| `Plugins/WxSave/.../Private/WxSaveGameSubsystem.cpp` | 맵/폰 스탬프, 로드 트래블 결정+가드, teardown 메모리 플러시, 버전 헤더 기록/적용, PIE 자동 슬롯 로드 | 수정 |
| `Plugins/WxSave/.../WxSaveGameLibrary.h/.cpp` | doc-comment 갱신 + `GetDefaultSlotName` 추가 | 수정 |
| `Source/WxGame/WorldObject/WxCheckPoint.cpp` | 슬롯 리터럴을 기본 슬롯 상수로 교체 | 수정 |
| `Source/WxGame/Framework/WxPlayerSpawningComponent.h/.cpp` | `TryGetSavedPawnSpawn` 추가(서브시스템 위임) | 수정 |
| `Source/WxGame/Framework/WxGameMode.h/.cpp` | `SpawnDefaultPawnFor_Implementation`·`FinishRestartPlayer` override로 세이브 위치/시선 우선 스폰 | 수정 |

### 접근 방식

- **저장 맵 기록**: 세이브에 PIE 접두사를 제거한 긴 패키지 이름을 스탬프하고, 로드는 그 맵으로 `ServerTravel(bAbsolute=true)`한다. 엔진 `LoadMap`이 접두사를 제거/재부여하므로 PIE·스탠드얼론 공통으로 안전하다. 미기록(구버전 파일)이면 기존처럼 현재 맵을 리로드한다.
- **맵 이탈 메모리 플러시 + 가드**: `OnWorldBeginTearDown`에서 현재 월드의 savable 전체를 메모리에만 캡처해 같은 세션 맵 왕복 상태를 유지한다. 로드로 인한 트래블 동안은 가드 플래그로 teardown 플러시·스트리밍-아웃 캡처를 모두 막아, 방금 로드한 세이브가 라이브 상태로 오염되는 것을 방지한다. 가드는 새 월드의 초기화 복원이 끝나는 지점에서 해제한다(구 월드의 모든 캡처 브로드캐스트는 그 전에 종료됨을 엔진 소스로 확인).
- **버전 헤더**: 레코드당 1개의 헤더 블롭에 패키지 파일 버전과 커스텀 버전 컨테이너(액터+컴포넌트 writer 합집합)를 기록하고, 복원 시 리더에 먼저 적용한다. 맨 `FMemoryReader`가 커스텀 버전을 현재 빌드로 리셋하는 함정을 막아 향후 세이브 포맷 마이그레이션을 가능하게 한다. 헤더가 별도 UPROPERTY라 기존 블롭 포맷은 불변이고 구 파일과 하위호환된다.
- **폰 위치 복원**: 샘플의 PlayerStartPIE 스폰 트릭 대신 GameMode 오버라이드 2점을 쓴다 — 스폰은 세이브 트랜스폼 우선, 시선은 리스타트 마무리에서 저장 값으로 덮어쓴다. 판정(유효 캡처+저장 맵=현재 맵)은 기존 위임 패턴대로 스포닝 컴포넌트가 소유하고, 실패 시 기존 PlayerStartTag 경로가 그대로 폴백이 된다.
- **PIE 자동 슬롯**: 서브시스템 Initialize에서 WorldContext가 PIE면 기본 슬롯을 메모리로만 로드한다(트래블 없음 — 최초 월드의 초기화 복원이 자연 적용). 슬롯 이름은 기존 파일·BP와 호환되게 "Test"를 상수로 유지한다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxSave/.../Public/WxSaveGame.h` | 레코드 버전 헤더 블롭 + 저장 맵·폰 트랜스폼·컨트롤 로테이션 필드 | 수정 |
| `Plugins/WxSave/.../Public/WxSaveGameSubsystem.h` | 기본 슬롯 상수(`WxSave::DefaultSlotName`), `GetSavedPawnTransform`·`LoadSlotIntoMemory`·`GetStableMapPackageName`·teardown 핸들러·가드 멤버 선언, doc 갱신 | 수정 |
| `Plugins/WxSave/.../Private/WxSaveGameSubsystem.cpp` | 맵/폰 스탬프, 저장 맵 트래블+가드, teardown 메모리 플러시, 버전 헤더 기록/적용, PIE 자동 슬롯, Dump 확장 | 수정 |
| `Plugins/WxSave/.../WxSaveGameLibrary.h/.cpp` | doc 갱신 + `GetDefaultSlotName`(BlueprintPure) | 수정 |
| `Source/WxGame/WorldObject/WxCheckPoint.cpp` | 슬롯 리터럴 → 기본 슬롯 상수 | 수정 |
| `Source/WxGame/Framework/WxPlayerSpawningComponent.h/.cpp` | `TryGetSavedPawnSpawn`(서브시스템 위임) | 수정 |
| `Source/WxGame/Framework/WxGameMode.h/.cpp` | `SpawnDefaultPawnFor_Implementation`·`FinishRestartPlayer` override — 세이브 위치/시선 우선, 기존 태그 경로 폴백 | 수정 |

### 구현·결정과 그 이유
- **맵 키를 PIE 접두사 제거 긴 패키지 이름으로 통일**: 엔진 LoadMap 이 트래블 시 접두사를 제거/재부여하므로 이 표현이 PIE·스탠드얼론 공통으로 안전하고, 동명 맵 충돌도 없다. 스탬프·일치 판정이 같은 정적 함수를 공유해 표현 불일치로 인한 조용한 복원 실패를 차단했다.
- **버전 헤더는 레코드당 1개**: 액터+컴포넌트 블롭은 캡처 시 한 빌드로 원자적으로 쓰이므로 하나로 충분하고, 레코드가 세션을 넘어 이기종 빌드로 누적되므로 파일 단위는 불가하다. 커스텀 버전 컨테이너는 archive 별로 분리돼 합집합 병합이 필요한데 같은 빌드에선 GUID 당 버전이 같아 충돌이 없다. 헤더가 별도 UPROPERTY 라 기존 블롭 포맷은 불변이고 구 파일은 헤더 부재 시 기존 경로 그대로라 하위호환이다.
- **로드 트래블 가드**: 로드 직후의 teardown 플러시·스트리밍-아웃 캡처가 막 로드한 세이브를 라이브 상태로 덮어쓰는 잠재 버그를 가드 플래그로 수정했다. 해제는 새 월드 복원 완료 지점 — 구 월드의 모든 캡처 브로드캐스트가 그 전에 끝남을 엔진 소스로 확인했다. 트래블 시작 실패 시 즉시 해제해 가드 고아화를 막는다.
- **폰 복원은 GameMode 오버라이드 2점**: 샘플의 PlayerStartPIE 스폰 트릭은 엔진 관례 의존이라, 스포닝을 직접 소유한 Wx 에서는 스폰 위치(SpawnDefaultPawnFor)와 시선(FinishRestartPlayer) 오버라이드가 더 신뢰성 높다. 판정(유효 캡처+맵 일치)은 기존 위임 패턴대로 스포닝 컴포넌트가 소유하고, 실패 시 PlayerStartTag 경로가 무변경 폴백이다.
- **PIE 자동 슬롯은 WorldContext 타입으로 판별**: PIE·스탠드얼론 모두 WorldContext 세팅 후 서브시스템 Initialize 가 호출되므로 월드 포인터 유효성 가정 없이 안전하다. 메모리 로드만 하고 트래블하지 않아 최초 월드의 초기화 복원이 자연 적용된다.

### 계획 대비 달라진 점
- LoadSlot 의 현재 맵 폴백도 `GetMapName()` 대신 안정 패키지 이름을 쓰도록 통일했다(같은 의미론, 더 견고한 표현).

### 후속 과제
- PIE 시나리오 수동 검증(가드 버그 회귀·맵 트래블·맵 왕복 플러시·자동 슬롯·구 파일 호환·WP 셀 왕복)은 에디터 실행이 필요해 미수행.
- UI BP(WBP_MainMenu/DeathScreen)의 슬롯 리터럴을 `GetDefaultSlotName` 노드로 교체(선택, 이름이 같아 미교체여도 동작 동일).
- PIE 자동 로드가 "새 게임" 테스트를 방해하면 CVar 옵트아웃 검토.

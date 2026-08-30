# WxSave PersistenceLab 방식 이식

## 계획

- 기존 저장 슬롯과의 호환은 제공하지 않고, 새 포맷 버전을 사용하는 저장 스키마로 전환한다. 구 포맷 슬롯은 로드하지 않으며 새로 저장하도록 처리한다.
- 외부 에셋이 참조하는 `UWxSaveLibrary`, `FWxStateTreeTask_SaveGame`, `UWxPlayerSpawnComponent`와 플레이어 능력치·스폰 복원 흐름은 유지한다.
- 수동 `IWxSavable` 액터/컴포넌트 바이트 직렬화를 제거하고 Unreal Engine 5.8의 Level Streaming Persistence를 월드 상태 저장·복원의 기준 구현으로 사용한다.
- 샘플의 구조에 맞춰 맵별 LSP 데이터, Instanced Actors 관리자 델타, Mass 엔티티 스냅샷을 `UWxSaveGame`에 저장하고 월드 라이프사이클에 연결한다.
- `IWxSavable`은 GUID 식별자 계약 대신 LSP 복원 완료 알림 계약으로 정리하고, `AWxDevice`와 `AWxSpawner`의 기존 복원 후처리를 보존한다.
- `WxSave` 플러그인과 프로젝트 설정에 필요한 LSP·Instanced Actors·Mass 의존성을 추가하고, 현재 영속화 대상인 장치 상태와 스포너 사망 상태만 속성 허용 목록에 등록한다.
- 기존 비동기 디스크 저장 완료 처리와 서버 권한 이동 흐름은 유지하며, 새 월드 스냅샷이 준비된 뒤 디스크 저장이 시작되도록 연결한다.
- UE 5.8 설치 경로를 런처 정보에서 확인한 뒤 `WxEditor` Development 타겟을 빌드하고, 컴파일 오류를 모두 해결한다.

## 완료

- SaveGame 포맷을 버전 2로 전환하고, 맵별 LSP 바이트·스트리밍 레벨별 IAM 델타·EntityConfig별 Mass 스냅샷을 저장하도록 구성했다.
- 구 포맷의 `ActorRecords`와 `SaveId` 직렬화 경로를 제거했다. 버전 필드가 없는 구 슬롯은 로드 시 새 저장으로 초기화한다.
- LSP 관리자를 월드 서브시스템 의존성으로 초기화하고, 명시 저장·스트리밍 아웃·맵 teardown에서 최신 상태를 플러시하도록 연결했다.
- IAM 데이터를 패키지/커스텀 버전 헤더와 함께 저장하고, `IA.DeferSpawnEntities=1` 환경에서 관리자 등록 뒤 entity spawn 전에 복원하도록 연결했다.
- Mass 스냅샷과 복원을 FrameEnd 경계에서 수행하고, EntityConfig opt-in Trait과 중복 스폰 방지용 영속 MassSpawner를 추가했다.
- 기존 BP 저장 라이브러리, StateTree 체크포인트 저장, 비동기 디스크 완료 신호, ServerTravel, 플레이어 재개 지점과 GAS 스탯 복원 흐름을 유지했다.
- LSP 속성 허용 목록에 장치 `StateTag`, 스포너 `bIsKilled`, 영속 MassSpawner의 `bHasEverSpawned` 및 이동 가능한 Pawn 루트 트랜스폼을 등록했다. 액터 파괴·런타임 재생성은 명시적으로 비활성 상태를 유지했다.
- `IWxSavable`을 LSP 복원 후처리 계약으로 정리하고 `AWxDevice`/`AWxSpawner`의 기존 런타임 후처리를 보존했다.
- `Wx.uproject`와 `WxSave.uplugin`에 Level Streaming Persistence 및 Instanced Actors 플러그인을 활성화하고 필요한 Mass 모듈 의존성을 추가했다.
- UE 5.8.2의 `WxEditor Win64 Development` 빌드에 성공했다. 전체 로그: `.Codex/skills/build-doctor/logs/build_2026-08-30_161310.log`.
- JSON 파싱, `git diff --check`, 레거시 C++ 심볼 검색을 추가로 통과했다. 에디터/PIE 런타임 시나리오 실행은 이번 검증 범위에 포함하지 않았다.

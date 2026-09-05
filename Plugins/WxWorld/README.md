# WxWorld — 월드 오브젝트 및 상호작용

> 문·상자·레버·스포너 같은 레벨 배치 오브젝트를 StateTree 로 구동하고, 플레이어가 그것들을 스캔·선택·상호작용하는 경로를 책임진다.

## 책임
**담당**
- StateTree 로 자기 상태를 도는 월드 장치(`AWxDevice`)와 그 상태의 실행·복제·복구(`UWxDeviceStateTreeComponent`)
- 플레이어 컨트롤러 쪽 상호작용 감지·선택·하이라이트와 서버로의 선택 전달(`UWxInteractionScannerComponent`)
- 레벨 배치 스포너의 스폰·처치·리스폰 상태(`AWxSpawner`, `IWxSpawnable`)
- 장치·연출을 조립하는 도메인 StateTree 태스크 모음(`StateTreeTask/`, `Interaction/`, `Spawnable/`)
- 싱글플레이 부활 지점(`UWxCheckpointSubsystem`)

**경계 (비담당)**
- `IWxInteractable` 계약 자체는 [[WxCore]] 정의(`WxInteractable.h`)를 가져다 쓴다
- 상호작용의 권위 검증(사거리·활성) 어빌리티 `WxAbility_Interact` 와 `Event.Interact` 처리는 [[WxCombat]]/GAS 측 — 스캐너는 폰 ASC 로 이벤트만 송출한다
- HUD 상호작용 리스트 표시(`UWxViewModel_InteractionList`)는 [[WxUI]]
- 스캐너 컴포넌트의 부착은 코드가 아니라 Experience 에셋 주입 설정([[WxGame]] 계열)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxDevice` | 상호작용 표면(IWxInteractable)+배선(LinkedDevices)만 남긴 장치 호스트 액터. 상태는 컴포넌트에 위임 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` |
| `UWxDeviceStateTreeComponent` | 장치 상태의 실행·복제(StateTag 스냅샷)·클라 복구를 전담 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` |
| `UWxInteractionScannerComponent` | 소유 클라에서 주변 상호작용 후보 스캔·선택·하이라이트, ServerInteract 전송 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | SpawnableActorClass 를 스폰하고 처치 상태를 보유하는 배치 액터 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스폰된 액터가 FinishSpawning 전에 스포너로부터 초기화받는 계약 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnable.h` |
| `TWxStateTreeWaitRegistry` | 통보까지 Running 으로 머무는 대기형 태스크들의 공용 등록부(템플릿) | `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h` |
| `UWxCheckpointSubsystem` | 맵 재시작을 넘겨 유지되는 싱글플레이 부활 트랜스폼 | `Plugins/WxWorld/Source/WxWorld/Public/System/WxCheckpointSubsystem.h` |
| `UWxSpawnerLibrary` | 서버 권위에서 스포너 일괄 리스폰(BP 노출) | `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h` |

## 확장 포인트 / 규약
- **새 장치**: `AWxDevice` 파생 BP 를 만들고(루트 없음 — BP 가 몸통을 세움) `UWxDeviceStateTreeComponent` 에 ST 에셋을 물린다. 태그가 루트 에셋의 유일한 상태 식별자이며, 하위 미태그 시퀀스는 상위 태그 진입 안에서 실행된다.
- **새 장치 동작**: `StateTreeTask/`·`Interaction/`·`Spawnable/` 에 `FStateTreeTaskCommonBase`(또는 유사 베이스) 파생 태스크를 추가한다. 기존 태스크(연출: `PlayAnimation`/`PlayLevelSequence`/`PlaySound`/`SpawnNiagara`/`SplineMove`/`ComponentMove`, 제어: `SendEvent`/`EnablePlayerInput`/`ApplyGameplayEffectToInteractor`, 흐름: `EnableInteraction`/`WaitForInteraction`/`TriggerSpawners`/`WaitSpawnersKilled`/`RespawnSpawners`/`RecordCheckpoint`)가 참고 틀.
- **대기형 태스크**: 통보 전까지 Running 을 유지하려면 `TWxStateTreeWaitRegistry<Payload>` 로 등록/해제/일치 완료를 처리한다(약한 실행 컨텍스트 + PIE 월드 격리 내장).
- **장치 컴포넌트 지목**: ST 에셋에서 레벨 컴포넌트는 이름으로만 가리킨다 — `FWxStateTreeComponentName`(에디터 드롭다운은 [[WxToolset]]/WxEditor 커스터마이제이션).
- **복구 vs 일회성**: 클라 상태 복구 중엔 상태 적용은 실행하되 일회성 연출은 건너뛴다 — `FWxDeviceExecutionPolicy::IsRestoring*` 로 분기.
- **리플리케이션/권한**: 장치는 최신 상태 스냅샷만 복제하고 각 피어가 ST 를 독립 실행해 수렴한다(연출을 큐로 재생하지 않음). 스캐너는 소유 클라 전용이고 상호작용 권위는 서버 어빌리티가 가진다. 스포너·체크포인트는 서버 권위.
- **스포너 설정**: `EWxSpawnerMode`(Auto/Manual), `bNeverRevive`, `UWxWorldDeveloperSettings.SpawnerClassIcons`(에디터 아이콘).

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` — 장치 액터의 계약과 상호작용 바인딩 구조. 모듈의 중심.
2. `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` — 상태 실행·복제·복구 모델. 장치가 왜 그렇게 도는지가 여기 있다.
3. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 스캔→선택→ServerInteract→ASC 이벤트까지의 상호작용 데이터 흐름(헤더 doc-comment 가 상세).
4. `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` — 스폰/처치/리스폰 사이클과 IWxSpawnable 초기화 시점.

## 관련
- 상위: 상호작용 계약과 공용 정의는 [[WxCore]], 상호작용 어빌리티는 [[WxCombat]], HUD 표시는 [[WxUI]], 컴포넌트 부착 Experience 는 [[WxGame]], 에디터 커스터마이제이션은 [[WxToolset]]

---
*문서 기준 커밋 `f826b21` · 생성일 2026-09-05 · 소스 61파일 — `/readme-writer`로 갱신*

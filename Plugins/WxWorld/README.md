# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 놓인 상호작용 장치(문·상자·체크포인트·엘리베이터·버튼)와 몬스터 스포너를 StateTree로 구동하고, 플레이어가 주변 대상을 감지·선택·발동하는 상호작용 파이프라인을 제공한다.

## 책임
**담당**
- StateTree로 자기 상태를 구동하는 월드 장치(`AWxDevice`)와 그 상태 실행·복제(`UWxDeviceStateTreeComponent`).
- 플레이어의 주변 상호작용 대상 스캔·선택·하이라이트(`UWxInteractionScannerComponent`)와 서버 발동 RPC.
- 레벨 배치 스포너(`AWxSpawner`)의 스폰·처치·리스폰과 스폰 대상 계약(`IWxSpawnable`).
- 장치·퀘스트 ST가 쓰는 월드 연출·게이트 태스크군(`Source/WxWorld/Public/StateTreeTask`, `Public/Interaction`, `Public/Spawnable`)과 그 완료 대기 등록부(`TWxStateTreeWaitRegistry`).

**경계 (비담당)**
- 상호작용의 권위 검증·사거리 판정은 `Ability.Interact` 어빌리티가 맡는다 — 스캐너는 선택만 서버로 넘긴다. 어빌리티 시스템은 [[WxCombat]] 계열.
- 상호작용 표면 계약(`IWxInteractable`)의 정의는 [[WxCore]].
- HUD 리스트 표시는 뷰모델(`UWxViewModel_InteractionList`)이 맡는다 — [[WxUI]].
- 스폰되는 몬스터·AI의 행동 자체는 [[WxAI]].

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxDevice` | `IWxInteractable` 호스트 액터. 상호작용 표면·배선(`LinkedDevices`)만 갖고 상태 구동은 컴포넌트에 위임 | `Source/WxWorld/Public/Device/WxDevice.h` |
| `UWxDeviceStateTreeComponent` | 장치 상태머신 실행기이자 복제 StateTag의 소유자. 서버 권위→클라 수렴 패턴의 중심 | `Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` |
| `UWxInteractionScannerComponent` | PlayerController 부착. 주변 후보 스캔·선택·하이라이트·`ServerInteract` | `Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | 레벨 배치 스포너. 스폰·처치 상태 보유, `Auto`/`Manual` 모드 | `Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스폰 대상 계약. FinishSpawning 이전 `OnSpawnedBy`로 초기값 주입 | `Source/WxWorld/Public/Spawnable/WxSpawnable.h` |
| `FWxStateTreeTask_WaitForInteraction` | 대상 상호작용까지 대기하는 퀘스트 게이트 태스크(폴링 없음) | `Source/WxWorld/Public/Interaction/WxStateTreeTask_WaitForInteraction.h` |
| `TWxStateTreeWaitRegistry` | 통보 올 때까지 Running으로 머무는 태스크들의 공용 등록부(템플릿) | `Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h` |
| `UWxSpawnerLibrary` | 스포너 일괄 조작 BP 함수 라이브러리(`TryRespawnAll`) | `Source/WxWorld/Public/System/WxSpawnerLibrary.h` |

## 확장 포인트 / 규약
- **장치 만들기**: `AWxDevice`를 상속한 BP에 몸통을 세우고 `UWxDeviceStateTreeComponent`에 ST 에셋·`InitialState`를 지정한다. 루트는 파생 BP가 세운다(베이스는 루트를 만들지 않음).
- **상태 구동 모델**: 상태는 서버 권위다. 권위 트리만 활성 상태 Tag를 복제 `StateTagName`에 기록하고, 클라는 그 값에 수렴하도록 라이브 전이를 요청한다. 상태 식별 키는 엔진 순정 상태 Tag(에셋 안에서 유일). 상세는 `WxDeviceStateTreeComponent.h` doc-comment.
- **월드 ST 태스크 추가**: `FStateTreeTaskCommonBase`(또는 `FStateTreeTaskBase`)를 상속하고 `Public/StateTreeTask`에 둔다. 대상 액터는 `FUniversalObjectLocator`(순정 구조체라 ST 컴파일러의 레벨 액터 참조 검증을 우회, 퀘스트 등 레벨 밖 호스트에서도 조립 가능), 대상 컴포넌트는 `FWxStateTreeComponentName`(이름 지목)으로 가리킨다.
- **대기형 태스크**: 통보 기반 완료는 `TWxStateTreeWaitRegistry`에 진입 시 등록하고 `ExitState`에서 핸들로 해제한다 — PIE 서버/클라 월드 분리를 등록부가 처리한다.
- **스폰 대상**: `IWxSpawnable`을 구현하고 `AWxSpawner::SpawnableActorClass`에 지정한다(`MustImplement`로 강제). `bNeverRevive`로 보스형 영구 처치 지정.
- **리플리케이션/권한**: 장치 상태·스폰/처치는 모두 서버 권위. 스캐너는 소유 클라 로컬 어포던스이며 발동만 `ServerInteract` RPC로 서버에 전달한다. `UWxWorldDeveloperSettings`는 에디터 스포너 아이콘 매핑(`Config=Game`).

## 여기서부터 읽어라
1. `Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` — 이 모듈 전체를 관통하는 서버 권위→클라 수렴 상태 구동 패턴의 근거.
2. `Source/WxWorld/Public/Device/WxDevice.h` — 상호작용 표면과 상태 컴포넌트가 만나는 지점, 장치의 계약 상대.
3. `Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 스캔→선택→서버 발동→어빌리티 검증으로 이어지는 상호작용 흐름 전모.
4. `Source/WxWorld/Public/Spawnable/WxSpawner.h` + `WxSpawnable.h` — 스포너/스폰 대상 계약과 리스폰 모델.

## 관련
- 상위: 상호작용 발동·검증은 [[WxCombat]](어빌리티), HUD 표시는 [[WxUI]], 스폰 몬스터 행동은 [[WxAI]]가 소비한다. 상호작용 표면 계약(`IWxInteractable`)·공용 정의는 [[WxCore]]. 장치·스포너 ST 태스크는 퀘스트([[WxQuest]]) 호스트 트리에서도 조립된다.

---
*문서 기준 커밋 `a1df17d` · 생성일 2026-09-04 · 소스 51파일 — `/readme-writer`로 갱신*

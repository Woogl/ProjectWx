# WxWorld — 월드 오브젝트 · 상호작용

> 문·상자·체크포인트·엘리베이터 같은 배치형 월드 장치와 그 상호작용, 적 스포너, 그리고 이들을 상태머신으로 구동하는 StateTree 태스크 모음을 담당한다. 장치의 상태는 서버 권위로 정해지고 클라는 복제된 상태를 추종하며, StateGame·퀘스트가 태스크로 이 세계를 연출·게이트한다.

## 책임
**담당**
- 월드 장치(`AWxDevice`)와 그 상태 구동 — StateTree 실행·상태(StateTag) 소유·복제·복원 수렴(`UWxDeviceStateTreeComponent`).
- 근접 상호작용 스캔·선택·하이라이트 — 소유 클라의 PlayerController 컴포넌트(`UWxInteractionScannerComponent`).
- 적/오브젝트 스폰과 처치 상태 보유(`AWxSpawner` + `IWxSpawnable`), 일괄 리스폰(`UWxSpawnerLibrary`).
- 장치·퀘스트 ST가 쓰는 월드 태스크 카탈로그 — 연출(애니메이션·시퀀스·사운드·나이아가라·이동), 상호작용 게이트, 스포너 제어.

**경계 (비담당)**
- 상호작용의 권위 판정(사거리·활성 검증, `Event.Interact` 처리)은 폰 ASC의 상호작용 어빌리티(GAS)가 한다 — 스캐너는 선택 액터만 서버로 원자 전송한다.
- `IWxInteractable`·`IWxSavable` 인터페이스와 `Event.Interact` 태그 정의는 [[WxCore]]. 이 모듈은 구현·발행만 한다.
- 상호작용 목록/선택의 HUD 표시는 [[WxUI]]의 상호작용 뷰모델이 `OnAnyScannerReady`로 붙어 읽는다.
- 장치·스포너 상태의 슬롯 직렬화·복원 오케스트레이션은 [[WxSave]]. 여기선 `SaveGame` 필드와 `OnSaveRestored` 후처리만 제공한다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxDevice` | 장치 공통 호스트(Abstract). 상호작용 표면·복원 신호를 받아 컴포넌트에 넘기는 계약 상대 하나 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` |
| `UWxDeviceStateTreeComponent` | 상태머신 실행기이자 StateTag 소유자. 서버 권위→클라 추종 수렴 로직의 중심 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` |
| `UWxInteractionScannerComponent` | 소유 클라 스캔·선택·하이라이트, ServerInteract RPC 진입점 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | 스폰 대상 생성·처치 상태 보유 배치 액터. 저장에 처치 상태 보존 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스폰 대상이 스포너를 주입받는 훅(`OnSpawnedBy`, FinishSpawning 이전) | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnable.h` |
| `FWxStateTreeComponentName` | ST 에셋이 레벨 컴포넌트를 이름으로 지목(직접 참조 불가 우회) | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceComponentName.h` |
| `TWxStateTreeWaitRegistry<T>` | 통보 올 때까지 Running으로 대기하는 태스크의 공용 등록부(폴링 없음) | `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h` |

## 확장 포인트 / 규약
- **새 장치**: `AWxDevice`를 상속한 BP를 만들고 `UWxDeviceStateTreeComponent`에 State Tree 에셋을 지정한다. 루트는 파생 BP가 세운다(베이스는 몸통을 만들지 않는다). 각 상태의 디테일 Tag 필드에 에셋 내 유일한 상태 태그를 달면 그 값이 곧 저장·복제·추종 키가 된다. 저장 없을 때의 시작 상태는 `InitialState`.
- **새 StateTree 태스크**: `FStateTreeTaskCommonBase`를 상속하고 `USTRUCT(meta=(DisplayName="…", Category="Wx"))`로 표기, 인스턴스 데이터 구조체와 `using FInstanceDataType`을 둔다. `GetInstanceDataType()`의 헤더 정의는 코딩 규칙 6의 명시 예외. 태스크 범주는 폴더로 나뉜다 — `StateTreeTask/`(연출·제어: 애니메이션·레벨 시퀀스·사운드·나이아가라·컴포넌트/스플라인 이동·이벤트 보내기·플레이어 입력·상호작용자 이펙트/몽타주), `Interaction/`(상호작용 켜기/대기), `Spawnable/`(스포너 발동/처치 대기/리스폰).
- **대기형 태스크**: 틱 폴링 대신 `TWxStateTreeWaitRegistry<T>`에 진입 시 등록하고, 외부 통보(예: `WaitForInteraction::NotifyInteracted`, 스포너 처치)로 약한 실행 컨텍스트를 통해 완료시킨다. `ExitState`에서 핸들로 자기 등록만 걷어낸다. PIE의 서버·클라 월드 혼선은 `NotifyWorld` 비교로 좁힌다.
- **레벨 밖 호스트(퀘스트 ST)에서 배치 액터 지목**: `FUniversalObjectLocator`를 쓴다 — 순수 구조체라 ST 컴파일러의 레벨 액터 참조 검증에 걸리지 않고 WP/PIE 해석이 엔진 내장이다.
- **리플리케이션/권한**: 장치 상태는 서버 권위. 권위 트리만 상태를 정해 `StateTagName`(FName, Replicated+SaveGame)에 기록하고, 클라·복원·레이트조인은 이 값을 진실로 삼아 라이브 전이로 수렴한다. 상호작용·스포너 처치 통보는 서버 권위 경로에서만 오므로 관련 대기 태스크는 권위 구동 ST 전용.
- **데이터 주도 설정**: 스포너 아이콘 매핑은 `UWxWorldDeveloperSettings`(Project Settings › Wx World Settings). 스캐너 부착은 코드가 아니라 Experience 에셋의 컴포넌트 주입으로 한다.

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` — 이 모듈의 심장. 서버 권위→클라 추종→복원 수렴 패턴을 여기서 이해하면 장치 전체가 풀린다.
2. `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` — 장치가 상호작용·세이브·배선(LinkedDevices)을 어떻게 컴포넌트에 위임하는지.
3. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 스캔→선택→ServerInteract→어빌리티로 이어지는 상호작용 흐름과 로컬리티 모델.
4. `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h` + `Interaction/WxStateTreeTask_WaitForInteraction.h` — 폴링 없는 대기 태스크의 표준형.

## 관련
- 상위: 장치·스포너 ST 에셋과 상호작용 어빌리티는 GameFeature/Experience 콘텐츠가 조립한다. 인터페이스·태그의 [[WxCore]], HUD의 [[WxUI]], 복원의 [[WxSave]]와 함께 본다.

---
*문서 기준 커밋 `bb06a17` · 생성일 2026-08-30 · 소스 51파일 — `/readme-writer`로 갱신*

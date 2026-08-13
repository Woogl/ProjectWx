// Copyright Woogle. All Rights Reserved.

# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 배치되는 상호작용 기믹·스포너를 담당한다. StateTree로 구동되는 기믹 상태머신, 플레이어 측 상호작용 스캐너, 처치/부활을 관리하는 스포너가 핵심이다.

## 책임
**담당**
- 기믹: `UWxGimmickStateTreeComponent` — 컴포넌트만 붙이면 어떤 액터든 상호작용 가능한 상태머신 기믹이 된다(전용 C++ 액터 불필요). 상태 구동·상호작용 계약(`IWxInteractable`)·상태 영속(`IWxSavable`)을 한 몸에 담고, 서버 권위 상태를 클라가 추종하는 복제 패턴을 구현한다.
- 기믹 StateTree 태스크군: 컴포넌트 이동/스플라인 이동, 몽타주·애니메이션·사운드·나이아가라·레벨시퀀스 재생, 입력 토글, 스포너 트리거/리스폰, GameplayEffect 적용 등 데이터 주도 기믹 저작용 재사용 태스크.
- 상호작용 StateTree 태스크: 대상의 상호작용을 여닫는 `Enable Interaction`, 성립까지 기다리는 `Wait For Interaction`. 기믹 자기 트리와 남의 트리(퀘스트 스텝) 양쪽이 같은 노드를 쓴다.
- 상호작용 스캐너: `UWxInteractionScannerComponent` — 소유 클라에서 주변 상호작용 메시를 주기 스캔해 in-range 집합·선택·하이라이트를 관리하고, 선택을 서버로 전송한다.
- 스포너: `AWxSpawner` — 레벨 배치 스폰 액터. 처치 상태(`bIsKilled`)를 GUID 키로 영속하고 부활 정책(`bNeverRevive`)을 관리한다.

**경계 (비담당)**
- 상호작용 인터페이스(`IWxInteractable`)·세이브 인터페이스(`IWxSavable`) 정의는 [[WxCore]] 소유. 본 모듈은 구현만 한다.
- 상호작용 권위 실행(사거리·활성 검증)은 폰 ASC의 `WxAbility_Interact`(GameplayAbility)가 맡는다. 스캐너는 선택 전송까지만.
- HUD 프롬프트 표시(`UWxViewModel_InteractionList` 등 뷰모델)는 [[WxUI]] 소유. 스캐너는 목록·선택 인덱스를 델리게이트로 알릴 뿐이다.
- 컴포넌트 부착·Experience 주입 설정은 GameMode/Experience 측 책임. 코드가 컨트롤러에 직접 붙이지 않는다.

## 의존성
- **주요 의존**: `WxCore`(유일한 Wx 의존 — `IWxInteractable`, `IWxSavable`). 엔진: StateTree/GameplayStateTree(기믹 상태머신·태스크), GameplayAbilities(상호작용 어빌리티 연동), ModularGameplay(`UControllerComponent`), Niagara·LevelSequence·MovieScene(연출 태스크), UniversalObjectLocator(스포너 지목).
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (`WxCore`만 참조)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxGimmickStateTreeComponent` | 기믹의 상태머신·상호작용·영속을 담는 StateTree 컴포넌트. 모듈의 중심 | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h` |
| `UWxInteractionScannerComponent` | 플레이어 컨트롤러 부착. 주변 상호작용 메시 스캔·선택·서버 전송 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | 레벨 배치 스폰 액터. 처치/부활 상태 영속 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스포너가 스폰 직후(빙의 전) 컨텍스트를 주입하는 훅 인터페이스 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnable.h` |
| `UWxSpawnerLibrary` | 월드 내 Auto 스포너 일괄 리스폰 BP 진입점 | `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h` |
| `UWxWorldDeveloperSettings` | 스포너 클래스별 에디터 아이콘 매핑 등 프로젝트 설정 | `Plugins/WxWorld/Source/WxWorld/Public/System/WxWorldDeveloperSettings.h` |
| `FWxStateTreeTask_ComponentMove` | 기믹 태스크군의 대표. 재사용 가능한 순수 비주얼 태스크의 형태 | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxStateTreeTask_ComponentMove.h` |

## 확장 포인트 / 규약
- **새 기믹 만들기**: 액터에 `UWxGimmickStateTreeComponent`를 붙이고 StateTree 에셋을 지정한다. 전이는 전부 ST 에셋이 정하며, 영속이 필요한 상태에는 상태 디테일의 Tag를 지정한다(그 Tag가 곧 세이브 키, 에셋 내 유일해야 함). 오너 액터의 `Replicates`는 배치 측이 켜야 한다.
- **새 기믹 태스크**: `FStateTreeTaskCommonBase`(또는 관련 베이스)를 상속하고 `FWxStateTreeTask_*InstanceData`를 짝지어 `Gimmick/`에 추가한다. `GetInstanceDataType()`의 인라인 반환은 코딩 규칙 6의 인정 예외(엔진 StateTree 관례).
- **스폰 대상**: `IWxSpawnable::OnSpawnedBy`를 구현하면 스포너가 빙의 전에 per-instance 컨텍스트를 주입한다. 스포너 대상 클래스는 `MustImplement=WxSpawnable` 메타로 제약된다.
- **스포너 지목 태스크**: `WxStateTreeTask_TriggerSpawnersByLocator`/`WxStateTreeTask_WaitSpawnersKilled`가 `FUniversalObjectLocator`로 특정 스포너를 지목·대기한다. 대기는 폴링이 아니라 `AWxSpawner::MarkKilled`가 부르는 통보로 판정하며, 진입 시 1회만 즉시 확인한다.
- **상호작용 토글**: `WxStateTreeTask_EnableInteraction`이 상태별 상호작용 가용성을 선언하고, 걸고 나면 완료한다. 영역 메시(바인딩)를 지목하면 프롬프트·발행자까지 기믹에 담고, 액터(UOL)를 지목하면 계약(`IWxInteractable::SetInteractionEnabled`)으로 대상에게 맡긴다 — 후자는 대상이 스트리밍 인 될 때까지만 기다린다.
- **상호작용 대기 게이트**: `WxStateTreeTask_WaitForInteraction`이 지목한 액터가 상호작용될 때까지 대기한다(퀘스트 스텝 등 외부 ST 용). 폴링이 아니라 상호작용 어빌리티가 성립 순간에 부르는 `NotifyInteracted`로 완료되며, 노드는 진입 시 자신을 대기 목록에 올리고 떠날 때 내린다.

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h` — 모듈의 중심. 헤더 doc-comment에 서버 권위/클라 추종 상태 복제 패턴이 상세히 정리돼 있다.
2. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 상호작용이 스캔→선택→서버→어빌리티로 흐르는 전체 경로를 잡는다.
3. `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` — 처치/부활 영속 모델과 IWxSavable 구현 예.

## 관련
- 상위: 상호작용 권위 실행은 [[WxCombat]]의 GameplayAbility, 프롬프트 표시는 [[WxUI]], 인터페이스·세이브 정의는 [[WxCore]]/[[WxSave]]와 맞물린다.

---
*문서 기준 커밋 `dfd2174` · 생성일 2026-08-12 · 소스 46파일 — `/readme-writer`로 갱신*

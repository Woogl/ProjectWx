# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 배치되는 상호작용 기믹(문/엘리베이터/보물상자/콘솔/컷신 트리거)과 스폰 시스템, 그리고 플레이어 상호작용 스캔·선택·프롬프트 레지스트리를 담당하는 도메인 플러그인.

## 책임
**담당**
- 상호작용 기믹의 공통 프레임: `AWxGimmick` 베이스가 서버 권위 State 태그(복제 + SaveGame)와 GimmickStateTree 구동 패턴(권위 쓰기 → 복제 → ST 이벤트 진입)을 제공한다.
- 기믹 구현체: Door / Elevator / TreasureChest / AlarmConsole / SpawnConsole / CutsceneTrigger.
- 기믹 StateTree 공용 노드 라이브러리(`WxGimmickStateTreeNodes`): 이동/애니/시퀀스/사운드/Niagara/스폰/인터랙션·입력 토글 태스크 + State 비교 조건.
- 상호작용 컴포넌트(`UWxInteractionComponent`)와 로컬 플레이어 레지스트리(`UWxInteractionRegistrySubsystem`): 볼륨 쿼리 표식, 인-레인지 수집, 선택/외곽선 강조.
- 스폰 시스템: `AWxSpawner`(처치 상태 GUID 보존, 리스폰/부활 금지) + `IWxSpawnableInterface` + `UWxSpawnerLibrary`(일괄 리스폰 BP 진입점).

**경계 (비담당)**
- 상호작용 어빌리티(스캐너 주기 스캔, TargetData 전달, 범용 몽타주) — [[WxCombat]] 계열(GAS 어빌리티 측).
- HUD 프롬프트 리스트 위젯·뷰모델(WBP_InteractionList / VM) — [[WxUI]].
- 보상 정의·지급 로직(`WxRewardTableRow`, `Wx Grant Reward` ST 태스크, `GrantReward`) — [[WxInventory]]. TreasureChest 는 보상 로우만 노출하고 지급은 위임한다.
- 공용 정의(`IWxSavable`, `IWxInteractionSource`/`FWxOnInteractedSignature`, `WxGameplayTags`, `WxCollisionChannels`) — [[WxCore]].
- 세이브 슬롯 직렬화·복원 오케스트레이션 — [[WxSave]].

## 의존성
- **주요 의존**: `WxCore`(유일한 Wx 의존). 엔진: `StateTree` + `GameplayStateTree`(기믹 상태머신), `Niagara`(FX), `LevelSequence`/`MovieScene`(컷신), `GameplayAbilities`, `GameplayTags`, `DeveloperSettings`.
- 규칙: 「WxCore 외 Wx 플러그인 참조」 검증 — 없음 ✅ (`.uplugin`·`Build.cs` 모두 Wx 중 `WxCore` 만 참조. TreasureChest 의 `WxInventory.WxRewardTableRow` 는 메타데이터 문자열 경로일 뿐 컴파일 의존이 아님).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGimmick` | 모든 기믹의 Abstract 베이스. 권위 State 커밋(`CommitGimmickState`)·복제·SaveGame·ST 이벤트 구동을 소유 | `Source/WxWorld/Public/Gimmick/WxGimmick.h` |
| `WxGimmickStateTreeNodes` | 전 기믹 공유 ST 태스크/조건 모음(이동·애니·시퀀스·사운드·Niagara·스폰·토글) | `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` |
| `UWxInteractionComponent` | 상호작용 상태·볼륨·강조 보유 SceneComponent. 서버 권위 `TryInteract` → `OnInteracted` 발화 | `Source/WxWorld/Public/Interaction/WxInteractionComponent.h` |
| `UWxInteractionRegistrySubsystem` | 로컬 플레이어별 인-레인지 수집·선택·강조 조율(HUD 리스트 소스) | `Source/WxWorld/Public/Interaction/WxInteractionRegistrySubsystem.h` |
| `AWxSpawner` | 레벨 배치 스포너. 처치 상태 GUID 보존, Auto/Manual·부활금지 | `Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnableInterface` | 스폰 대상이 구현. `OnSpawnedBy` 훅 + 에디터 프리뷰 메시 제공 | `Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h` |
| `UWxSpawnerLibrary` | `TryRespawnAll` BP 진입점(월드의 Auto 스포너 일괄 리스폰) | `Source/WxWorld/Public/System/WxSpawnerLibrary.h` |
| `UWxWorldDeveloperSettings` | 스포너 클래스별 에디터 아이콘 매핑(Config=Game) | `Source/WxWorld/Public/System/WxWorldDeveloperSettings.h` |

## Gameplay Tags
- 본 모듈은 Native Tag 를 **선언하지 않는다**. 기믹 State 태그(`Gimmick.*`)와 복원 마커 `Gimmick.Restore` 는 [[WxCore]]의 `WxGameplayTags` 에서 선언되며 여기서는 사용만 한다.

## 확장 포인트 / 규약
- **새 기믹 추가**: `AWxGimmick` 상속(Abstract), 생성자에서 컴포넌트와 초기 `State` 태그 지정, 인터랙션 핸들러에서 `CommitGimmickState(NewState)` 로만 State 를 쓴다(서버 권위 단일 진입점, 클라는 복제 추종). 비주얼/사이드이펙트는 자식 BP 에 할당한 ST 에셋이 State 태그 이벤트로 진입해 처리한다. State 쓰기는 C++ 만.
- **ST 노드 규약**: 태스크는 State 를 읽지 않는 순수 비주얼/사이드이펙트가 원칙(재사용). 초기 진입(복원/시작/레이트조인)과 라이브 전이는 `Transition.SourceStateID` 유효성으로 구분하고, 발동 FX/사운드/스폰은 복원 시 침묵(`bPlayOnRestore` 로 지속 FX 예외). 즉시완료 토글 태스크는 `bConsideredForCompletion=false` 로 재선택 루프를 피한다.
- **복원 마커**: 저장 State 진입 시 `Gimmick.Restore` 이벤트를 함께 발행 → 일회성 노드가 스냅·스킵한다.
- **새 스폰 대상**: `IWxSpawnableInterface` 구현. `AWxSpawner::SpawnableActorClass` 는 `MustImplement` 로 강제. Manual 모드 스포너는 콘솔 등 외부 트리거로만 스폰한다.
- **데이터 주도**: 기믹 이동/애니/오프셋/보상 등 세부는 ST 에셋과 액터 UPROPERTY(디자이너 편집)로 author.

## 여기서부터 읽어라
1. `Source/WxWorld/Public/Gimmick/WxGimmick.h` — 전 기믹 공통 State 구동/복제/Save 패턴. 헤더 doc-comment 가 핵심 계약을 설명한다.
2. `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` — 기믹 비주얼/사이드이펙트를 구성하는 ST 태스크·조건 카탈로그.
3. `Source/WxWorld/Public/Gimmick/WxDoor.h` — 가장 단순한 구현체로 State→ST 흐름을 구체 확인. 이후 Elevator/CutsceneTrigger 로 확장 패턴 비교.
4. `Source/WxWorld/Public/Interaction/WxInteractionComponent.h` — 상호작용 볼륨/스캔/강조/서버 권위 발화 흐름.

## 관련
- 상위/함께: 상호작용 어빌리티·스캐너 [[WxCombat]], HUD 프롬프트 [[WxUI]], 보상 [[WxInventory]], 세이브/복원 [[WxSave]], 공용 정의 [[WxCore]].

---
*문서 기준 커밋 `94f2eaf` · 생성일 2026-07-20 · 소스 29파일 — `/readme-writer`로 갱신*

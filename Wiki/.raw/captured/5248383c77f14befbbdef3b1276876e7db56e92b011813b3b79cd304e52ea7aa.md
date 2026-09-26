---
title: "장치 StateTree 정리: 복원 판정을 컴포넌트로·중복 장치 제거·몽타주 태스크 WxCombat 이관·연출 태스크 수정"
source: "MANUAL"
type: notes
ingested: 2026-09-24
tags: [wx, world, statetree, combat, architecture]
summary: "FWxDeviceExecutionPolicy를 없애고 복원 판정을 UWxDeviceStateTreeComponent::IsRestoring 정적 함수로 옮겼다. 트리 시작 복원 표시·IsRunning 오버라이드·InitialState 사전 검증·대기 등록부 템플릿을 지웠다. 상호작용자 몽타주 재생을 WxCombat '몽타주 1회 재생' 태스크로 옮겼고, 사운드는 원샷 전용, 레벨 시퀀스 카메라 컷은 당사자 피어만, 스포너 발동은 장치 복원 전이에서 스폰하지 않는다."
---

# 장치 StateTree 정리

2026-09-24 커밋 `b32c1f622`·`844010a94`·`f6b4af9d4`·`9e922a15b`·`36fbb4371`·`80e3e370b`·`8f06b1ffc`·`70495c0e7`. Wiki refresh에서 반영 누락으로 찾았고, HEAD `142fab5d6`에서 코드를 다시 읽어 대조했다. `36fbb4371`까지는 상호작용 계약 작업의 WxEditor 빌드가 이 커밋들 위에서 통과했다([상호작용 계약 통합](2026-09-24-interaction-contract-options-only.md)). 뒤의 세 커밋은 빌드 기록이 없다. 인게임 동작은 모두 확인하지 않았다.

## 복원 판정은 장치 컴포넌트의 정적 함수 (`844010a94`·`9e922a15b`)

- 별도 구조체 `FWxDeviceExecutionPolicy`(`IsRestoring`·`IsRestoringDevice`)를 지웠다. `UWxDeviceStateTreeComponent::IsRestoring(Context, Transition)`이 판정한다.
  - `Transition.SourceStateID`가 무효면 복원이다. 엔진은 트리 시작(재시작 포함) 때 소스 상태를 비워 둔다.
  - 그 밖에는 이 컴포넌트가 스냅샷을 따라 요청한 복원 전이(`bRestoringState`)일 때만 복원이다. 장치가 아닌 트리는 트리 시작만 복원이다.
- 트리 시작 때 `StartLogic`·`RestartLogic`·`EnterState`가 `TGuardValue`로 세우던 복원 표시를 지웠다. 소스 상태 무효 검사가 같은 경우를 이미 잡는다.
- `ComponentMove`·`SplineMove`는 `IsRestoringDevice`(복원 표시만)를 쓰다가 같은 `IsRestoring`으로 바뀌었다. 트리 시작에서도 목표 위치로 즉시 맞춘다.
- 헤더 계약(`WxDeviceStateTreeComponent.h:57-61`): 상태 적용(위치·표시)은 복원에서도 실행하고, 일회성 효과(사운드·보상·스폰)는 실제 진입에서만 실행한다.
- HEAD 사용처: WxWorld의 `TriggerSpawners`·`ApplyGameplayEffectToInteractor`·`ComponentMove`·`PlayLevelSequence`·`PlaySound`·`RecordCheckpoint`·`RespawnSpawners`·`SplineMove`·`TriggerLinkedDevices`. 다른 도메인의 WxCombat `PlayMontageOnce`, WxInventory `GiveRewards`·`RefillItemCharges`는 WxWorld를 참조할 수 없어 `!Transition.SourceStateID.IsValid()`만 본다.

## 이유가 사라졌거나 엔진과 중복된 장치 제거 (`844010a94`)

- `IsRunning` 오버라이드를 지웠다. 오버라이드는 순정 `bIsRunning`이 자동 완료 때 갱신되지 않아 끝난 트리로 상호작용을 보내지 않으려던 것이다.
  - `AWxDevice::GetInteractionOptions`·`GetAcceptedOptions`는 대기 노드 등록(`WaitingTask`)만 본다. 트리가 끝나거나 멈추면 대기 노드의 이탈이 등록을 걷는다(`WxDevice.h` 주석).
  - `EnterState`는 `GetStateTreeRunStatus() != Running`이면 재시작한다. 순정 `IsRunning`은 스스로 끝난 트리(Tree Succeeded 전이)에도 참이라 실행 상태를 직접 본다.
  - 트리가 끝나면 컴포넌트 틱을 끄던 `DisableTick` 호출도 지웠다.
- `InitialState` 사전 검증을 지웠다. `BeginPlay`의 존재 확인·경고, `InitialTarget` 멤버, 첫 틱의 도달 실패 경고가 없어졌다.
  - 지금은 `SynchronizeAfterStart`가 `EnterState(InitialState 태그, 복원)`를 요청한다. `EnterState`는 루트 에셋에 그 태그 상태가 없거나 트리를 시작하지 못하면 에러 로그를 남기고 false를 돌려준다. 그러면 루트 상태를 그대로 발행한다.
  - InitialState 전이는 그 요청이 깨운 틱이 발행한다.
- 스냅샷의 `FName StateTagName`을 `FGameplayTag StateTag`로 바꿨다. 변환 함수 `GetStateTag`와 `HasState`를 지웠다.

## 대기 등록부 템플릿을 상호작용 대기 안으로 (`b32c1f622`)

- `TWxStateTreeWaitRegistry`(`WxStateTreeWaitRegistry.h`)는 상호작용 대기 하나만 썼다. 이 템플릿을 지우고 `WxStateTreeTask_WaitForInteraction.cpp` 안에 등록 배열(`FWxInteractionWait`: 핸들·대상 Locator·약한 실행 컨텍스트)을 뒀다.
- 동작은 같다.
  - 핸들은 재사용하지 않는다. 늦은 해제가 다른 등록을 걷지 않는다.
  - 통보는 같은 월드의 대기만 완료한다. PIE에서는 서버·클라 월드가 한 프로세스에 있다.
  - 오너가 사라진 등록은 통보 때 걷는다. 조회(`IsAwaited`)는 걷지 않는다.

## 상호작용자 몽타주 재생을 WxCombat 태스크로 (`f6b4af9d4`)

- WxWorld `FWxStateTreeTask_PlayInteractorMontage`를 지우고 WxCombat `FWxStateTreeTask_PlayMontageOnce`(표시 이름 "몽타주 1회 재생")를 추가했다. 사용자가 효과 도메인에 두자고 제안해 적용했다.
- 이유: 도메인 사이 의존이 금지라 WxWorld 태스크는 WxCombat 어빌리티 클래스를 직접 참조하지 못했다. 효과 도메인에 두면 `UWxAbility_PlayMontageOnce`를 직접 쓰고, 장치 밖 트리에서도 쓸 수 있다. 선례는 WxInventory `GiveRewards`다.
- 입력:
  - `Target`: `Category = "Input"`. 장치 트리에서는 `Actor.InteractingCharacter`에 바인딩한다.
  - `Instigator`: 있으면 Target이 이쪽을 바라본다. 장치 트리에서는 `Actor`에 바인딩한다.
  - `Montage`
- 동작:
  - 권위에서 `GiveAbilityAndActivateOnce`로 어빌리티를 1회 부여·발동한다. 페이로드는 Instigator·Target·OptionalObject(몽타주)다.
  - 스펙이 걷히거나 비활성이 되면 Succeeded로 끝난다. 몽타주 종료·취소, Target 소멸이 여기에 해당한다.
  - 권위가 아닌 피어는 Running으로 머물며 서버가 발행하는 다음 상태를 기다린다. 그래서 이 태스크를 둔 상태와 다음 상태는 서로 다른 태그 상태여야 한다.
  - 트리 시작(소스 상태 무효)에서는 재생하지 않는다.
- 권위의 장치 복원 전이는 InitialState 적용뿐이다. 그때는 당사자가 비어 있어 이 태스크는 아무것도 하지 않는다.
- `AWxDevice::InteractingCharacter`를 `UPROPERTY(Transient)`에서 `VisibleInstanceOnly`로 바꿨다. 바인딩 피커가 편집 가능 프로퍼티만 보여 주기 때문이다.
- WxCombat은 `StateTreeModule` 모듈과 `StateTree` 플러그인을 의존에 추가했다.
- `Content` 문자열 검색에서는 두 태스크를 쓰는 에셋이 나오지 않았다. FName 분리 등 검색 한계는 있다.

## InitialState 전이와 다른 도메인 태스크 (`36fbb4371`)

- 서버는 트리를 루트로 시작한 뒤 `InitialState` 상태로 복원 전이를 요청한다. 엔진은 틱 밖의 전이 요청에 루트 프레임의 활성 상태를 소스로 채우므로 `SourceStateID`가 유효하다.
- 그래서 `IsRestoring`을 모르는 다른 도메인 태스크는 이 전이를 실제 진입으로 본다. 예를 들어 WxInventory `보상 지급`이 있는 상자를 InitialState "열림"으로 배치하면 레벨 시작 때 서버가 보상을 준다. `RefillItemCharges`도 같다.
- 현재 InitialState를 쓰는 배치는 피스톤(`Device.Piston.Off`)뿐이다. `LV_DevCombat`에 1개, `SiegeCannonEmplacement01` 레벨 인스턴스에 12개가 있어 문제가 드러나지 않는다.
- `InitialState` 필드 주석에 저작 규칙을 남겼다. "그 상태에 일회성 효과를 두지 않는다."
- 순정 해법은 `FStartParameters::SelectStateOverrideArgs`로 트리를 지정 상태에서 시작하는 것이다. 하지만 `UStateTreeComponent::StartTree`는 이 인자를 넘길 길이 없다. 쓰려면 시작 루틴과 모듈 밖에 공개되지 않은 틱 깨우기 확장(`FStateTreeComponentExecutionExtension`)을 복제해야 한다. 그래서 엔진이 컴포넌트에 시작 상태 지정을 열 때까지 보류한다(2026-09-24 사용자 결정).
- 복원까지 재시작으로 바꾸는 안은 기각했다. 공통 부모 상태가 다시 진입되어 `나이아가라 스폰`이 복원마다 중복되고, 선택 실패 시 트리가 Failed로 멈춘다.
- 근거는 WxWorld 모듈 리뷰 3번이다.

## 연출 태스크 (`80e3e370b`·`8f06b1ffc`·`70495c0e7`)

- **레벨 시퀀스 재생:** 카메라 컷은 장치 당사자를 조종하는 피어에서만 켠다(`bDisableCameraCuts = !Interactor || !Interactor->IsLocallyControlled()`).
  - 카메라 컷은 그 월드의 첫 로컬 플레이어에게 걸린다.
  - 재생은 여전히 모든 피어가 하므로 상태가 끝나는 시점(서버의 재생 종료)은 그대로다.
  - 당사자가 없는 트리에서는 카메라를 전환하지 않는다.
- **스포너 발동**(`TriggerSpawners`): 소스 상태 무효만 보던 판정을 `IsRestoring`으로 바꿨다. 이제 서버의 InitialState 적용 같은 장치 복원 전이에서도 스폰하지 않는다.
- **사운드 재생:** `bPlayOnRestore`를 지우고 라이브 발동에서만 1회 재생하는 원샷으로 정리했다.
  - 재생 핸들을 남기지 않으므로 상태를 떠나도 소리가 멈추지 않는다.
  - 그래서 복원 때 재생하는 지속 사운드는 멈출 방법이 없었다. 루프 사운드는 넣지 않는다.

## 파일별 SHA-256 (HEAD `142fab5d6`)

| 파일 | SHA-256 |
|---|---|
| `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp` | `395d2db33b12155e34937a3d83be05663d92eea98cdcc020a6af76c9e82b9a77` |
| `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` | `b0923fe71b9dc9fc9d3c74fbee45ad4034c15801d7bab428d8b2ba77e6367859` |
| `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp` | `09adbf04772d631ba18b0ef643563f7b954fcb3f76d8ce7d241e660570f53eea` |
| `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` | `476ac7406d60bf6e565e9547c89d44b71f96f8a3a91ad0d68bf469f5f8ffaf2b` |
| `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp` | `06e4ebea339f0d8d467f4fb29459ca906a287e5ff00114fce370fe8eff3971c7` |
| `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayLevelSequence.cpp` | `3f773c58f0dcf1969277d0eb4f2110f91627f94c3e2398ef5569a7f4b47e73ae` |
| `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlaySound.cpp` | `5e9ef827782b97c48afe48db54690597a21251ebb2c307399cba729351a1f26c` |
| `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_PlaySound.h` | `7eedc9190648bd938909683e66013f80f7954ba3a1501507718d5a52850bd3c3` |
| `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp` | `f8cc112164a701ab06bade7827500e160e849db14b1a7c3e46ec36abc46ba774` |
| `Plugins/WxCombat/Source/WxCombat/Private/StateTreeTask/WxStateTreeTask_PlayMontageOnce.cpp` | `7383ebb6bc26732719befcdf5b6e6383eb18adab67d78f1befa10d1bedd3acc1` |
| `Plugins/WxCombat/Source/WxCombat/Public/StateTreeTask/WxStateTreeTask_PlayMontageOnce.h` | `44d8d7b28962ff32faf32cdd9ccd855cb91da244c7f135e5c6b87be1004bd690` |
| `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs` | `b4cab7fb7a5459b1e27f96dbfe42e4db252ce2f6ced21c46fc9c10fd50e12956` |

근거: [장치 StateTree 컴포넌트](../../../Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp), [컴포넌트 헤더](../../../Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h), [장치](../../../Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp), [상호작용 대기](../../../Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp), [몽타주 1회 재생](../../../Plugins/WxCombat/Source/WxCombat/Private/StateTreeTask/WxStateTreeTask_PlayMontageOnce.cpp), [레벨 시퀀스 재생](../../../Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayLevelSequence.cpp), [사운드 재생](../../../Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_PlaySound.h), [스포너 발동](../../../Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp), [WxWorld 리뷰](../../../.agents/workflow/tasks/module_review_WxWorld.md).

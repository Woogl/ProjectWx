# WxWorld — 코드 리뷰

> 서버 권위 StateTree의 상태 수렴 경계는 비교적 명확하지만, 통보 기반 대기와 클라이언트 표시 계층에는 실제 진행 정지·표시 불일치 경로가 남아 있다. README와 모듈 의존성, 51개 C++ 소스의 규칙 준수를 훑고 장치 상태머신·상호작용 스캐너·스포너 수명·대기 등록부·주요 StateTree 태스크 구현을 깊게 봤다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 3 |
| 🟢 사소 | 1 |

## 결과

### 1. 🔴 복원된 처치 상태가 대기 등록부에 통보되지 않아 태스크가 영구 대기한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:96`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:76`
- **범주**: 버그/정확성
- **문제**: `WaitSpawnersKilled`는 진입 시 한 번 평가한 뒤 `AWxSpawner::MarkKilled()`의 통보에서만 재평가한다. 그러나 LSP 복원 후 호출되는 `AWxSpawner::OnSaveRestored()`는 `bIsKilled`가 참이면 인스턴스만 정리하고 `NotifySpawnerKilled()`를 부르지 않는다. 진입 시 언로드 상태여서 해석되지 않았던 스포너가 나중에 처치 상태로 복원되면 새 통보가 오지 않아 태스크가 끝나지 않는다. 헤더가 진입 시 언로드를 허용한다고 명시하므로 실제 조립과 충돌한다.
- **제안**: `OnSaveRestored()`에서 `bIsKilled`가 참일 때 `FWxStateTreeTask_WaitSpawnersKilled::NotifySpawnerKilled(this)`를 호출한다. 에디터 기본값 등으로 이미 처치된 채 `BeginPlay()`에 진입하는 경로도 같은 통보가 필요한지 함께 정리한다.
- **확신도**: 높음

### 2. 🟡 하이라이트 해제가 숨겨진 프리미티브를 건너뛴다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:287`
- **범주**: 버그/정확성
- **문제**: `SetActorHighlighted()`는 켜기와 끄기를 구분하지 않고 `IsVisible()`이 거짓인 프리미티브를 건너뛴다. 선택된 메시가 장치 연출이나 LOD 전환으로 숨겨진 동안 선택이 풀리면 `SetRenderCustomDepth(false)`가 적용되지 않아, 다시 보일 때 선택되지 않은 메시가 이전 외곽선을 유지한다.
- **제안**: 가시성 필터는 `bHighlighted`가 참일 때만 적용하고, 해제할 때는 모든 `UPrimitiveComponent`의 Custom Depth를 끈다.
- **확신도**: 높음

### 3. 🟡 링크된 서브트리의 상태 Tag는 발행되지만 추종·복원할 수 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:215`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:245`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:267`
- **범주**: 설계/구조
- **문제**: `GetActiveStateTag()`는 모든 활성 프레임을 역순으로 훑어 링크된 서브트리의 Tag까지 `StateTag`로 발행한다. 반면 `RequestState()`와 `HasState()`는 루트 `StateTreeRef`에서만 핸들을 찾는다. 링크된 서브트리 상태가 발행·저장되면 클라이언트 추종은 실패하고, 권위 복원은 상태를 찾지 못해 복원을 포기한다.
- **제안**: 상태 키 계약을 루트 에셋의 Tag로 제한해 발행 범위를 좁히거나, 활성 프레임의 에셋까지 추종·복원할 수 있도록 조회와 전이 요청을 대칭적으로 확장한다.
- **확신도**: 중간

### 4. 🟡 재생 중 상호작용자가 사라지면 몽타주 태스크가 실패로 끝난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp:45`
- **범주**: 버그/정확성
- **문제**: 진입 시 대상이나 몽타주가 없으면 `Succeeded`를 반환하지만, 재생 중 `InteractingCharacter`가 사망·언포제스·파괴되면 `Failed`를 반환한다. 헤더는 폴링이 대상 소실까지 정상 종료로 본다고 설명하므로 구현과 계약이 어긋나며, 실패 전이가 없는 장치 StateTree는 여기서 멈출 수 있다.
- **제안**: 재생 중 대상 소실도 진입 경로와 동일하게 `Succeeded`로 처리하고 필요하면 Verbose 로그만 남긴다.
- **확신도**: 높음

### 5. 🟢 죽은 약참조를 건너뛰어 프롬프트와 선택 인덱스가 어긋난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:79`
- **범주**: 버그/정확성
- **문제**: `GetPrompts()`는 인덱스 정합을 위해 빈 텍스트를 넣는다고 주석에 적었지만, 약참조가 이미 무효이면 항목 자체를 추가하지 않는다. 스캔 사이에 앞쪽 대상이 파괴되면 반환 배열이 당겨져 `GetSelectedIndex()`가 가리키는 액터와 프롬프트가 잠시 어긋난다.
- **제안**: 무효 약참조에도 `FText::GetEmpty()`를 추가해 `InRangeActors`와 같은 길이·인덱스를 유지한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`
- **훑은 파일**: `Plugins/WxWorld/README.md`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/Source/WxWorld/Public/` 전체 헤더, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/`의 나머지 태스크, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerLocatorUtils.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/` 전체
- **미검토 / 한계**: StateTree 에셋의 실제 Tag·전이 조립과 BP/WBP 내부는 범위 밖이다. 네트워크 수렴, 월드 파티션 스트리밍, LSP 복원 순서는 실행 재현 없이 코드 경로로 검토했다. 링크된 StateTree 에셋을 실제 장치가 사용하는지는 확인하지 못해 3번은 의도된 설계 범위일 수 있다. 51개 소스의 첫 줄 저작권, Wx 의존성, Prefix, `BlueprintCallable`, 인라인 예외 주석, 람다·델리게이트 바인딩을 검색했으며 명시된 규칙 위반은 찾지 못했다.

---
*문서 기준 커밋 `66c0f6fd` · 리뷰일 2026-08-30 · 소스 51파일 — `/module-review`로 갱신*

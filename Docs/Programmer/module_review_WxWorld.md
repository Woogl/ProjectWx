# WxWorld — 코드 리뷰

> 장치 스냅샷 동기화, 스폰 수명, 상호작용과 상태 태스크의 실패·복구 경로를 재검토했다. 현재 미커밋 변경을 포함했으며, 변경된 `bIsKilled`의 UPROPERTY 제거 자체에서는 새 결함을 찾지 못했다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 몽타주 도중 대상 소실이 정상 종료 대신 실패로 전파된다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp:46`
- **범주**: 버그/정확성
- **문제**: 진입 시 대상 부재와 틱 중 AnimInstance 부재는 Succeeded이나, 틱 중 Character 부재만 Failed이다. 헤더의 대상 소실도 종료로 본다는 계약과 어긋난다. 캐릭터 파괴 후 성공 전이만 저작된 장치에서는 다음 정상 상태로 이동하지 못하고 실패가 상위로 전파될 수 있다. 장치 컴포넌트는 완료 상태도 복제하므로 클라이언트도 정지할 수 있다.
- **제안**: 대상 소실을 Succeeded로 통일하거나, 별도 실패 상태를 요구하는 계약과 에셋 검증을 명시한다.
- **확신도**: 높음

### 2. 🟡 링크 에셋의 태그를 발행하지만 루트 에셋에서만 추종한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:196`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:438`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:454`
- **범주**: 설계/구조
- **문제**: ObserveActiveState는 모든 활성 프레임을 역순 탐색해 링크 에셋의 태그까지 선택한다. HasState와 RequestState는 루트 에셋만 조회하므로 루트에 없는 링크 태그가 스냅샷에 실리면 FollowAuthorityState가 동기화 실패를 기록한다. 동일 스냅샷으로는 재시도하지 않는다. 헤더가 선언한 루트 태그 계약을 관측 코드가 강제하지 않는다.
- **제안**: 관측 대상을 루트 에셋 프레임으로 제한하고 링크 태그 저작을 검증하거나, 상태 식별과 전이 요청에 에셋 문맥까지 포함한다.
- **확신도**: 중간

### 3. 🟡 스포너 정리가 무관한 부착 액터까지 파괴한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:173`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:178`
- **범주**: 버그/정확성
- **문제**: 추적한 SpawnedActor를 파괴한 후 직속 부착 액터를 종류나 생성 주체 확인 없이 모두 Destroy한다. 스포너에 별도 조명·연출 액터를 부착한 배치에서는 Respawn이나 EndPlay가 그 액터까지 제거한다. 추적 약참조를 보완한다는 목적보다 파괴 범위가 넓다.
- **제안**: 생성 인스턴스만 추적해 정리한다. 보완 탐색이 필요하다면 해당 스포너에서 생성한 인스턴스라는 명시적 식별을 요구한다.
- **확신도**: 높음

### 4. 🟡 애니메이션 재생의 복구 정책이 이동 태스크와 다르다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayAnimation.cpp:26`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_PlayAnimation.h:33`
- **범주**: 버그/정확성
- **문제**: PlayAnimation은 복구 중에도 처음부터 재생하지만 ComponentMove는 IsRestoringDevice일 때 목표 위치로 즉시 이동한다. 두 태스크를 조합한 장치는 레이트조인·초기 상태 복원에서 이동과 애니메이션 진행이 어긋날 수 있다. 헤더의 Component Move와 동일한 방침이라는 설명도 현재 구현과 다르다. 재선택 정책 역시 ComponentMove는 재진입을 억제하지만 PlayAnimation에는 같은 설정이 없다.
- **제안**: 복구 시 끝 포즈 적용과 재선택 시 재생 여부를 명시적으로 정하고 두 태스크의 계약을 맞춘다. 처음부터 재생하는 의도를 유지하면 헤더의 동일 정책 설명을 정정한다.
- **확신도**: 중간

### 5. 🟡 폰 입력 차단이 HUD 상호작용 요청을 차단하지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp:49`
- **범주**: 설계/구조
- **문제**: DisableInput은 폰 입력만 끈다. `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:86`의 RequestInteract는 스캐너를 직접 호출하고, 스캐너는 ServerInteract로 요청을 보내므로 이 차단을 거치지 않는다. 컷신 등에서 다른 상호작용까지 금지하려고 이 태스크를 사용하면 요청이 계속 가능하다. 현재 어빌리티의 차단 태그에도 이 태스크가 연결되어 있지 않다.
- **제안**: 전체 조작 금지가 목적이라면 서버 어빌리티와 로컬 스캐너가 함께 검사하는 상호작용 차단 상태를 추가하고, 태스크 수명 동안 유지한다.
- **확신도**: 중간

### 6. 🟡 스포너 발동이 장치의 초기 상태 복구를 일회성 실행으로 처리한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp:24`
- **범주**: 버그/정확성
- **문제**: SourceStateID의 유효성만 보고 초기 실행을 건너뛴다. 공용 IsRestoring은 장치의 bRestoringState도 검사한다. 따라서 장치가 InitialState로 전이하는 동안 SourceStateID가 유효하면 복구 중에도 Respawn을 호출해 기존 스폰 대상을 파괴하고 다시 생성할 수 있다.
- **제안**: 다른 일회성 태스크처럼 FWxDeviceExecutionPolicy::IsRestoring을 사용한다.
- **확신도**: 중간

### 7. 🟢 루프 Niagara를 상태 이탈 시 종료할 수 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp:40`
- **범주**: 설계/구조
- **문제**: 태스크는 컴포넌트를 생성하지만 ExitState 정리 경로가 없다. 자동 파괴는 시스템의 완료를 전제로 하므로 루프 FX는 상태를 떠나도 계속 재생한다. 상태에 묶인 지속 효과를 저작하려면 별도 종료 수단이 필요하다.
- **제안**: 기존 단발 효과를 보존하는 기본값으로 상태 이탈 시 Deactivate하는 선택 옵션을 제공한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 8. 🟢 공개 헤더의 기반 타입 모듈이 Private 의존성이다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h:6`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs:27`
- **범주**: 설계/구조
- **문제**: Public 헤더가 GameplayStateTreeModule의 StateTreeComponent.h를 포함하지만 해당 의존성은 Private이다. 소비 모듈이 이 공개 헤더를 포함하면 독립적인 의존성 선언 없이는 포함 경로가 보장되지 않는다. 현재 클래스가 export되지 않아 외부 소비 범위가 제한되는 점도 Public 배치와 맞지 않는다.
- **제안**: 외부 사용이 필요하면 기반 타입 모듈을 Public 의존성으로 옮기고 export 정책을 정한다. 내부 전용이면 헤더를 Private로 옮긴다.
- **확신도**: 중간

## 검토 범위

- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceExecutionPolicy.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayAnimation.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp`
- **훑은 파일**: `Plugins/WxWorld/README.md`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_PlayAnimation.h`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.h`. 상호작용 소비 경로는 `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Character/WxNpc.cpp`의 관련 호출을 검색해 대조했다.
- **미검토 / 한계**: C++ 정적 검토이며 빌드·런타임 재현·StateTree/BP/WBP 에셋 검증을 수행하지 않았다. 그 밖의 태스크 구현과 엔진 내부 전체는 재통독하지 않았다. 기존 발견 중 부모 배선 시점은 현재 저작에서의 실패 경로를 확정하지 못해 결과에서 제외했다. IsAwaited의 null 검사 부재는 확인했으나 현재 확인한 호출자는 this를 전달하며, 로케이터 재해석은 스트리밍 재생성을 반영하려는 명시적 의도가 있어 성능 측정 없이 개선 항목으로 유지하지 않았다. 나머지 기존 발견은 현재 코드로 다시 확인해 위에 정리했다. 실제 장치 배치·링크 트리 사용·루프 FX 여부에 따라 조건부 발견의 영향은 달라진다. 소스 수는 Intermediate와 생성 헤더·cpp를 제외한 h/cpp 기준이다.

---
*문서 기준 커밋 `1fab89cf4` · 리뷰일 2026-09-12 · 소스 57파일 — `/module-review`로 갱신*

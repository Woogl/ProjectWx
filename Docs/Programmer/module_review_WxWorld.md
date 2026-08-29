# WxWorld — 코드 리뷰

> 장치 상태의 권위·복제 수렴과 스포너 수명 관리는 전반적으로 경계가 잘 나뉘어 있다. 다만 멀티플레이 입력 대상 판정과 스트리밍 중 스포너 대기 완료에는 실제 진행을 막을 수 있는 결함이 있다. 이번 리뷰는 README·모듈 설정·Public/Private 헤더와 장치 상태머신, 상호작용 스캐너, 스포너, StateTree 태스크의 핵심 구현을 검토했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 2 |
| 🟡 개선 | 3 |
| 🟢 사소 | 0 |

## 결과

### 1. 🔴 다른 플레이어의 장치 전이도 각 클라이언트 자신의 입력을 차단한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp:34`
- **범주**: 버그/정확성
- **문제**: 태스크가 상호작용 당사자 대신 월드의 `GetFirstLocalPlayerController()`를 무조건 선택한다. 장치 StateTree는 권위 상태를 모든 피어가 추종해 실행하므로, 한 플레이어가 입력 차단 상태로 장치를 전이하면 각 원격 클라이언트도 자기 첫 로컬 폰을 `DisableInput` 한다. 따라서 협동 플레이에서 당사자가 아닌 플레이어까지 조작 불능이 된다.
- **제안**: 오너 `AWxDevice`의 `InteractingCharacter`와 로컬 컨트롤러의 폰이 같은 경우에만 토글하고, 스플릿스크린까지 지원해야 하면 모든 로컬 플레이어 중 당사자에 대응하는 컨트롤러를 찾는다.
- **확신도**: 높음

### 2. 🔴 언로드된 스포너가 이미 처치된 상태로 다시 로드되면 대기 태스크가 영구 대기한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:76`
- **범주**: 버그/정확성
- **문제**: 진입 시 `AreAllSpawnersKilled()`가 로케이터를 해석하지 못하면 대기 등록만 하고, 이후 재평가는 `AWxSpawner::MarkKilled()`가 보내는 통보에서만 일어난다. 대기 중 월드 파티션 셀에 들어온 스포너가 세이브로 이미 `bIsKilled=true`인 경우에는 새 `MarkKilled()` 호출이 없으므로 완료 조건이 충족되어도 태스크가 깨지지 않는다. 퀘스트·상태 전이가 그 상태에 남는다.
- **제안**: 스포너의 등록/복원 시에도 같은 대기 등록부를 재평가하도록 통보하거나, 로케이터가 미해석인 등록에 한해 스트리밍 인을 감지해 재검사하는 경로를 둔다.
- **확신도**: 높음

### 3. 🟡 죽은 약참조가 프롬프트 배열에서 빠져 선택 인덱스와 목록이 어긋난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:77`
- **범주**: 버그/정확성
- **문제**: `GetPrompts()`는 `InRangeActors`의 약참조가 무효이면 해당 항목을 추가하지 않는다. 반면 `SelectedIndex`는 원본 배열 인덱스를 유지한다. 스캔 간격 사이에 대상이 파괴되거나 스트리밍 아웃된 상태에서 뷰모델이 공개 getter로 초기값을 읽으면, HUD의 프롬프트 배열은 한 칸 당겨지고 선택한 대상과 표시 문구가 달라진다.
- **제안**: 약참조가 무효여도 `FText::GetEmpty()`를 추가해 배열 길이와 인덱스를 보존하거나, getter에서 무효 항목을 제거하면서 선택과 변경 이벤트를 함께 갱신한다.
- **확신도**: 높음

### 4. 🟡 숨겨진 프리미티브는 하이라이트 해제에서 제외되어 다시 보일 때 외곽선이 남는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:287`
- **범주**: 버그/정확성
- **문제**: `SetActorHighlighted()`는 켜기와 끄기 모두 `IsVisible()`이 거짓인 프리미티브를 건너뛴다. 선택 중 컴포넌트가 숨겨지면 `SetRenderCustomDepth(false)`가 적용되지 않고, 나중에 가시화될 때 선택되지 않은 메시가 이전 stencil 외곽선을 유지한다.
- **제안**: 가시성 필터는 켤 때만 적용하고, 해제할 때는 모든 `UPrimitiveComponent`의 Custom Depth를 끈다.
- **확신도**: 높음

### 5. 🟡 상호작용자가 재생 도중 사라지면 몽타주 태스크가 실패 분기로 전이한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp:43`
- **범주**: 버그/정확성
- **문제**: 진입 시 당사자나 몽타주가 없으면 `Succeeded`를 반환하지만, 재생 중 `InteractingCharacter`가 없어지면 `Failed`를 반환한다. 캐릭터 사망·언포제스·파괴로 재생 대상을 잃는 정상 수명 경로가 실패 전이를 타며, 실패 전이를 저작하지 않은 상태는 완료하지 못한다. 태스크 주석의 "대상이 사라진 것까지 종료로 본다"는 계약과도 다르다.
- **제안**: 당사자 소실은 진입 경로와 동일하게 `Succeeded`로 처리하고, 필요하면 진단 로그만 남긴다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp`
- **훑은 파일**: `Plugins/WxWorld/README.md`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/Source/WxWorld/Public/` 전체, `Plugins/WxWorld/Source/WxWorld/Private/`의 나머지 시스템·상호작용·스포너·StateTree 태스크 구현
- **미검토 / 한계**: StateTree 에셋의 실제 태그·전이 조립, Blueprint 이벤트 그래프와 런타임 세이브 데이터는 범위 밖이다. 네트워크 수렴과 월드 파티션 스트리밍은 코드 독해로 검토했으며 다중 클라이언트 PIE로 재현하지는 않았다.

---
*문서 기준 커밋 `b48c1930` · 리뷰일 2026-08-29 · 소스 51파일 — `/module-review`로 갱신*

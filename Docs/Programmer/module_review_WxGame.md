# WxGame — 코드 리뷰

> 이번 검토는 HEAD `08c73f51`에 더해 현재 미커밋 작업 트리의 인벤토리 조회·MVVM 수명 변경을 대상으로 한다. 승인된 PlayerController당 인벤토리 하나를 종료 후 교체하는 범위에서 신규 결함은 확인하지 못했다. 모듈 전체 재리뷰가 아니다.

## 요약

| 심각도 | 이번 변경의 신규 발견 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 0 |

## 결과

이번 변경에서 수정이 필요한 신규 결함을 확인하지 못했다.

- 최초 관찰은 PlayerController에서 직접 조회하고 `HasBegunPlay()`·`IsBeingDestroyed()`로 연결 가능 여부를 검사한다. 이후 `Ready`가 전달한 인스턴스를 직접 연결하며, `Ended`에서는 연결만 해제하고 관찰을 유지한다.
- 아이템·인벤토리 리졸버는 위젯별 뷰모델을 생성하고, 해제 대상 인스턴스의 구독만 정리한다. 고정 아이템은 인벤토리 도착 전 정적 정보를 제공하고, 종료 후 동적 표시를 초기화한다.
- `ContentsChanged`에 의한 스냅샷 갱신은 획득 통지와 분리되어 있으며, 정의가 아직 없는 슬롯은 목록에서 제외하다 정의 복제 통지 후 다시 구성한다.
- `UWxItemUseComponent`의 직접 조회는 기존 소유 Pawn의 PlayerController를 조회하는 범위를 보존한다.

## 이전 지적의 상태

- **보스 위젯 공유 구독 해제 — 해결 확인**: 현재 `UWxViewModelResolver_BossCharacter`는 위젯별 `UWxViewModel_BossDisplay`를 만들며, 그 인스턴스가 자신의 델리게이트 핸들만 제거한다. 이 판정은 기존 지적의 원인 해소 확인이며 보스 전체 동작 재리뷰는 아니다.
- **고정 아이템의 늦은 인벤토리 미연결 — 해결 확인**: `StartObserving`에서 Ready·Ended 구독을 유지하고 인벤토리가 늦게 나타나거나 새로 생성될 때 같은 뷰모델을 연결한다. 추가 정적 조회 헬퍼는 제거되어 있다.
- 아래 세 항목은 **이전 리뷰의 미해결 기록을 보존**한다. 이번 변경의 신규 발견 및 위 요약 개수에 포함하지 않으며, 관련 코드·라인 전체를 이번에 재검증하지 않았다. 위치는 이전 검토 시점 기준이다.

### 3. 🟡 MetaHuman 부착물을 제거해도 리더 메시의 애니메이션 틱 설정이 복원되지 않는다

- **위치**: `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:56`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:131`
- **범주**: 성능/안전
- **문제**: 바디를 조립할 때 리더의 `VisibilityBasedAnimTickOption`을 `AlwaysTickPoseAndRefreshBones`로 바꾸지만, `OnUnregister`는 표시만 켜고 틱 옵션을 복원하지 않는다. MetaHuman 컴포넌트만 등록 해제하거나 제거하여 리더가 계속 살아 있는 경우, 바디가 없어져도 화면 밖 본 리프레시 비용이 계속 발생한다. 월드 전체가 함께 파괴되는 경우에는 지속 비용이 없으므로 문제 범위는 리더가 생존하는 해제 경로이다.
- **제안**: 리더 설정을 변경하기 전에 원래 틱 옵션과 표시 상태를 저장하고, 해당 변경을 적용한 등록 주기의 해제에서 복원한다. 리더를 유지한 채 MetaHuman만 등록·해제해 원래 옵션으로 돌아오는지 확인한다.
- **확신도**: 높음

### 4. 🟢 입력 액션 콜백 다섯 개가 `Handle` 명명 규칙을 따르지 않는다

- **위치**: `Source/WxGame/Character/WxPlayerCharacter.cpp:113`, `Source/WxGame/Character/WxPlayerCharacter.cpp:117`, `Source/WxGame/Character/WxPlayerCharacter.cpp:126`, `Source/WxGame/Character/WxPlayerCharacter.cpp:131`, `Source/WxGame/Character/WxPlayerCharacter.cpp:132`
- **범주**: 규칙 위반
- **문제**: `AGENTS.md` 코딩 규칙 4는 델리게이트 콜백의 `Handle` 접두사를 요구한다. `BindAction`에 연결된 `Move`, `Look`, `ToggleCrouch`, `AbilityInputTriggered`, `AbilityInputReleased`는 이를 따르지 않는다. 엔진 함수명에 맞춰야 하는 `Jump`·`StopJumping`은 이 지적에서 제외한다.
- **제안**: 프로젝트가 정의한 다섯 콜백의 선언·정의·바인딩을 `Handle` 접두사로 통일한다.
- **확신도**: 높음

### 5. 🟢 일반 ViewModel 함수 네 개에 금지된 `BlueprintCallable`이 붙어 있다

- **위치**: `Source/WxGame/MVVM/WxViewModel_Item.h:47`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h:43`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h:46`, `Source/WxGame/MVVM/WxViewModel_Dialogue.h:29`
- **범주**: 규칙 위반
- **문제**: `RequestUseConsumable`, `RequestInteract`, `RequestCycle`, `RequestAdvance`는 일반 ViewModel의 함수이지만 `BlueprintCallable`로 노출되어 있다. `AGENTS.md` 코딩 규칙 5의 허용 대상인 Blueprint Function Library 또는 Blueprint Async Action 팩토리에 해당하지 않는다. `SetCurrentCategory`의 BlueprintSetter 연계와는 별개의 네 지점이다.
- **제안**: 기존 BP·MVVM 호출 사용처를 확인하여 기능을 보존하면서 허용된 Function Library 진입점 등으로 옮긴다. 지정자만 제거하여 기존 바인딩을 깨뜨리지 않는다.
- **확신도**: 높음

## 검토 범위

- **깊게 본 파일**: `Source/WxGame/MVVM/WxViewModel_Item.h`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.h`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/MVVM/Tests/WxInventoryViewModelTests.cpp`.
- **훑은 파일**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.cpp`, `Source/WxGame/MVVM/WxViewModel_BossDisplay.cpp`.
- **경계 확인**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp`의 Ready·Ended·스냅샷·슬롯/합계 통지와 조회, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`의 복제 콜백, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`의 GC 해제·비동기 이미지 취소 계약을 확인했다.
- **테스트 코드 확인**: 기존 `Wx.MVVM.Inventory.Lifecycle`·`DelayedDefinition`은 VM/소스 생성 순서 양쪽, 다른 Controller 제외, 종료 초기화, 후속 Ready 연결, 한 VM 해제 후 다른 VM 유지, 정의 도착 후 목록 갱신을 다룬다. Lifecycle은 직접 생성한 VM에 `DestroyInstance`를 호출하므로 실제 위젯을 통한 리졸버 `CreateInstance` 경로 검증은 아니다. 정의 지연도 반영 프로퍼티와 콜백으로 재현하며 실제 네트워크 패킷 지연 검증은 아니다.
- **미검토 / 한계**: 이번은 정적 코드 리뷰이며 빌드·자동화 테스트를 재실행하지 않았다. BP/WBP 내부, 실제 PIE 네트워크 지연, FieldNotify 수신측에서 동기적으로 위젯을 해제하는 재진입 흐름은 실행 검증하지 않았다. 인벤토리 복수 동시 소유는 승인된 설계 범위 밖이다. 관련 없는 기존 MetaHuman·입력·BlueprintCallable 지적은 위에 이전 기록으로 보존했다. 소스 코드는 수정하지 않았다.

---
*문서 기준 커밋 `08c73f51` · 리뷰일 2026-09-05 · 소스 78파일 — `/module-review`로 갱신*

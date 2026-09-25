# 보스 표시 VM 단순화

상태: 확인 대기 · 빌드·위젯 바인딩 확인
다음 행동: 보스 진입·사망·언로드와 보스 간 표시 전환을 확인한다.

이전 상태: 구현 완료 — 인간 코드 리뷰·인게임 확인 대기

- 날짜: 2026-09-23
- 요청: `UWxViewModel_BossDisplay`·`UWxViewModelResolver_BossCharacter`의 WxUI 이동 검토. 이어서 사용자가 현재 구조 자체가 복잡하다고 지적했다.

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 보스 표시·숨김 | 게임: 적 BP 하나의 IdentityTags에 Character.Boss를 지정하고(확인 뒤 되돌림) 교전하면 HUD 상단에 이름·HP·GP·효과가 보이고, 비교전·사망·언로드 때 숨겨진다 | 사람 | 대기 |  |
| 보스 간 전환 | 게임: 보스 둘과 동시에 교전하면 먼저 교전한 보스가 유지되고, 그 보스가 빠지면 다른 보스로 넘어간다 | 사람 | 대기 |  |
| 보스 없음 | 게임: 보스가 없으면 HUD 보스 바가 숨겨져 있다 | 사람 | 대기 |  |
| 코드 리뷰 | UWxBattleSubsystem·보스 표시 VM 변경 | 사람 | 대기 |  |

## 조사

- 두 클래스는 WxGame의 `AWxEnemyCharacter`(정적 보스 교전 이벤트, `IsBoss`)에 의존해 그대로는 WxUI로 옮길 수 없었다.
- 구조는 3단이었다: 리졸버 → BossDisplay VM → Character VM. BossDisplay VM이 다음을 모두 떠안았다.
  - 정적 이벤트 구독
  - 교전 보스 목록
  - 늦은 생성 시 액터 순회
  - 월드 필터
- 원인: "현재 싸우는 보스"라는 게임 상태를 소유하는 곳이 없어서 UI 쪽이 이를 추론했다.
- 이력:
  - 6571b5f0e: "모델이 VM을 알면 안 된다"는 지적으로 리졸버가 구독을 맡았다.
  - 08c73f513: 공유 리졸버의 `RemoveAll(this)` 버그를 고치면서 위젯별 상태가 VM으로 옮겨졌다.
- 기획(`Docs/CombatDesign/WX_첫_보스_기획서.md`, `Nameplate_System.md` 7장)은 보스전을 하나의 흐름으로 정의한다: 보스룸 입장, 컷신, 지정 위치 시작, 재도전, 페이즈.
- `bIsBoss`를 켠 에셋은 없다. `BP_Boss`는 e0e3ecc51에서 삭제되었다.

## 사용자 결정 (2026-09-23)

1. 보스 식별은 태그로 한다. `AWxCharacterBase`가 식별 태그를 보유한다.
2. 전용 위젯 클래스를 만들지 않고 기존 `WBP_Nameplate_Boss`를 활용한다. `UWxNameplateComponent`는 머리 위 표시 용도라 게시 역할에 맞지 않는다.
3. 보스전 상황은 월드 서브시스템 `UWxBattleSubsystem`이 소유한다. 서브시스템은 VM을 모르는 순수 모델로 유지한다.
4. UIManager는 보스 VM을 알지 않는다.
5. VM은 WxUI에 모은다. 구조는 세 층이다.
   - 모델: WxGame 서브시스템
   - 연결: WxGame 리졸버
   - VM: WxUI
6. `InitializeDependency(UMVVMGameSubsystem)`는 넣지 않는다. UE 5.8은 다른 서브시스템의 Initialize 안에서 `GetSubsystem`을 부르면 요청한 서브시스템을 그 자리에서 초기화한다(`SubsystemCollection.cpp:97-106`). 최종 구조에서는 글로벌 컬렉션을 쓰지 않아 해당 코드 자체가 없다.

## 구현

- **WxCore**: `Character.Boss` 태그 추가.
- **WxGame 모델**
  - `AWxCharacterBase::IdentityTags`(EditDefaultsOnly, Categories=Character) 추가. `PostInitializeComponents`에서 전 머신의 ASC에 loose 태그로 올린다.
  - `AWxEnemyCharacter`
    - 제거: `bIsBoss`, `IsBoss()`, 정적 `OnAnyBossEngagementChanged`와 델리게이트 타입.
    - `RefreshEngagement`·`EndPlay`는 교전 태그를 갱신한 뒤 `UWxBattleSubsystem::NotifyEngagementChanged`를 직접 호출한다.
  - `Source/WxGame/Battle/WxBattleSubsystem.h/.cpp` 신규 (월드 서브시스템, MVVM 의존 없음)
    - `Character.Boss`를 가진 캐릭터만 교전 순서 목록에 모은다.
    - `GetCurrentBoss()`: 맨 앞 보스.
    - `OnCurrentBossChanged`: 현재 보스가 바뀔 때만 발행한다. 새 보스가 합류해도 먼저 교전한 보스가 유지되고, 그 보스가 빠지면 다음 순서로 넘어간다.
- **WxGame 연결**: `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.h/.cpp` (같은 이름으로 다시 작성)
  - CreateInstance: 위젯을 Outer로 `UWxViewModel_Character`를 만든다. VM을 소유자로 하는 약한 델리게이트를 서브시스템에 걸고, 현재 보스를 즉시 한 번 반영한다.
  - DestroyInstance: `RemoveAll(ViewModel)`로 그 VM의 구독만 끊는다. 리졸버는 상태를 갖지 않는다.
- **삭제**: `WxViewModel_BossDisplay`.
- **WxUI**: 소스 변경 없음. 기존 `UWxViewModel_Character`를 그대로 쓴다. 중간에 UIManager·`UWxUILibrary`에 넣었던 보스 슬롯·파사드는 되돌렸다.
- **WxToolset**: `WxMVVMToolset.SetBindingSourcePath`(BlueprintCallable·AICallable) 추가.
  - 바인딩 소스 경로나 변환 함수 인자 하나의 경로만 바꾸며, 나머지 인자의 경로·기본값은 유지한다.
  - 경로 해석은 기존 `SetBindingConversionFunction`과 private `ResolvePropertyPath`로 공유한다.
- **WBP_Nameplate_Boss**
  - VM `VM_BossCharacter`를 `UWxViewModel_Character`로 재지정하고, 생성 방식을 Resolver(`UWxViewModelResolver_BossCharacter`)로 두었다.
  - 5개 경로에서 `Character` 단계를 제거했다: 효과 목록, 이름, 가시성 변환 인자, HP·GP 변환 인자.
  - 부모 클래스는 `UserWidget` 그대로다.

## 검증

- **빌드**: WxEditor Win64 Development 빌드 성공. 최종 구조 빌드: `Saved/Logs/BuildDoctor/build_2026-09-23_155341_077_16332.log`.
- **바인딩 경로 전환**: `Saved/MigrateBossNameplate.py` → `Saved/Logs/BossNameplateMigrate.log`.
  - 저장된 경로와 핀 기본값을 확인했다: HP/MaxHP, GP/MaxGP, SelfHitTestInvisible/Collapsed.
  - 전환 중 ensure 2건이 있었다. 삭제된 VM 클래스 때문에 로드 시 컴파일이 실패한 상태에서 변환 래퍼 그래프를 만들다 스켈레톤에 VM 프로퍼티가 없어 발생했다. 저장본에는 영향이 없다.
- **리졸버 전환**: `Saved/BossNameplateResolver.py` → `Saved/Logs/BossNameplateResolver.log`.
- **재검증**: `Saved/VerifyBossNameplate.py` → `Saved/Logs/BossNameplateVerify.log`. 새 프로세스에서 다음을 확인했다.
  - 컨텍스트가 Resolver 방식이고, 리졸버 클래스와 VM 클래스(`WxUI.WxViewModel_Character`)가 맞다.
  - 바인딩 5개가 있고 BossDisplay 참조가 없다.
  - `WBP_Nameplate_Boss`·`WBP_GameLayout`이 경고를 오류로 취급한 컴파일을 통과했다.
- **엔진 확인**
  - 리졸버는 위젯 클래스가 공유하고, 뷰마다 `CreateInstance`(`MVVMViewClass.cpp:150`)와 `DestroyInstance`(`:203`)를 호출한다.
  - `UObject::GetWorld`는 Outer를 따라가므로 위젯 Outer VM에서 월드를 찾는다(`Obj.cpp:1209`).
- **미실행**: 런타임·인게임 표시. 저장소에 GameInstance를 포함한 자동화 하네스가 없고, 보스 콘텐츠도 없다.

## AI 코드 리뷰 (2026-09-23, 최종 구조)

`code-review` 스킬로 후보 12건을 받았고, 주요 항목은 엔진 코드로 직접 확인했다. 사용자 승인으로 아래를 반영했다.
- 서브시스템: 보스 태그 판정을 추가할 때만 한다. 교전 중 태그가 빠져도 비교전 알림으로 목록에서 내려간다.
- `AWxEnemyCharacter`: `UWorld::GetSubsystem<>(GetWorld())`로 바꿔 월드가 null이어도 안전하게 했다.
- `IdentityTags`: `SetLooseGameplayTagCount(Tag, 1)`로 여러 번 실행돼도 결과가 같게 했다. 스트리밍 레벨이 다시 보이면 `PostInitializeComponents`가 재실행되기 때문이다(`Actor.cpp:3238`, `Level.cpp:3876`).
- `ability-resolver-to-wxui.md`: 보스 이동 제약이 해소되었다는 후속 메모를 추가했다.

이 수정분은 16:12:54 빌드(`build_2026-09-23_161254_241_18748.log`, 동시에 실행된 다른 빌드)에서 컴파일·링크까지 성공했다.

반영하지 않은 항목:
- **기존 버그, 별도 작업 권장**: 같은 이유로 사망·래그돌 태그 구독이 중복되어 `OnDeath`가 두 번 발행되고, 보상이 두 번 지급될 수 있다.
- **설계 한계, 회귀 아님**: 보스 판정이 로컬 플레이어와 연결되지 않고, 락온 대상이 잠깐 끊기면 순서가 바뀐다. 이전 BossDisplay와 동작이 같다.
- **경미·선택**:
  - DestroyInstance에서 VM을 Deinitialize하지 않는다.
  - 리졸버가 `ExpectedType`을 무시한다. 기존 PlayerCharacter 리졸버와 같은 관례다.
  - `IdentityTags`는 베이스에 있지만 교전 알림은 적만 보낸다.
  - `WxMVVMToolset`: D→S 방향 미지원, 소스 이름을 잘못 쓰면 모호한 오류, 중복 코드.

## Wiki 반영 (2026-09-23)

- 사용자 요청으로 인게임 확인 전에 반영했다. 보스 표시 동작에는 "인게임 미검증"을 표기했다.
- 원자료 `.wiki/raw/notes/2026-09-23-boss-battle-three-layer.md`를 수집하고, `wiki/topics/ui.md`(세 층 규칙·보스 바), `wiki/topics/game.md`(보스전 상태·식별 태그·재초기화 문제), `wiki/references/editor-tools.md`(MVVM 소스 경로 도구·전환 함정)에 편찬했다.
- 수정한 4개 문서의 상대 링크 67개가 모두 존재함을 확인했다. PowerShell 7이 없어 뷰어(`Export-Wiki.ps1`)는 갱신하지 못했다.

## 인게임 확인 방법 (인간 확인 필요)

1. 적 BP 하나의 `IdentityTags`에 `Character.Boss`를 지정한다. 확인 후 되돌린다.
2. 교전 시 HUD 상단에 이름·HP·GP·효과가 표시되고, 비교전·사망·언로드 시 숨겨지는지 확인한다.
3. 보스 둘을 동시에 교전시킨다. 먼저 교전한 보스가 유지되고, 그 보스가 빠지면 다른 보스로 넘어가는지 확인한다.
4. 보스가 없을 때 HUD 보스 바가 숨겨져 있는지(Collapsed) 확인한다.

## 남은 제약

- 보스전 시작 신호는 여전히 AI 교전 상태다. 기획의 보스룸 입장·컷신·재도전 흐름은 미구현이다. 서버 권위 상태가 필요해지면 복제되는 액터(보스룸 액터나 GameState 컴포넌트)가 소유하고, 서브시스템은 그것을 모으는 역할로 둔다.
- WxGame에 남은 `WxViewModel_InteractionList`·`WxViewModel_Inventory`도 같은 세 층 규칙으로 WxUI에 옮길 수 있다. 다만 둘 다 따로 설계가 필요하다.
  - VM에서 도메인으로 가는 명령(`RequestInteract`, `RequestUseConsumable`)의 전달 방식.
  - Inventory 공개 헤더의 `UWxItemDefinition` 의존.

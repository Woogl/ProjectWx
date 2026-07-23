# 상호작용 IWxInteractable 인터페이스 도입 — 응답+프롬프트를 대상이 직접 제공

## 계획

### 목표
상호작용 효과를 `UWxInteractionComponent::OnInteracted`(dynamic 델리게이트) 바인딩 대신, 대상이 WxCore 계약 인터페이스 `IWxInteractable`를 직접 구현하는 단일 구조로 바꾼다. 확장성↑·대상 코드 종속성↓, 순수 C++(BP 없음), StateTree/State 권위는 불변.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCore/.../Public/WxInteractable.h` | `IWxInteractable`(`OnInteracted`, `GetInteractionPrompt`) 신설 | 신규 |
| `Plugins/WxCore/.../Public/WxInteractionSource.h` | 파일 삭제(`FWxOnInteractedSignature` 포함) | 삭제 |
| `Plugins/WxWorld/.../Interaction/WxInteractionComponent.{h,cpp}` | 델리게이트·텍스트·`IWxInteractionSource` 제거, 순수 감지로 축소. `TryInteract`가 소유자 `IWxInteractable::OnInteracted` 호출 | 수정 |
| `Plugins/WxWorld/.../Interaction/WxInteractionRegistryComponent.cpp` | `GetPrompts`가 `IWxInteractable::GetInteractionPrompt`로 pull | 수정 |
| `Plugins/WxWorld/.../Gimmick/WxGimmick.{h,cpp}` | `public IWxInteractable` 추가, `InteractionPrompt` 필드+기본 `GetInteractionPrompt`, `OnInteracted` pure | 수정 |
| `Plugins/WxWorld/.../Gimmick/{WxTreasureChest,WxAlarmConsole,WxSpawnConsole,WxCutsceneTrigger,WxDoor,WxElevator}.{h,cpp}` | 델리게이트 핸들러→`OnInteracted` override(엘베는 `Source` 분기) | 수정 |
| `Source/WxGame/WorldObject/{WxLaserCorridor,WxCheckPoint}.{h,cpp}` | 동상 | 수정 |
| `Source/WxGame/Character/WxEnemyCharacter.{h,cpp}` | 비기믹, `public IWxInteractable`+`OnInteracted`/`GetInteractionPrompt` | 수정 |
| `Plugins/WxInventory/.../Items/WxItemPickup.{h,cpp}` | 비기믹, `IWxInteractable` 구현, `SetInteractionText`/자동바인딩 제거, 프롬프트는 `GetInteractionPrompt` | 수정 |
| `Docs/Programmer/Interaction_System.md`, `Gimmick_State_Authority.md`, 각 README | 델리게이트→인터페이스·프롬프트 pull 갱신 | 수정 |

### 접근 방식
- **응답 디스패치**: `TryInteract`(서버 권위 가드 이후)가 `Cast<IWxInteractable>(GetOwner())->OnInteracted(Instigator, this)` 호출. 대상별 핸들러는 인터페이스 override로 이동, `BeginPlay`의 `AddDynamic` 제거.
- **다중 영역**: `Source`(엔진 타입 `UActorComponent*`)로 엘리베이터가 영역을 가른다(구현부에서 `UWxInteractionComponent`로 캐스트해 멤버 비교).
- **프롬프트 pull**: 레지스트리가 컴포넌트 텍스트 대신 대상의 `GetInteractionPrompt(Source)`를 읽는다. 컴포넌트에서 텍스트 제거 → `IWxInteractionSource` 삭제. `AWxGimmick`에 `InteractionPrompt` 흡수.
- **StateTree 불변**: 기믹 `OnInteracted`는 여전히 `CommitGimmickState(태그)`만 호출 → State 복제·OnRep·ST 이벤트·에셋·태스크 전부 그대로.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCore/.../Public/WxInteractable.h` | `IWxInteractable`(`OnInteracted`, `GetInteractionPrompt`) | 신규 |
| `Plugins/WxCore/.../Public/WxInteractionSource.h` | 삭제 | 삭제 |
| `Plugins/WxWorld/.../Interaction/WxInteractionComponent.{h,cpp}` | 델리게이트·텍스트·`IWxInteractionSource` 제거, `TryInteract`가 소유자 인터페이스 호출 | 수정 |
| `Plugins/WxWorld/.../Interaction/WxInteractionRegistryComponent.cpp` | 프롬프트를 `GetInteractionPrompt`로 pull | 수정 |
| `Plugins/WxWorld/.../Gimmick/WxGimmick.{h,cpp}` | `IWxInteractable` 추가, `InteractionPrompt`+기본 `GetInteractionPrompt`, `OnInteracted`은 `PURE_VIRTUAL` | 수정 |
| `Plugins/WxWorld/.../Gimmick/{WxTreasureChest,WxAlarmConsole,WxSpawnConsole,WxCutsceneTrigger,WxDoor,WxElevator}.{h,cpp}` | 델리게이트 핸들러→`OnInteracted` override(엘베는 `Source` 분기), `AddDynamic`·빈 `BeginPlay` 제거 | 수정 |
| `Source/WxGame/WorldObject/{WxLaserCorridor,WxCheckPoint}.{h,cpp}` | 동상(기믹) | 수정 |
| `Source/WxGame/Character/WxEnemyCharacter.{h,cpp}` | 비기믹, `IWxInteractable` 구현, ctor `SetInteractionText` 제거 | 수정 |
| `Plugins/WxInventory/.../Items/WxItemPickup.{h,cpp}` | 비기믹, `IWxInteractable` 구현, 자동바인딩·`UpdateInteractionText` 제거 | 수정 |
| `Source/WxGame/Controller/WxPlayerController.cpp` | 선택 프롬프트를 `GetInteractionPrompt`로 pull | 수정 |
| `Docs/Programmer/{Interaction_System,Gimmick_State_Authority}.md`, `Plugins/{WxCore,WxWorld,WxInventory}/README.md` | 델리게이트→인터페이스·프롬프트 pull 반영 | 수정 |

### 구현·결정과 그 이유
- **응답 디스패치를 델리게이트에서 인터페이스로**: 대상마다 `BeginPlay`에서 델리게이트를 바인딩하던 배관을 없애고, 감지 컴포넌트가 소유자의 계약 메서드를 직접 부른다. 배선 보일러플레이트가 사라지고, 순수 C++ 호출이라 구현으로 곧장 스텝인된다(사용자 요구인 C++ 제어 + 디버깅 우위).
- **추상 베이스라도 응답 메서드에 본체를 준다**: 엔진은 `UCLASS(Abstract)`에도 CDO를 만들므로 진짜 순수 가상은 CDO 생성을 막는다(빌드 실패). 미구현 계약은 `PURE_VIRTUAL`로 표시해 인스턴스화는 허용하되 미override 시 런타임에 걸리게 했다(`IWxSavable`과 같은 관례).
- **프롬프트를 push에서 pull로**: 컴포넌트가 텍스트를 들고 크로스플러그인 setter로 갱신하던 구조 대신, 대상이 프롬프트를 제공하고 레지스트리가 스캔 때 읽는다. 덕분에 텍스트 setter 계약이 불필요해져 대상 계약을 인터페이스 하나로 통합하고 컴포넌트를 순수 감지로 축소, 계약 인터페이스를 완전히 제거했다.
- **다중 영역은 소스 컴포넌트로 가른다**: 인터페이스가 상호작용을 일으킨 컴포넌트를 함께 넘겨(WxCore가 볼 수 있는 엔진 타입), 한 액터의 여러 영역을 구현부가 구분한다. 엘리베이터의 세 콜백이 단일 응답의 분기로 합쳐졌다.
- **중복 권위 가드 제거**: 실행 진입점이 이미 서버 권위에서만 응답을 부르므로 각 대상의 권위 가드는 잉여였다(상태 커밋 경로에도 자체 가드가 있음). 응답 본문을 효과 자체로 단순화했다.

### 계획 대비 달라진 점
- **베이스 응답 메서드**: 계획은 `OnInteracted`을 베이스에서 순수 가상으로 두려 했으나 CDO 제약으로 `PURE_VIRTUAL` 본체 제공으로 변경(빌드 검증에서 발견·수정).
- **다중 영역 프롬프트**: 계획의 "영역별 프롬프트 override"는 보류하고 베이스의 단일 프롬프트 필드를 세 영역이 공유하게 뒀다. 영역별 텍스트가 필요해지면 그 대상만 프롬프트 게터를 override 하면 된다.

### 후속 과제
- **BP 데이터 재저작(미검증)**: 프롬프트 데이터가 컴포넌트에서 대상 액터로 이동했다. 기존 기믹 BP가 컴포넌트에 커스텀 프롬프트를 설정했다면 액터 defaults로 옮겨야 한다(엔진은 사라진 컴포넌트 프로퍼티를 조용히 버린다).
- **런타임 검증(미실행)**: 컴파일만 확인했다. PIE 단일/리슨호스트에서 상자·픽업·엘리베이터 3영역·문·체크포인트·처형 실동작과 HUD 프롬프트 표시는 아직 인게임 확인 전.
- **`Interaction_System.md` 잔여 stale**: 실행 절이 이번 변경과 무관하게 서버 전용 전환(2026-07-22) 이전 서술을 남기고 있어 별도의 폭넓은 갱신이 필요하다.

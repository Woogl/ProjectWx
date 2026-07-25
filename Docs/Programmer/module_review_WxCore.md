# WxCore — 코드 리뷰

> 선언·상수·계약만 담는 foundation 모듈이라 전반적으로 매우 깨끗하다 — 심각(🔴) 결함은 없고, 로직을 가진 유일한 파일(`WxInteractable.cpp`, 11줄)에 계약의 암묵 전제가 몰려 있다. 소스 8개 전부를 통독했고, 신규 진입한 `IWxInteractable`은 엔진(UE 5.8 `OverlapComponent` 구현)·소비 도메인(WxWorld/WxGame/WxDialogue/WxInventory)·`Config/DefaultEngine.ini`까지 교차 확인했다. Gameplay Tag 95개는 선언·정의 1:1 및 변수명↔문자열 치환 규칙을 스크립트로 전수 대조해 불일치 0을 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 `IsMeshInRange`가 쿼리 콜리전·피직스 애셋 부재 시 조용히 false를 돌려준다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:18`
- **범주**: 버그/정확성
- **문제**: 판정이 `Mesh->OverlapComponent(...)` 하나에 달려 있는데, 이 호출은 컴포넌트에 물리 바디가 있어야만 참을 낼 수 있다. UE 5.8 기준 `UPrimitiveComponent::OverlapComponent`는 `GetBodyInstance()->OverlapTest`로, `USkeletalMeshComponent::OverlapComponent`는 `Bodies` 순회로 구현되므로, 영역 메시가 `NoCollision`이거나(스켈레탈이면) 피직스 애셋이 없으면 **항상 false** → 스캐너 후보에서 빠지고(`Plugins/WxWorld/.../WxInteractionScannerComponent.cpp:165`) 서버 사거리 검증도 실패해(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:87`) 상호작용이 통째로 무동작한다. 경고·`ensure` 한 줄 없어 원인 추적이 어렵다. 하필 이번 리팩터링의 헤드라인이 "누가 대상인가는 콜리전과 무관하다"(`WxInteractable.h:15`, 스캐너 주석 147행)여서, 기획자·프로그래머가 영역 메시의 프리셋을 `NoCollision`으로 바꾸는 것은 계약상 안전해 보인다 — 실제로는 그 순간 프롬프트가 사라진다. `AWxNpc`는 이 전제 때문에 몸통 충돌이 캡슐로 옮겨간 뒤에도 메시의 `QueryOnly`를 남겨 둔 상태다(`Plugins/WxDialogue/Source/WxDialogue/Private/WxNpc.cpp:34`). 덧붙여 스캐너 주석 164행은 "콜리전 형상 기준, 없으면 바운즈"라고 폴백이 있는 것처럼 적고 있으나 실제 구현에 폴백은 없다.
- **제안**: 폴백 도입은 워크로그(`2026-07-25-상호작용-채널-제거.md`)에서 "판정 기준이 대상마다 갈린다"는 이유로 의도적으로 배제했으므로 되돌릴 필요는 없다. 대신 전제를 눈에 보이게 만든다 — 개발 빌드에서 `Mesh->IsQueryCollisionEnabled()`(스켈레탈이면 바디 유무)를 확인해 `ensureMsgf`/경고 로그를 남기고, `WxInteractable.h:38`의 전제를 스캐너 164행 주석과 일치시킨다.
- **확신도**: 높음 (조용한 실패 경로 자체는 엔진 구현으로 확인. 폴백 대신 진단으로 가자는 처방은 기존 설계 결정을 존중한 선택).

### 2. 🟡 "단일 조회 관문" 주장이 이미 깨져 있다 — `Find`를 우회하는 캐스트가 남아 있음
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:32`
- **범주**: 설계/구조
- **문제**: `Find`의 주석은 "소비처(스캐너·상호작용 어빌리티)가 전부 이 조회를 거치므로, 장차 계약이 액터에서 컴포넌트로 내려가도 바뀌는 곳은 여기 하나다"라고 보장하지만, `Source/WxGame/Controller/WxPlayerController.cpp:211`이 `Cast<IWxInteractable>(Selected->GetOwner())`로 같은 조회를 직접 재구현한다 — `Find`의 본문(`WxInteractable.cpp:11`)과 완전히 동일한 식이다. 계약이 소유 컴포넌트 기준으로 내려가는 순간 이 한 줄만 조용히 옛 규칙으로 남아, 프롬프트 표시(ViewModel push)만 다른 대상을 가리키는 형태로 어긋난다. 주석이 사실이 아닌 보장을 하고 있어 다음 작업자가 `Find`만 고치고 끝낼 위험이 크다. (`WxInteractionScannerComponent.cpp:152`의 캐스트는 액터 순회 필터라 성격이 달라 `Find`로 대체할 수 없다 — 이 역시 "액터가 구현한다"는 가정이 박혀 있는 두 번째 지점이다.)
- **제안**: `WxPlayerController.cpp:211`을 `IWxInteractable::Find(Selected)`로 교체한다(한 줄, 동작 동일). 액터 순회 필터는 대체 불가이므로, `Find` 주석에서 "전부"라는 보장을 빼고 "메시→구현체 조회는 여기 하나, 액터 브로드페이즈는 스캐너에 별도로 존재"로 현실을 적는다.
- **확신도**: 높음.

### 3. 🟢 코드에서 사라진 `WxInteractable` 콜리전 채널이 ini에 남아 있다
- **위치**: `Config/DefaultEngine.ini:38` (기준 계약: `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h:9`)
- **범주**: 중복/복잡도 (데드 설정)
- **문제**: `WxCollisionChannels.h`는 스스로를 ini 채널 등록과 동기화되는 단일 출처로 선언하는데, 지금 코드에 남은 채널 상수는 `ECC_WxAttack`(`ECC_GameTraceChannel1`) 하나뿐인 반면 ini에는 `ECC_GameTraceChannel2 = "WxInteractable"`이 그대로 등록돼 있다. 워크로그 `2026-07-25-상호작용-채널-제거.md`는 이 줄을 삭제했다고 기록(계획 15행·완료 50행)하지만 실제로는 남아 있어, 상수·주석만 지워지고 ini 정리가 누락된 미완 변경이다. 현재 이 채널을 참조하는 코드·프로파일은 없어 런타임 영향은 없으나, 다음에 새 Wx 채널을 추가하며 `ECC_GameTraceChannel2`를 집는 순간 에디터에는 "WxInteractable"이라는 죽은 이름과 `DefaultResponse=Ignore`가 붙은 채널이 재사용되어 원인을 짚기 어려운 응답 버그가 된다.
- **제안**: `Config/DefaultEngine.ini:38`의 `WxInteractable` 등록 줄을 삭제한다. 겸사겸사 `Plugins/WxCore/README.md:8`, `:25`가 아직 `ECC_WxInteractable`을 담당 항목으로 적고 있으니 함께 정리한다.
- **확신도**: 높음 (ini·헤더·워크로그 3자 대조).

### 4. 🟢 `GetActiveInteractionMeshes`의 출력 배열 규약(append/reset)이 명시돼 있지 않다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:50`
- **범주**: 설계/구조
- **문제**: out 파라미터를 받는 순수 가상 함수인데 "구현체가 배열을 비우고 채우는가, 뒤에 덧붙이는가"를 계약이 정하지 않는다. 실제 구현 4개는 전부 덧붙이는 쪽이고(`WxGimmick.cpp:75`는 `Reserve(OutMeshes.Num() + N)`으로 append를 명시적으로 전제, `WxNpc.cpp:43`·`WxItemPickup.cpp:71`·`WxEnemyCharacter.cpp:79`는 `Add`만), 호출부 2개도 우연히 맞게 쓰고 있다(스캐너는 `ActiveMeshes.Reset()` 후 호출 — `WxInteractionScannerComponent.cpp:159`, 어빌리티는 매번 지역 배열 새로 선언 — `WxAbility_Interact.cpp:78`). 즉 지금은 무해하지만, 새 호출부가 배열을 재사용하며 `Reset()`을 빼면 이전 액터의 메시가 누적돼 "이미 사라진 영역이 후보로 남는" 형태로 조용히 틀어진다. 계약 정의가 존재 이유인 모듈이므로 이 한 줄은 계약이 답해야 한다.
- **제안**: 주석에 "구현체는 `OutMeshes`를 비우지 않고 덧붙인다(호출부가 재사용 시 `Reset`)"를 한 줄 추가한다. 동작 변경 없음.
- **확신도**: 높음 (구현·호출부 전수 확인).

### 5. 🟢 `Source` 파라미터 타입이 실제보다 넓고, `Find`가 const를 세탁한다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:34`, `:56`, `:66`, `:72`
- **범주**: 성능/안전 (타입 계약 이완)
- **문제**: 같은 계약 안에서 영역을 내보낼 땐 `UPrimitiveComponent*`(50행), 되받을 땐 `const UActorComponent*`(56·66·72행)로 타입이 갈린다. `Source`로 올 수 있는 값은 정의상 `GetActiveInteractionMeshes`가 내보낸 원소뿐인데도 넓은 타입을 받으므로, 메시가 필요한 구현체는 매번 다운캐스트해야 한다 — `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmick.cpp:88`이 `const_cast<UPrimitiveComponent*>(Cast<UPrimitiveComponent>(Source))`로 캐스트와 const 제거를 한 줄에서 하고 있다. 아울러 `Find`는 `const UActorComponent*`를 받아 **비const** 구현체 포인터를 돌려주므로, const 문맥에서도 `OnInteracted`(상태 변경) 호출이 컴파일된다(`WxInteractionScannerComponent.cpp:78`의 `GetPrompts() const`가 실제로 const 경로에서 `Find`를 부른다 — 지금은 프롬프트만 읽어 문제없음).
- **제안**: `Source` 계열 파라미터를 `const UPrimitiveComponent*`로 좁히면 구현체의 캐스트가 사라진다. 다만 넓은 타입은 "장차 계약이 액터에서 컴포넌트로 내려간다"는 로드맵(32·45행 주석)을 위한 의도적 여지일 수 있으므로, 유지한다면 그 이유를 주석에 남겨 다음 사람이 좁히려다 되돌리지 않게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음).

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` + `Private/WxGameplayTags.cpp`(전수 대조)
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`
- **교차 확인(리뷰 대상 아님, 계약 준수 확인용)**: `Plugins/WxWorld/.../Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/.../Gimmick/WxGimmick.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Plugins/WxDialogue/.../WxNpc.cpp`, `Plugins/WxInventory/.../WxItemPickup.cpp`, `Config/DefaultEngine.ini`, UE 5.8 엔진 소스(`PrimitiveComponent.cpp:3937`, `SkeletalMeshComponentPhysics.cpp:3080`)
- **점검했으나 문제 없음**:
  - 모듈 경계 — `WxCore.Build.cs` 의존은 `Core`/`CoreUObject`/`Engine`/`GameplayTags` 엔진 모듈뿐, `WxCore.uplugin`의 Plugins 의존 0. 다른 Wx 플러그인 미참조 규칙 준수.
  - CLAUDE.md 코딩 규칙 — 소스 8개 전부 첫 줄 `// Copyright Woogle. All Rights Reserved.` 존재, `Wx` prefix 일관, `BlueprintCallable` 0건, 람다 0건, override는 `IModuleInterface` 빈 구현뿐이라 `Super::` 대상 없음, 델리게이트 콜백 자체가 없어 `Handle` prefix 대상 없음.
  - Gameplay Tag — 선언 95개 / 정의 95개가 1:1 완전 대응(누락·고아 0), 변수명↔문자열 치환 규칙(`_`↔`.`) 불일치 0, `WXCORE_API` 부착 누락 0. 프로젝트 내 `UE_DEFINE_GAMEPLAY_TAG` 사용처도 이 모듈 밖에는 서드파티 `PersistenceExamples`뿐이라 태그 선언 독점이 지켜지고 있다.
  - `IWxSavable` — 이전 리뷰(2026-07-21)에서 지적한 존재하지 않는 심볼 `GetWxSaveId()` 주석 오표기가 수정되어 현재 `GetSaveId()`로 일치한다.
- **미검토 / 한계**: `IsMeshInRange`의 실기 판정 감각(스켈레탈 피직스 애셋 유무별 실제 반응)은 코드 정적 분석으로만 확인했고 PIE 실측은 하지 않았다 — 워크로그도 실기 미검증을 후속 과제로 남겨 두었다.

---
*문서 기준 커밋 `c42b5fec` · 리뷰일 2026-07-25 · 소스 8파일 — `/module-review`로 갱신*

# WxCore — 코드 리뷰

> 선언·상수·계약만 담는 foundation 모듈이라 여전히 매우 깨끗하다 — 🔴 결함은 없고, 로직을 가진 유일한 파일(`WxInteractable.cpp`, 실행 코드 12줄)의 암묵 전제와 계약 주변 설정 정리가 남은 전부다. 소스 9개를 전부 통독했고, `IWxInteractable`·`IWxSavable` 계약은 소비처(WxWorld 스캐너/기믹 ST 컴포넌트, WxGame 상호작용 어빌리티, WxDialogue/WxInventory 구현체)와 UE 5.8 엔진 구현(`PrimitiveComponent.cpp:3937`)까지 교차 확인했다. Gameplay Tag 87개는 선언·정의 1:1 대응과 변수명↔문자열 치환 규칙을 스크립트로 전수 대조해 불일치 0을 확인했고, 프로젝트 어디에도 WxCore 밖 태그 선언은 없다(선언 독점 유지).

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 `IsMeshInRange`가 쿼리 콜리전·피직스 바디 부재 시 조용히 false를 돌려준다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:31`
- **범주**: 버그/정확성
- **문제**: 사거리 판정이 `Mesh->OverlapComponent(...)` 한 줄에 달려 있는데, UE 5.8 구현은 `GetBodyInstance()->OverlapTest` → 실패 시 `GetAllPhysicsObjects()` 순회(`PrimitiveComponent.cpp:3937-3946`)라 물리 바디가 없으면 무조건 false다. 즉 영역 메시가 `NoCollision`이거나 스켈레탈인데 피직스 애셋이 없으면 서버 사거리 검증이 항상 실패해(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:85`) 상호작용이 통째로 무동작하고, 같은 이유로 클라 스캐너의 월드 오버랩(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:161`)에도 아예 잡히지 않아 프롬프트조차 뜨지 않는다. 경고·`ensure` 한 줄이 없어 "왜 이 액터만 상호작용이 안 되지"가 원인 추적 불가에 가깝다. 하필 계약의 헤드라인이 "대상 자격은 콜리전 프리셋·응답과 무관하다"(`WxInteractable.h:15`, `:41`)여서 기획자·프로그래머가 영역 메시의 프리셋을 내리는 것이 계약상 안전해 보이는데, 실제로는 그 순간 상호작용이 사라진다. 2026-07-25 리뷰에서 동일 지적, 미수정.
- **제안**: 폴백(바운즈 등) 도입은 "판정 기준이 대상마다 갈린다"는 이유로 이미 의도적으로 배제됐으므로 되돌리지 말고 전제를 드러낸다 — `IsMeshInRange` 진입부에서 개발 빌드 한정으로 `Mesh->IsQueryCollisionEnabled()`를 확인해 `ensureMsgf`/경고 로그를 남긴다(런타임 동작 변화 없음).
- **확신도**: 높음 (엔진 구현·양쪽 소비처 코드로 확인).

### 2. 🟢 클라 스캔 반경과 서버 검증 반경이 서로 다른 모듈의 독립 필드라 드리프트하면 조용히 무동작한다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:40` (계약 문구), 실제 값은 `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h:82`·`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.h:41`
- **범주**: 설계/구조
- **문제**: 계약 주석은 "클라 스캐너는 같은 원점·반경의 구 오버랩으로 주변을 모으므로 결과가 이 판정과 일치한다"고 단언하지만, 그 반경은 서로 다른 두 모듈의 독립 `EditDefaultsOnly float`(둘 다 기본값 150.f)이고 이를 맞춰 주는 코드·어서션이 없다. 한쪽 BP 기본값만 올리면 클라에는 프롬프트가 뜨는데 서버가 사거리에서 튕겨 "눌러도 아무 일 없음"이 되고, 이 경로 역시 로그가 없다(위 1번과 같은 무증상 실패). 값을 각자 보유하는 것 자체는 변조 방지 설계라 유지가 맞지만, **기본값**까지 각자 적을 이유는 없다.
- **제안**: 공유 기본값 상수(예: `WxInteractable.h`에 `WxInteraction::DefaultScanRadius`)를 WxCore에 두고 양쪽 필드 초기화에 쓴다. 계약 주석에는 "반경 일치는 코드가 강제하지 않는다"를 명시한다.
- **확신도**: 중간 (분리 자체는 의도된 설계, 기본값 중복만이 지적 대상).

### 3. 🟢 코드에서 사라진 `WxInteractable` 콜리전 채널이 아직 ini에 남아 있다
- **위치**: `Config/DefaultEngine.ini:38` (기준 계약: `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h:9`)
- **범주**: 중복/복잡도 (데드 설정)
- **문제**: `WxCollisionChannels.h`는 스스로를 ini 채널 등록과 순서가 일치해야 하는 단일 출처로 선언하는데, 코드에 남은 상수는 `ECC_WxAttack`(`ECC_GameTraceChannel1`) 하나뿐인 반면 ini에는 `ECC_GameTraceChannel2 = "WxInteractable"`이 그대로 등록돼 있다. 이 채널을 참조하는 C++·프로파일은 전무하고(전수 grep 0건), 남은 참조는 `Plugins/WxWorld/.../Gimmick/WxGimmickStateTreeNodes.h`의 옛 주석 한 줄뿐이라 런타임 영향은 없다. 다만 다음에 새 Wx 채널을 추가하며 `ECC_GameTraceChannel2`를 집는 순간, 에디터에서는 "WxInteractable"이라는 죽은 이름과 `DefaultResponse=Ignore`가 붙은 채널이 재사용되어 원인을 짚기 어려운 응답 버그가 된다. 2026-07-25 리뷰에서 동일 지적, 미수정.
- **제안**: `Config/DefaultEngine.ini:38` 등록 줄을 삭제하고, 그 채널을 언급하는 WxWorld ST 노드의 옛 주석도 함께 정리한다.
- **확신도**: 높음 (ini·헤더·전수 grep 대조).

### 4. 🟢 계약 안에서 같은 "영역 메시"가 두 타입으로 오간다 — 구현체마다 캐스트 발생
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:52` vs `:58`, `:68`, `:74`
- **범주**: 설계/구조
- **문제**: 영역 메시를 물을 땐 `const UPrimitiveComponent*`(52행), 되받을 땐 `const UActorComponent*`(58·68·74행)로 타입이 갈린다. 실제 호출부는 예외 없이 `UPrimitiveComponent*`를 넘기므로(`WxInteractionScannerComponent.cpp:87`·`:189`, `WxAbility_Interact.cpp:92`·`:97`) 넓은 타입에서 얻는 이득은 없고, 메시가 필요한 구현체만 매번 다운캐스트한다 — `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:115`·`:122`가 `const_cast<UPrimitiveComponent*>(Cast<UPrimitiveComponent>(Source))`를 두 번 반복한다. 아울러 `Find`(`:35`)는 `const` 컴포넌트를 받아 비const 구현체 포인터를 돌려주므로 const 문맥에서도 `OnInteracted`(상태 변경) 호출이 컴파일된다(`WxInteractionScannerComponent.cpp:76`의 `GetPrompts() const`가 실제로 const 경로에서 `Find`를 부른다 — 지금은 읽기만 해서 무해).
- **제안**: `Source` 계열 파라미터를 `const UPrimitiveComponent*`로 좁힌다(호출부 변경 없음, 구현체의 `Cast` 제거). 넓은 타입이 "영역이 장차 비-프리미티브 컴포넌트가 될 수 있다"는 의도적 여지라면 그 이유를 주석에 남겨 다음 사람이 좁히려다 되돌리지 않게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음).

### 5. 🟢 README가 기믹 태그 추가 위치를 헤더 주석과 반대로 안내한다
- **위치**: `Plugins/WxCore/README.md:39` (헤더의 정본: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:108`)
- **범주**: 설계/구조 (계약 문서 불일치)
- **문제**: README는 "신규 기믹의 상태 태그는 ini 에 추가한다"고 적었지만, 헤더 주석은 "코드가 안 읽어도 태그는 여기서만 만든다 — 신규 기믹의 상태 이름도 이 파일에 추가한다"이고 README 자신도 46행에서 "다른 모듈에서 임의 선언 금지"를 말한다. 게다가 프로젝트에는 `DefaultGameplayTags.ini`가 아예 없고(전수 검색 0건) 태그 87개 전부가 네이티브 선언이다. README를 진입점으로 삼는 다음 세션이 존재하지 않는 ini에 태그를 추가하려다 헤매거나, 에디터로 태그를 만들어 선언 독점을 깨뜨릴 수 있다.
- **제안**: README 39행을 "신규 기믹의 상태 태그도 `WxGameplayTags.h`/`.cpp`에 쌍으로 추가한다"로 바로잡는다.
- **확신도**: 높음 (헤더·README 내부 모순 + ini 부재 확인).

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` + `Private/WxGameplayTags.cpp`(스크립트 전수 대조), `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxActorTarget.h`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`
- **교차 확인(리뷰 대상 아님, 계약 준수 확인용)**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Plugins/WxDialogue/.../WxNpc.cpp`, `Plugins/WxInventory/.../WxItemPickup.cpp`, `Config/DefaultEngine.ini`, UE 5.8 엔진 소스(`PrimitiveComponent.cpp:3937`). 이 과정에서 모듈 경계(`WxCore.Build.cs`는 엔진 모듈 5개뿐, `.uplugin` Plugins 의존 0), CLAUDE.md 코딩 규칙(첫 줄 Copyright 9/9, `Wx` prefix 일관, `BlueprintCallable`·람다·델리게이트 콜백 0건), 태그 선언 독점, `FWxActorTarget`의 모듈 간 링크(UHT가 `WXCORE_API`로 생성자 함수 export)는 모두 문제없음을 확인했다.
- **미검토 / 한계**: `IsMeshInRange`의 실기 반응(스켈레탈 피직스 애셋 유무별)은 정적 분석·엔진 소스로만 확인했고 PIE 실측은 하지 않았다. 기믹 상태 태그가 실제로 어떤 ST 에셋의 어느 상태에 붙었는지는 BP/에셋 영역이라 확인하지 않았다(BP 스냅샷 디렉터리가 현재 비어 있어 대조 불가).

---
*문서 기준 커밋 `c37b6fa6` · 리뷰일 2026-07-31 · 소스 9파일 — `/module-review`로 갱신*

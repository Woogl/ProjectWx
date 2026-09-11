# WxUI — ViewModel 독립성·재사용성 코드 리뷰

> **2026-09-12 최종 적용 범위**: 아래는 수정 전 리뷰 기록이다. 사용자가 범위 축소를 승인하여 AbilitySystem의 팩토리 내부 초기화 계약과 Ability·Attribute·Effect의 표시 초기화/FieldNotify 수정만 유지했다. Attribute의 Max 생략은 Current로 통일했다. Ability 갱신과 Effect 제거는 기존처럼 부모 AbilitySystem이 관리한다. 독립 관찰과 Resolver 타입 확장은 되돌렸다.
> **사용 조건**: 공유 AbilitySystem은 `GetOrCreate(ASC)`로 얻으며, `Deinitialize`는 공유본 전체의 사용 종료에만 호출한다. 독립 VM 사용과 순정 ASC의 단순 제거·클라이언트 복제 갱신 제약은 이번 범위에서 해결하지 않는다.

> 표시 데이터와 도메인 구현 사이의 경계는 좋다. 다만 GAS ViewModel은 현재의 공유 팩토리와 생성·폐기 흐름에 기대므로, 직접 생성하거나 동일 인스턴스의 소스를 교체하는 용도까지 안전하다고 보기는 어렵다.
> WxUI의 ViewModel 10개(베이스 포함), Resolver 2개, 변환 라이브러리 및 주요 생성·소비 경로를 검토했다. 전체 모듈 리뷰를 대체하지 않는다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 0 |

같은 프로젝트의 여러 화면에서 쓰는 재사용성은 대체로 좋다. `WxUI.Build.cs`는 다른 Wx 도메인 대신 `WxCore`와 엔진 모듈에 의존하며, Character는 이름·초상화·ASC를 주입받고 Item은 원본 UObject의 구체 타입을 해석하지 않는다. 다른 프로젝트로 이식하려면 WxCore, GAS, MVVM 및 WxUI 모듈 의존성을 함께 고려해야 한다. 특히 Ability는 단일 비용 자원, `Max` 접두 최대치, 쿨다운 GE 스택을 충전 소비 수로 해석하는 게임 규약이 들어 있어 순수한 범용 GAS 표시기는 아니다(`Private/MVVM/WxViewModel_Ability.cpp:52`, `:511`, `:574`; 이하 경로는 검토 범위에 전체 기재).

## 결과

### 1. 🟡 AbilitySystem의 공개 Initialize는 소스 교체를 안전하게 처리하지 않는다

- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:29`, `:47`, `:92`, `:116`
- **범주**: 버그/정확성
- **문제**: `Initialize(ASC_A)` 후 `Initialize(ASC_B)`를 호출하면 기존 구독과 자식 캐시를 남긴 채 `CachedASC`만 바꾸고 B에도 구독한다. 기존 키로 조회한 Attribute/Ability 자식은 A를 계속 본다. 이후 Deinitialize는 B의 구독만 제거하므로 A의 이벤트가 재사용된 VM에 들어올 수 있다. 동일 ASC로 두 번 초기화해도 구독과 활성 Effect 항목이 중복된다. 현재 GetOrCreate 경로는 한 번 초기화하므로 이 문제를 피하지만 공개 API 자체는 재바인딩 가능한 형태다.
- **제안**: ASC별 일회 생성이 의도라면 Initialize를 팩토리 내부 API로 제한하고 같은 인스턴스의 재바인딩을 금지한다. 소스 교체를 지원하려면 기존 구독, 자식 캐시, 목록 알림을 함께 정리하고 공유 키인 Outer와 실제 소스가 어긋나지 않게 설계한다.
- **확신도**: 높음

### 2. 🟡 Ability는 직접 생성하면 어빌리티 목록 변경을 스스로 추적하지 못한다

- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:10`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:41`, `:246`
- **범주**: 설계/구조
- **문제**: Ability의 Initialize는 GE 추가와 태그 변경만 구독한다. Spec 변경으로 자식의 RefreshBoundAbility를 호출하는 책임은 부모 AbilitySystem에 있다. 따라서 `NewObject<UWxViewModel_Ability>() → Initialize(ASC, Tags)`만 수행한 슬롯은 태그나 GE 변경이 없는 어빌리티 부여·교체 후 표시가 갱신되지 않는다. 현재 게임 Resolver는 부모의 GetOrCreateAbilityViewModel을 이용하므로 정상 경로에서는 보완된다. 부모를 Deinitialize한 뒤 외부 위젯이 자식만 유지한 경우에도 이 재매칭 지원은 사라진다.
- **제안**: 현재 구조를 유지한다면 Ability를 부모 팩토리로 얻어야 하는 조건을 공개 계약에 명시한다. 단독 사용이 실제 요구라면 Spec 변경 관찰도 Ability가 담당하게 하거나 호출자가 갱신 이벤트를 전달하는 명시적 경로를 제공한다.
- **확신도**: 높음

### 3. 🟡 Ability를 빈 슬롯으로 재초기화하면 기존 화면에 초기화 통지가 전달되지 않는다

- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:104`, `:179`
- **범주**: 버그/정확성
- **문제**: 재초기화가 호출하는 Deinitialize는 Title, Icon, 비용, 충전, 활성화 여부를 직접 기본값으로 대입한다. 새 소스에 일치 어빌리티가 없으면 MatchedAbility와 CachedAbility가 모두 null이므로 RefreshBoundAbility는 즉시 반환한다. 객체 값은 비었지만 이미 연결된 FieldNotify 바인딩에는 변경이 오지 않아 이전 스킬 표시가 남을 수 있다. 새 어빌리티를 찾더라도 새 값이 리셋 기본값과 같으면 해당 필드의 setter가 변경을 감지하지 못한다.
- **제안**: 살아 있는 VM의 Reset과 파괴 중 구독 정리를 구분한다. 재바인딩 시 표시를 비우는 알림을 발행하고, BeginDestroy 경로에서만 알림을 억제한다. populated → empty 및 populated → 기본값이 많은 다른 스킬 전환을 검증한다.
- **확신도**: 높음

### 4. 🟡 Attribute 직접 재초기화에서 생략한 최대값이 이전 소스 값으로 남는다

- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp:18`, `:32`, `:43`, `:140`
- **범주**: 버그/정확성
- **문제**: Deinitialize는 구독과 바운드 Attribute만 비우고 표시 수치는 유지한다. 예를 들어 처음 최대값 100을 바인딩한 뒤 `Initialize(NewASC, Current, FGameplayAttribute())`를 호출하면 새 Max 구독은 없지만 `MaxAttributeAmount`는 100이고 비율 계산도 이를 사용한다. 부모 팩토리는 Max 생략을 Current로 정규화하므로 이 문제를 우회하지만 직접 Initialize와 팩토리의 의미가 다르다. Deinitialize만 호출해도 표시에는 기존 수치가 남는다.
- **제안**: Max 생략의 의미를 Initialize와 팩토리에서 일치시키고, 재사용 Reset에서 값·비율·상태 플래그를 함께 초기화한다. 최대값이 있는 소스 → 없는 소스 전환을 검증한다.
- **확신도**: 높음

### 5. 🟡 Effect의 시간 필드는 재사용 시 남으며 무한 효과의 제거는 부모에 의존한다

- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:43`, `:70`, `:185`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:197`
- **범주**: 버그/정확성
- **문제**: Effect의 Deinitialize는 시간·스택·표시 필드를 초기화하지 않는다. 유한 효과 VM을 무한 효과로 재초기화하면 비율만 1로 바뀌고 Duration/TimeRemaining은 이전 값이 남는다. 또한 단독 생성한 무한 Effect에는 ticker도 효과 제거 구독도 없어 제거 후 값이 자동으로 비워지지 않는다. 부모가 배열에서 제거하는 현재 목록 사용은 이 수명 처리를 대신하지만, 독립 상세창이나 동일 VM 풀링에서는 호출자가 직접 처리해야 한다.
- **제안**: 효과 재바인딩 전에 표시 필드를 통지하며 초기화하고 무한 지속의 Duration/Remaining 표현을 명확히 정한다. 단독 사용을 지원한다면 제거 이벤트 처리도 제공하고, 목록 전용이라면 소유자의 제거 책임을 계약에 명시한다.
- **확신도**: 높음

## 클래스별 독립 사용 조건

| 클래스 | 단독 생성·사용 조건 | 동일 인스턴스 재사용 / 화면 재활용성 |
| --- | --- | --- |
| `UWxViewModel` | 추상 베이스. 파생에서 이미지 결과를 FieldName별 필드에 반영하고 참조를 보유한다. | 이미지 슬롯별 취소와 파괴 시 정리 공통화는 좋다. 베이스 Deinitialize는 표시 Reset 계약이 아니다. |
| `UWxViewModel_Item` | UObject, 표시 이름, 이미지 주입만 필요하다. 인벤토리 없이 사용 가능하다. | 높음. 재초기화와 살아 있는 VM의 빈 값 통지가 구현되어 있다. |
| `UWxViewModel_Interaction` | Prompt와 Selected를 외부에서 설정하면 된다. | 높음. 특정 InteractionList 없이 사용 가능하다. 재사용 시 두 필드를 호출자가 다시 설정한다. |
| `UWxViewModel_Indicator` | 거리(m)와 화면 가장자리 여부를 외부에서 공급한다. | 높음. 투영·액터 탐색은 외부 책임이다. 재사용 시 두 값을 다시 공급한다. |
| `UWxViewModel_Subtitle` | 직접 생성 후 Show/Hide로 사용 가능하다. 기본 Resolver와 ST 태스크를 쓰면 GameInstance의 고정 `VM_Subtitle` 공유본을 쓴다. | 단일 자막 채널에는 좋다. 여러 채널이나 로컬 플레이어별 자막에는 기본 조회 경로를 그대로 쓸 수 없다. 최신 핸들만 Hide할 수 있어 지연 종료의 간섭은 방어한다. |
| `UWxViewModel_Character` | ASC, 이름, 초상화를 주입한다. 구체 캐릭터 타입은 불필요하나 현재 Initialize는 ASC를 필수로 요구한다. | 좋음. 이름표는 개별 생성, 플레이어는 공유 생성한다. Deinitialize는 표시를 비우되 공유 AbilitySystem을 중단하지 않는다. ASC 없는 인물 도감에는 그대로 쓰기 어렵다. |
| `UWxViewModel_Attribute` | ASC와 Attribute 쌍을 주입한다. 부모 없이 값 변경 구독 가능하다. | 같은 유효 Attribute 쌍을 보는 화면에는 좋다. 직접 재바인딩·Max 생략은 발견 4 보완 필요다. |
| `UWxViewModel_Ability` | ASC와 슬롯 태그, 게임 비용·충전 규약이 필요하다. 부여 변화는 부모 또는 호출자가 전달해야 한다. | 공유 HUD 슬롯에는 적합하다. 단독 사용과 인스턴스 재활용은 발견 2·3 보완 필요다. |
| `UWxViewModel_Effect` | ASC, 유효 ActiveEffect 핸들, IWxUIData가 필요하다. | 현재 효과 목록에는 적합하다. 독립 제거 관찰과 풀링은 발견 5 보완 필요다. |
| `UWxViewModel_AbilitySystem` | ASC를 GetOrCreate에 전달하고 소비자가 강한 참조를 유지한다. | 동일 ASC의 여러 화면 공유에는 좋다. 다른 ASC로 동일 객체 재사용은 발견 1 때문에 부적합하다. |

`GetOrCreate` 공유본은 한 위젯의 전용 객체가 아니다. 소비 위젯이 닫힐 때 임의로 Deinitialize하면 다른 화면의 관찰에도 영향을 준다. 반대로 외부의 강한 참조 없이 Outer만 지정했다고 지속 보유되는 계약도 아니다. 공유 키는 소스 Outer와 정확한 클래스이며, 팩토리는 구체 기본 클래스를 생성하므로 서브클래스 교체를 지원하는 확장 지점은 현재 없다.

Attribute Resolver는 OwningPlayer의 현재 Pawn에서 ASC를 찾는 어댑터다. 타깃·보스·미리보기의 임의 ASC에는 직접 주입 경로를 사용해야 한다. 기본 HUD는 `WxPlayerLayoutComponent.cpp:43`에서 Pawn 교체 시 제거·재생성하므로 Resolver 자체가 지속 재바인딩하지 않는다는 사실만으로 현재 HUD의 결함이라고 판정하지 않았다.

## 검토 범위

- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` 및 동일 디렉터리의 `WxViewModel_Ability.h`, `WxViewModel_AbilitySystem.h`, `WxViewModel_Attribute.h`, `WxViewModel_Character.h`, `WxViewModel_Effect.h`, `WxViewModel_Indicator.h`, `WxViewModel_Interaction.h`, `WxViewModel_Item.h`, `WxViewModel_Subtitle.h`; 각 헤더에 대응하는 `Plugins/WxUI/Source/WxUI/Private/MVVM/`의 cpp 10개.
- **깊게 본 소비 경로**: `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`.
- **훑은 파일**: `Plugins/WxUI/README.md`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxMVVMConversionLibrary.h`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`; WxGame의 PlayerCharacter/BossDisplay/InteractionList/InventoryItem 생성 지점은 검색으로 확인했다.
- **미검토 / 한계**: BP/WBP 바인딩과 실제 화면, 네트워크 복제 타이밍은 검증하지 않았다. 빌드·런타임 실험 없이 현재 C++의 분기와 구독 경로로 판단한 리뷰다. 소스 56파일은 WxUI 전체의 h/cpp 수이며 모두를 통독했다는 뜻은 아니다. 기존 미커밋 `module_review_WxUI.md`는 보존하고 이 문서만 신규 작성했다. 소스는 수정하지 않았다.

---
*문서 기준 커밋 `1fab89cf4` · 리뷰일 2026-09-12 · 소스 56파일 — `/module-review`로 갱신*

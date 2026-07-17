# 모듈 경계 감사 (Module Boundary Audit)

> 작성일: 2026-06-13
> 대상: `Wx.uproject` 전체 (플러그인 7종 + `WxGame` 게임 모듈 + `WxEditor`)
> 성격: 읽기 전용 분석. 코드 변경 없음.

## 0. TL;DR

- **선언 레벨 경계는 완벽하다.** 모든 도메인 플러그인의 `.Build.cs`·`.uplugin`이 Wx 플러그인 중 **`WxCore`만** 참조한다. 플러그인↔플러그인 직접 의존 엣지는 **0건**.
- **경성 위반(컴파일/콘텐츠로 강제 결합되는 진짜 위반)도 0건.** 소프트 참조·문자열 경로 로드·교차 `#include`·리플렉션 우회·CDO 침범 모두 발견되지 않았다. 콘텐츠(BP) 레벨에서도 도메인↔도메인 하드 레퍼런스는 없다.
- 실질적인 약화는 **"규칙은 어기지 않지만 규칙의 의도를 우회하는"** 합법적 회색지대 3종에 집중되어 있다:
  1. **WxGame이 도메인 로직을 떠안음** — 장비 컴포넌트의 인벤토리 규칙 + GAS 효과 수명 관리가 `WxGame`에 있음.
  2. **WxGame 글루가 누락된 계약을 대신함** — UI 리졸버가 "WxUI는 WxCombat에 의존할 수 없어서"를 코드 주석으로 자인하며 양쪽을 잇고 있음.
  3. **WxCore에 단일 도메인 전용 태그가 적재됨** — `WxCombat`만 소비하는 내부 분류 태그가 공용 계약 사이에 섞여 있음.
- 추가로, 파운데이션 성격의 타입(`FWxCharacterUIData`, `EWxTeam`)이 소비 도메인 플러그인 안에 들어 있어 **잠재적 결합(latent coupling)** 을 만든다. 현재는 `WxGame`만 소비하므로 합법이지만, 타입 위치가 틀렸다.

종합 등급: **양호(Healthy).** DAG는 살아 있고, 위에 열거한 항목은 "위반"이라기보다 "엔트로피"다. 지금이 가장 싸게 정리할 시점이다.

---

## 1. 경계 규칙 (검증 기준)

`.claude/CLAUDE.md` 기준:

- 게임 주요 시스템은 플러그인 단위로 분리한다.
- **모든 플러그인은 `WxCore`를 제외한 다른 플러그인을 참조하면 안 된다.**
- `WxCore`는 **공용 계약만** 보유한다(태그·인터페이스·상수·베이스 타입). 도메인 컨텐츠/데이터 타입 신설 금지.
- `WxGame`은 게임 모듈(플러그인 아님) = 컴포지션 루트. 모든 플러그인을 참조해도 된다.

목표 의존 그래프(DAG):

```
        WxGame (게임 모듈, 컴포지션 루트)
          │  모든 플러그인 참조 가능
          ▼
  ┌──────┬──────┬──────┬──────┬──────┬──────┐
WxCombat WxInventory WxUI WxWorld WxAI WxQuest WxSave
  └──────┴──────┴──────┴──────┴──────┴──────┘
          │  전부 WxCore만 참조
          ▼
        WxCore (foundation, 공용 계약)
```

---

## 2. 검증 방법

| 벡터 | 방법 | 결과 |
| --- | --- | --- |
| 선언 의존 | 모든 `.Build.cs` / `.uplugin` 정독 | **Clean** |
| 교차 `#include` | 각 플러그인 public 헤더명 집합 ↔ 타 플러그인 소스 grep | **0건** |
| 소프트/문자열 참조 | `TSoftObjectPtr`/`TSoftClassPtr`/`FSoftObjectPath`/`StaticLoad*`/`FindObject`/`ConstructorHelpers` + `"/WxXxx/"` 경로 리터럴 | **0건** |
| 리플렉션 우회 | 타 플러그인 `UDeveloperSettings`에 대한 `GetDefault<>`, 타입명 기반 UObject 순회 | **0건** |
| 컴포넌트/서브시스템 침범 | `GetComponentByClass`/`GetSubsystem<>`로 타 플러그인 구체 타입 조회 | **0건** (인터페이스 경유는 정상) |
| 콘텐츠(BP) 참조 | `WxBlueprintSnapshot` JSON 87개 grep (parent/component/변수/하드참조) | 도메인↔도메인 **0건** (자세한 사각지대는 §6) |
| WxCore 오염 | WxCore 공용 타입별 실제 소비 플러그인 역추적 | 태그 일부 단일 도메인 (§5) |
| WxGame 누수 | WxGame 멀티 플러그인 클래스 정독 | 2건 (§4) |

---

## 3. 선언 레벨: 깨끗함 (위반 아님, 기준선)

8개 `.Build.cs`에서 Wx 플러그인 간 의존 엣지가 하나도 없다.

| 모듈 | 참조하는 Wx 모듈 | 판정 |
| --- | --- | --- |
| WxCombat / WxInventory / WxUI / WxWorld / WxAI / WxQuest / WxSave | `WxCore`만 | ✅ |
| WxGame (게임 모듈) | 전 플러그인 | ✅ (컴포지션 루트) |
| WxEditor (에디터 모듈) | WxCombat, WxCore, WxInventory, WxUI | ✅ (에디터 모듈) |

`.uplugin`의 `Plugins` 배열도 동일하게 Wx 중 `WxCore`만 선언한다. **이 기준선이 깨지지 않은 것이 이 프로젝트 아키텍처의 가장 큰 자산이다.**

WxCore 비-태그 표면은 모범적이다:

- `IWxInteractionSource` (`WxInteractionSource.h:22`) — 생산자 `WxWorld`, 소비자 `WxInventory`. 두 도메인을 직접 의존 없이 연결하는 정석 계약.
- `IWxSavable` (`WxSavable.h:26`) — 생산자 `WxWorld`, 소비자 `WxSave`. 동일 패턴.
- `UWxAbilityComponent` (`WxAbilityComponent.h:17`) — 어빌리티에 도메인별 데이터를 붙이는 공용 앵커. `WxUI`가 `UWxAbilityComponent_UIData`로 상속.
- `ECC_WxAttack` (`WxCollisionChannels.h`) — 프로젝트 공용 콜리전 채널. 3개 모듈 소비.

---

## 4. 🔴 티어 1 — WxGame이 떠안은 도메인 로직 / 누락 계약 글루

`WxGame`은 모든 플러그인을 참조할 수 있으므로 아래는 **컴파일상 합법**이다. 그러나 "도메인 로직은 도메인 플러그인에, WxGame은 배선만"이라는 의도를 우회한다. 가장 먼저 정리할 가치가 있는 항목들이다.

### 4-1. `UWxEquipmentComponent` — 인벤토리 규칙 + GAS 효과 수명이 WxGame에 있음 ✅ 해소(2026-06-13)

- 파일: `Source/WxGame/Component/WxEquipmentComponent.cpp`
- 증거:
  - `Items/WxItemDefinition.h`, `Items/WxItemFragment.h`(WxInventory) + `Weapon/WxWeaponBase.h`(WxCombat) + `GameplayEffect.h` 동시 include (`:11-14`)
  - `EquipItem`이 `UWxItemFragment_Equippable`을 읽고 스왑 규칙("이전 GE 제거 후 신규 적용")을 보유 (`:47`, `:53-62`)
  - `ApplyEquipEffects`(`:112`)·`RemoveActiveEquipEffects`(`:149`)가 오너 ASC에 GE를 적용/제거하는 **수명 관리 로직**을 직접 수행
- 무엇이 문제인가: 이건 배선이 아니라 **인벤토리 도메인 규칙 + 범용 GAS 플러밍**이다. `WxInventory`는 이미 `GameplayAbilities`에 의존하므로(`WxInventory.Build.cs:24`), GE 적용 로직을 `WxInventory`로 내려도 전혀 막히지 않는다.
- 시각/무기 액터 측(`ApplyEquipmentVisuals` `:70`, `AWxCharacterBase::GetEquippedWeapon`)만 게임 측 무기 액터가 진짜로 필요하다.
- 권고: GE 수명 관리 + 스왑 규칙을 `WxInventory`의 장비 컴포넌트로 이관. `WxGame` 컴포넌트는 `IWxEquipmentInterface`의 비주얼/무기 액터 측만 얇게 구현하도록 분리.
- **✅ 해소(2026-06-13)**: 컴포넌트를 `Plugins/WxInventory/.../Inventory/WxEquipmentComponent`로 이관(상태 + EquipEffect GE 수명). 외형 반영은 캐릭터가 `OnEquipVisualChanged`(USkeletalMesh*, FName) 멀티캐스트 델리게이트를 바인딩해 자기 `WeaponActor`/무기 `SetVisualMesh`로 처리 — 경계를 넘는 데이터는 엔진 타입뿐. 잉여가 된 `IWxEquipmentInterface` 제거, `WxInventory.Build.cs`의 `GameplayAbilities` Public 승격, 모듈 이동 CoreRedirect 추가. 빌드 통과. 상세: `.claude/works/2026-06-13-장비컴포넌트-WxInventory-이관.md`.

### 4-2. `WxViewModelResolver_PlayerCharacter` — "WxUI가 WxCombat에 의존 못 해서" 글루 (자인)

- 파일: `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp`
- 증거: 주석이 직접 사유를 명시 — *"WxUI는 WxCombat(어빌리티)에 의존할 수 없어 ... 양쪽에 의존하는 본 리졸버가 주입한다"* (`:35-37`)
  - `WxAbilityBase`(WxCombat) + `WxAbilityComponent_UIData`·`WxViewModel_Ability`(WxUI)를 동시에 include (`:4`, `:8`, `:11`)
  - ASC의 모든 어빌리티 스펙을 순회하며 어빌리티의 UIData 아이콘을 VM에 주입하는 **지속적 투영 루프** (`:38-60`)
- 무엇이 문제인가: 한 번의 배선이 아니라 **어빌리티→UI 아이콘이라는 안정적 도메인 사실을 컴포지션 루트에서 매번 재유도**하는 글루다. 이는 "누락된 WxCore 계약"의 전형적 신호다.
- 권고: WxCore에 `IWxAbilityIconProvider`(어빌리티가 구현, WxUI가 소비) 같은 계약을 두면 WxUI가 스스로 아이콘을 채울 수 있어 이 루프가 사라진다. (동일 폴더의 Inventory/Item/Boss 리졸버는 단일 도메인이라 깨끗 — 이 리졸버만 예외.)

---

## 5. 🟡 티어 2 — WxCore에 적재된 단일 도메인 태그

> 전제: **태그를 WxCore에 발행해 도메인 간 통신을 디커플링하는 것은 이 프로젝트의 승인된(권장) 패턴이다.** 따라서 단지 존재한다는 이유로 태그를 문제 삼지 않는다. 아래는 **소비자가 `WxCombat` 단 하나뿐이며, 도메인 간 신호가 아니라 `WxCombat` 내부 구현 분류**인 태그 그룹만 추린 것이다.

파일: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`

| 태그 그룹 | 위치 | 유일 소비자 | 성격 |
| --- | --- | --- | --- |
| `SetByCaller.*` (Recovery.UP/MP, ReflectDP, Coeff.ATK, RawDamage, Duration) | `:151-166` | WxCombat | GE SetByCaller 매그니튜드 키 — 순수 내부 수식 배선 |
| `GameplayCue.*` (Damage, PerfectGuard, BuffATK, Exceed, Burn, HitStop) | `:88-106` | WxCombat | 각 태그가 WxCombat `WxCueNotify_*`와 1:1 — 내부 VFX 분류 |
| `ANS.*` (WeaponCollision, ComboWindow) | `:80-83` | WxCombat | AnimNotifyState 마커 — 몽타주/콜리전 내부 흐름 |
| `Ability.Pattern.*` (1~10, Phase) | `:135-146` | WxCombat | 보스 공격 패턴 인덱스 — WxAI조차 소비 안 함 |
| `Damage.*` (Critical, Unblockable, ParryHitReact) | `:111-117` | WxCombat | 데미지 판정 분류 |

- 무엇이 문제인가: 이들은 도메인 간 핸드셰이크가 아니라 `WxCombat`의 내부 메커니즘이다. `WxCombat` 안의 `WxCombatTags.h`로 옮겨도 다른 플러그인은 아무것도 잃지 않는다. WxCore의 "공용 계약만" 규칙을 천천히 마모시키는 부분.
- 실질 피해는 작다(불활성 native 태그 선언일 뿐). 단, 이런 단일 도메인 태그가 누적되면 WxCore가 점점 "전역 태그 덤프"가 된다.
- 권고(낮은 우선순위): WxCombat 전용 분류 태그는 `WxCombat` 내부 태그 파일로 분리. WxCore에는 2개 이상 도메인이 실제로 주고받는 신호만 남긴다.
- **대조군(옮기지 말 것):** `State.*`, `UI.Layer.*`/`UI.Action.*`, `Input.*`, `Event.HitReact.*`, `Ability.*`(Pattern 제외)는 WxUI 네임플레이트·WxAI 퍼셉션·WxGame 어빌리티 등 **2개 이상 도메인이 실제로 소비**하는 진짜 cross-domain 계약이다.

---

## 6. 🟡 티어 3 — 파운데이션 타입의 도메인 플러그인 오배치 (잠재 결합)

아래 두 타입은 **현재 소비자가 소유 도메인 + `WxGame`뿐**이라 `WxGame→플러그인` 엣지(합법) 범위에 머문다. 즉 **지금은 경계 위반이 아니다.** 그러나 의미상 파운데이션(여러 도메인이 공유하는 개념)인데 한 소비 도메인 안에 들어 있어, 향후 다른 도메인이 C++에서 이 타입을 필요로 하는 순간 곧장 위반으로 전환된다.

### 6-1. `FWxCharacterUIData` — WxUI에 있으나 WxCore 성격

- 정의 위치: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxCharacterUIData.h`
- 소비: WxUI(`WxNameplateComponent`, `WxViewModel_Character`) + WxGame(`WxCharacterBase.h:12`, `:59`, `:104`)
- 문제: 게임플레이 액터 `AWxCharacterBase`가 **이름/초상화를 노출하려고 WxUI 헤더를 include**해야 한다(`WxCharacterBase.h:12`). "캐릭터가 UI에 공표하는 표시 데이터"는 cross-domain 데이터 계약이므로 WxCore가 자연스러운 자리다.
- 권고: `FWxCharacterUIData`를 WxCore로 이동(순수 데이터 구조체).

### 6-2. `EWxTeam` / `WxTeamTypes` — WxAI에 있으나 WxCore 성격

- 정의 위치: `Plugins/WxAI/Source/WxAI/Public/WxTeamTypes.h`
- 소비: WxAI + WxGame(`WxCharacterBase.h:13`, `:108`; `WxPlayerCharacter.cpp`; `WxEnemyCharacter.cpp`)
- 문제: 팀 정체성(아군/적군)은 AI 타게팅 + 전투 피아식별 양쪽이 쓰는 파운데이션 개념인데 enum이 AI 도메인에 묶여 있다. 현재 WxCombat은 엔진 `IGenericTeamAgentInterface`를 통해서만 팀을 다뤄 직접 의존을 피하고 있지만, 전투 코드가 `EWxTeam`을 직접 봐야 하는 순간 `WxCombat→WxAI` 위반이 강제된다.
- 권고: `EWxTeam`/`WxTeamTypes`를 WxCore로 이동. 어트리뷰트 계산이 아닌 attitude 로직 자체는 베이스 캐릭터에 남아도 무방하다.

---

## 7. ⚠️ 감사 사각지대 (발견이 아니라 한계)

- `WxBlueprintSnapshot`는 **`Content/Game/` 만 미러링**한다(JSON 87개 전부 `/Game` 하위). 그런데 콘텐츠를 담을 수 있는 도메인 플러그인이 4개 있다: **WxUI, WxWorld, WxInventory, WxQuest** (`CanContainContent: true`).
- 따라서 **이들 플러그인 Content 내부의 BP/WBP/DataAsset이 타 도메인 플러그인 애셋을 하드 참조하는지는 스냅샷만으로 정적 검증되지 않는다.**
- C++ 측이 깨끗하고 에디터가 통상 의존성을 요구하므로 위험은 낮지만, 콘텐츠 레벨 결합은 컴파일러가 잡지 못한다.
- 권고: `WxBlueprintSnapshot`의 IncludeDirectories에 위 4개 플러그인의 `Content/`를 포함시켜, 다음 감사 때 도메인↔도메인 콘텐츠 참조까지 정적으로 확인 가능하게 한다.

---

## 8. 권고 요약 (우선순위)

| 순위 | 항목 | 작업 | 근거 |
| --- | --- | --- | --- |
| 1 | §4-1 장비 GE 로직 | WxGame → WxInventory로 이관 (WxInventory는 이미 GAS 의존) | 도메인 로직 누수, 가장 큰 레버 |
| 2 | §4-2 어빌리티 아이콘 글루 | WxCore에 `IWxAbilityIconProvider` 계약 신설, 리졸버 루프 제거 | 누락 계약을 코드가 자인 |
| 3 | §6-1·6-2 타입 이동 | `FWxCharacterUIData`, `EWxTeam`을 WxCore로 | 잠재 결합 차단(저비용) |
| 4 | §5 태그 분리 | WxCombat 단일 도메인 태그를 WxCombat 내부 파일로 | WxCore 마모 방지(저우선) |
| 5 | §7 스냅샷 범위 | 콘텐츠 보유 플러그인 4종을 스냅샷 대상에 추가 | 감사 사각지대 제거 |

> 결론: 경계의 **하드 게이트(빌드 DAG)는 견고**하다. 남은 것은 "WxGame이 도메인 로직을 흡수하는 경향"과 "WxCore에 단일 도메인 항목이 침적되는 경향" 두 가지 엔트로피이며, 위 1~3번을 처리하면 의도한 모듈 그림에 다시 정렬된다.

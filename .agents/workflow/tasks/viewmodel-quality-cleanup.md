# ViewModel 코드 품질 정리

상태: 완료 · 체크리스트 5/5 통과
다음 행동: 변경 시 기록된 테스트 범위와 제약을 참고한다. MVVM 원칙에 맞춘 재설계는 viewmodel-mvvm-redesign.md에서 이어간다.

- woogle 결정(2026-09-29): "전부 다 고치는게 나을까요?" → AI 추천: 조사 2·3·4·5·8번은 이번에 고치고, 1번은 별도 작업, 7번은 나중에 하며, 6번은 뺀다.
- woogle 결정(2026-09-29): "6번은 지금은 안쓰는게 맞고, 나중에 필요해지면 그 때 구현하죠" → 바인딩이 없는 필드는 지금 지우고 필요해지면 다시 만든다. 6번을 이번 범위에 넣는다.
- woogle 결정(2026-09-29): 코드 리뷰 중 공유 VM 조회 방식을 검토했다. Outer 방식, 전용 서브시스템, Global Collection, MVVM 원칙(VM 계층 + 위에서 내려주기)을 비교한 뒤 "아까 얘기했던 MVVM 원칙에 맞게 뷰모델 재설계하는 것은 새 일감으로 만들고, 일단 이 일감은 완료 처리합시다." → 이 일감은 지금 구조(Outer 키 + `FindObjectWithOuter`)로 완료하고, 재설계는 viewmodel-mvvm-redesign.md로 넘긴다.
- woogle 결정(2026-09-29, 완료 처리 직후·커밋 전): "GetOrCreate 함수의 두번째 인자가 &&을 사용하는 이유가 있나요? 코드 파악이 좀 어렵네요" → "더 쉽고 직관적인 방법도 있나요?" → "AI 입장에서 작업할 때 뭐가 제일 낫나요?" → AI 추천(사람과 AI 모두 직접 쓰기가 낫다) → "네. 그렇게 합시다." 공용 `WxViewModel::GetOrCreate<T>` 템플릿을 없앤다. Character·AbilitySystem·Inventory의 `GetOrCreate`가 `FindObjectWithOuter`로 찾고, 없으면 `NewObject`로 만든 뒤 `Initialize`를 부르는 흐름을 직접 쓴다. `WxViewModelUtils`에는 `RequestImageAsync`만 남긴다. 코드가 바뀌므로 완료 상태를 풀고 코드 리뷰를 다시 받는다.
- woogle 제안(2026-09-29, 코드 리뷰 전): "WxViewModelBase 클래스를 지우고 GetOrCreateViewModel<T> 유틸 함수를 만들까요? 이유는 클래스 상속을 줄이고, 무의미한 가상함수도 없애기 위함입니다." → AI 추천: 찬성. 공유 조회는 템플릿으로, 이미지 로드는 유틸 함수로 옮기고, 파괴 시 정리는 없앤다. 같은 VM 파일을 건드리므로 이 작업에 합쳐 리뷰를 한 번에 받는다. → woogle 2026-09-29: "네 추천대로 하세요" (구현 계획 9번)

## 요청

- 요청 · woogle 2026-09-29

> ViewModel 관련 코드를 읽고 코드 품질을 개선할 수 있는 부분을 조사해주세요.

## 구현 계획

구현 승인: woogle 2026-09-29 ("네, 진행하세요.")

추가 질문 없음.

범위는 조사 결과의 2·3·4·5·6·8번이다. 1번(AbilitySystem VM 생성 경로를 하나로, WBP 2개 수정)은 별도 작업으로, 7번(InteractionList VM의 WxUI 이동)은 나중으로 뺀다. Inventory VM의 WxUI 이동은 `UWxItemDefinition` 의존 때문에 따로 설계해야 해서 다루지 않는다.

1. Setter/Getter 보일러플레이트 제거 (조사 2번)
   - `UWxViewModel_Ability`·`UWxViewModel_Effect`·`UWxViewModel_Attribute`에서 `Setter, Getter` 지정자와 한 줄짜리 Get/Set 함수 쌍을 지운다. 호출부는 다른 VM처럼 `UE_MVVM_SET_PROPERTY_VALUE`를 직접 쓴다.
   - 파생 필드를 함께 갱신하는 두 함수는 private으로 남긴다: Ability `SetMaxRecharges`(→ `HasMultipleCharges`), Effect `SetStackCount`(→ `IsStackCountAboveOne`).
   - `UWxViewModel_AbilitySystem::ActiveEffectViewModels`의 Getter는 유지한다. 이펙트 목록의 지연 구독이 이 Getter에 달려 있다.
   - Deinitialize 순서를 베이스 주석대로 "자기 정리 → Super"로 맞춘다.
   - 근거: 필드가 모두 `BlueprintReadOnly`라 Setter를 쓰는 곳이 없다. 엔진 MVVM은 Getter가 없으면 프로퍼티를 직접 읽는다(UE 5.8 `MVVMBindingHelper.cpp:705` `GetValue_InContainer`). 바인딩은 프로퍼티 이름을 가리키므로 WBP는 수정하지 않는다.
2. 비용 최대치 구독 제거 (조사 3번)
   - `UWxViewModel_Ability`의 `CostMaxAttribute` 필드, `"Max" + 이름` 리플렉션, 구독·해제를 지운다. 비용 자원(`CostAttribute`) 하나만 구독한다.
   - 근거: 비용은 CDO에 고정된 `CostAmount`(`UWxMMC_Cost`)이고 엔진 `CheckCost`는 현재값만 본다. 최대치 구독은 이미 지워진 `CostMax`·`CostRemainingPercent` 표시용이었다(b453d8517).
3. 리졸버 정리 (조사 4번, 8번 일부)
   - `WxViewModelResolver_AbilitySystem.cpp`: 익명 namespace를 없애고 호출부에서 `CreateLambda`로 쓴다.
   - `WxViewModelResolver_Ability`: 익명 namespace를 없애고, 자기 `CreateInstance`에서만 부르는 public static `GetOrCreate`를 `CreateInstance`에 합친다. 빈 태그 검사는 `GetOrCreateAbilityViewModel`에 맡긴다. ASC 조회는 AbilitySystem 리졸버와 같은 `UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent`로 맞춘다.
4. 엔진 기능을 다시 구현한 곳 정리 (조사 5번)
   - 직접 비교하고 통지하는 곳을 `UE_MVVM_SET_PROPERTY_VALUE`로 바꾼다(FText는 엔진이 `IdenticalTo`로 비교한다). 대상: Subtitle `ShowSubtitle`·`HideSubtitle`, QuestObjective `SetObjectiveText`, Indicator setter, Inventory `SetCurrentCategory`.
   - 남은 시간 계산을 `FActiveGameplayEffect::GetTimeRemaining(WorldTime)`으로 바꾼다. 대상: Effect `UpdateEffectState`, Ability `QueryCooldownStacks`.
5. 바인딩이 없는 필드 제거 (조사 6번, 근거: Content 전체 uasset 문자열 검색 0건)
   - Ability: `CanActivate`, `CooldownDuration`. 발동 가능 판정 자체는 `CheckCost`를 계산할 때 계속 쓴다.
   - Attribute: `IsAttributeEmpty`
   - Character: `Portrait`, 그리고 `GetOrCreate`·`Initialize`의 초상화 인자와 `ApplyLoadedImage` 재정의. 호출부 3곳(PlayerCharacter·Boss 리졸버, `WxNameplateManagerComponent`)에서 인자를 뺀다. `AWxCharacterBase::Portrait`·`GetIcon()`은 에디터 썸네일(`WxUIDataThumbnailRenderer.cpp:32`)이 쓰므로 남긴다.
   - Subtitle: `bHasSubtitle`
   - Quest: `bHasActiveQuest`와 `SetJournal`의 첫 인자. 그 결과 호출처가 없어지는 `UWxQuestComponent::HasActiveQuest()`도 지운다. 컴포넌트 내부의 `bHasActiveQuest` 필드는 로직에 쓰므로 남긴다.
   - Indicator: `bClamped`. `SetProjection(float, bool)`을 `SetCameraDistance(float)`로 바꾸고 호출부 `WxIndicator.cpp:188`을 고친다.
   - Inventory: `AllItems`는 내부 원본으로 두고 `FieldNotify`·`BlueprintReadOnly`·통지만 없애 private으로 옮긴다.
   - ConversionLibrary: `Conv_TagRequirementsToVisibility`
6. 사소한 정리 (조사 8번)
   - 쓰지 않는 `Engine/Texture2D.h` include를 지운다(Ability·Effect .cpp).
   - `WxViewModel_AbilitySystem.h`의 중복 전방 선언을 지우고, `WxViewModel_Ability.h`의 전방 선언을 델리게이트 선언 위로 올린다.
   - Attribute의 가득참·비율 계산이 세 곳에 반복된다. 기존 `RecalculateAttributePercent`를 파생 값 갱신 함수로 넓혀 하나로 모은다.
   - Ability `HandleCooldownTimer`의 불필요한 else `StopCooldownTimer`를 지운다.
7. 리뷰 순서: ① 1번(Setter/Getter 제거, 기계적 변경), ② 나머지 동작 정리, ③ 9번(베이스 VM 제거). 변경 파일 목록은 체크리스트의 코드 리뷰 항목에 적는다.
8. 검증 방법
   - build-doctor로 WxEditor Development를 빌드한다.
   - 헤드리스 에디터로 영향 WBP 16개를 로드·컴파일한다: WBP_Ability, WBP_ItemQuickSlot, WBP_PlayerSkills, WBP_Effect, WBP_AttributeBar, WBP_RadialBar, WBP_StaminaBar, WBP_Nameplate_Player/Enemy/Boss, WBP_Subtitle, WBP_QuestTracker, WBP_QuestObjective, WBP_QuestIndicator, WBP_Inventory, WBP_AcquiredItemList.
   - 헤드리스 게임에서 임시 자동화 테스트(`Source/WxGame/Tests/`, 확인 뒤 삭제하고 지운 상태로 다시 빌드)로 VM 값과 위젯 값을 함께 확인한다.

9. 베이스 VM `UWxViewModel` 제거 (추가 승인: woogle 2026-09-29 "네 추천대로 하세요")
   - VM 14개가 엔진 `UMVVMViewModelBase`를 직접 상속한다. `WxViewModel.h/.cpp`는 지운다.
   - 새 WxUI `MVVM/WxViewModelUtils.h/.cpp`, `namespace WxViewModel`(선례: `WxTargetingPreview`)
     - `GetOrCreate<T>(Source, Initialize)`: Source를 Outer로 만든 T를 찾고, 없으면 만들어 Initialize를 한 번 부른다. 정확한 클래스 일치와 수거 대기 객체 제외는 기존 규칙 그대로다. 템플릿이라 헤더에 두고 규칙 3 예외 주석을 단다. 사용처는 Character·AbilitySystem·Inventory의 `GetOrCreate`다.
     - `RequestImageAsync(Owner, InOutHandle, Image, OnLoaded)`: 같은 필드의 이전 요청 취소, 이미 로드된 경우 즉시 반영, 완료 콜백 안에서 새 요청을 건 경우의 보호를 한 곳에 둔다. VM은 이미지 필드마다 `TSharedPtr<FStreamableHandle>`를 하나 둔다(Ability·Effect·Item의 `Icon`). FName 슬롯 맵과 가상 `ApplyLoadedImage`는 없앤다.
   - `BeginDestroy`에서 부르던 가상 `Deinitialize`를 없앤다. VM의 구독은 모두 약참조이고 월드 타이머도 UObject에 묶여 있어, 엔진이 대상이 살아 있을 때만 실행하기 때문이다(`TimerManager.cpp:417`).
     - 파괴 때만 불리던 AbilitySystem·Inventory의 `Deinitialize`, 생성 직후 한 번뿐인 `Initialize`에서만 불리던 Ability·Attribute의 `Deinitialize`는 통째로 지운다.
     - 그 결과 `RF_BeginDestroyed` 분기도 없어진다.
     - 직접 부르는 곳이 있는 Character(보스 바)·Effect(효과 제거)·InteractionList(리졸버)의 `Deinitialize`는 일반 함수로 남긴다. Effect는 진행 중인 아이콘 요청도 여기서 취소한다.
   - 검증: 빌드, 위젯 BP 헤드리스 컴파일, 헤드리스 게임 테스트를 다시 돌린다. 버프 제거 뒤 필드 비움과 보스 교전 종료 뒤 비움처럼 `Deinitialize`에 걸린 항목, 그리고 아이콘 로드를 포함한다.

테스트 체크리스트 초안:

| 항목 | 확인 방법 | 담당 |
| --- | --- | --- |
| Editor 빌드 | 임시 테스트 제거 후 build-doctor 실행 | AI | 통과 | GetOrCreate를 직접 쓰기로 바꾼 뒤 다시 확인: build_2026-09-29_215249_454_6456.log Result Succeeded, 컴파일 경고 0. Source/WxGame/Tests 없음, Build.cs 변경 없음. git diff --check 통과 |
| 영향 WBP 로드·컴파일 | 헤드리스 에디터 CompileAllBlueprints(-BlueprintBaseClass=WidgetBlueprint)로 위젯 BP 전체를 저장 없이 컴파일 | AI | 통과 | 9번(VM 14개 부모 교체) 반영 뒤 다시 확인: 위젯 BP 88개 모두 successful, 0 error(경고 1건은 MCP 플러그인 EULA 안내). 영향 16개 모두 포함. Content에 베이스 클래스 이름 단독 참조 0건. WBP_Ability의 "Mask_Cooldown circularly depends" Display 메시지는 변경 전 에디터 로그(Saved/Logs/Wx.log)에도 있던 것. FindObjectWithOuter 전환 뒤 프로젝트 전체 블루프린트 571개도 컴파일: 570개 성공. 이번 변경과 무관한 기존 문제 2건: EUB_SnapToActor(NiagaraExamples 에디터 유틸리티, 켜지지 않은 DataprepLibraries 플러그인 함수 호출)는 실패, BP_ItemPickup은 HEAD에도 없는 WxInteractionComponent 컴포넌트 템플릿이 남아 있다는 경고만 내고 성공 |
| 스킬·이펙트·어트리뷰트 표시 | 헤드리스 게임(LV_DevCombat -game -nullrhi) 임시 자동화 테스트 Wx.Temp.ViewModelCleanup(확인 뒤 삭제) | AI | 통과 | 9번 반영 뒤 다시 실행. 공유 조회: AbilitySystem·Inventory·Character GetOrCreate가 기존 공유본을 돌려주고 Character 표시 데이터를 유지. 슬롯 5개의 MaxRecharges·HasMultipleCharges·CostAmount·CheckCost·Icon이 정의값·엔진 CheckCost와 같음. 회피 발동 → IsOnCooldown·남은 시간 감소·충전 1/2, 쿨다운 GE 제거 → 표시 비워짐·2/2. SP 0 → CheckCost false, Effect.IgnoreCosts 태그 → true, 태그 제거 → false, 복구 → true. HP 절반 → Percent 0.5·Full false, 가득 → 1·true. 즉석 5초 버프 → 제목·아이콘·Duration 5, 재적용 → 스택 2·IsStackCountAboveOne, 남은 시간 감소, 제거 → 목록에서 빠지고 Deinitialize로 필드·아이콘 비워짐. 아이콘 비동기 로드: 로드 안 된 T_UI_Cards 요청 직후 빈 값, 빈 값 재요청으로 앞 요청 취소, T_UI_Oxo 요청은 0.5초 뒤 채워짐. 스폰한 샌드백을 보스로 교전 → 보스 바 VM이 그 ASC VM·이름으로 채워지고 종료 → Deinitialize로 비워짐 |
| 자막·퀘스트·인디케이터·인벤토리 표시 | 같은 테스트. 인디케이터 투영은 -RenderOffscreen으로 한 번 더 실행 | AI | 통과 | 자막 표시 → VM·자막 위젯 텍스트에 문구, 다른 핸들 회수 무시, 회수 → VM·위젯 비워짐. 퀘스트 제목 → VM·트래커 텍스트, 목표 → VM·목표 ListView 항목(nullrhi는 엔트리 위젯을 만들지 않음). 인디케이터: -RenderOffscreen에서 액터 투영 → CameraDistance 12(기대 12)·위젯 Distance "12". 인벤토리 카테고리 3개 전환 → 목록이 그 카테고리만 담음(소모품 Potion×1, 아이콘 potion_full). nullrhi 75개·RenderOffscreen 76개 모두 통과. GetOrCreate 리뷰 수정 뒤와 FindObjectWithOuter 전환 뒤 임시 Wx.Temp.GetOrCreate(확인 뒤 삭제) 각각 11개 통과, 직접 쓰기 전환 뒤 10개 통과(공용 템플릿 전용 확인 1개는 빠짐, null 소스는 세 GetOrCreate 모두 확인): 플레이어 ASC의 AbilitySystem VM·그 Character VM·PC의 Inventory VM이 각각 하나, 세 GetOrCreate가 기존 공유본을 돌려주고 Character 표시 데이터 유지, 새 소스는 한 번만 만들고 Initialize 한 번, 같은 소스라도 클래스가 다르면 따로 만듦, null·가비지 표시 소스는 nullptr, 공유 인벤토리 VM이 목록을 채움 |
| 코드 리뷰 | 변경 42개 파일(추가 242줄·삭제 1113줄, 새 파일 WxViewModelUtils.h/.cpp 42줄 별도, 삭제 WxViewModel.h/.cpp는 git rm으로 스테이징됨). 다시 볼 곳: 공용 GetOrCreate 템플릿을 없애고 Character·AbilitySystem·Inventory의 GetOrCreate가 IsValid 검사 → FindObjectWithOuter 조회 → NewObject·Initialize를 직접 쓰는 부분(세 cpp), WxViewModelUtils.h에 RequestImageAsync만 남은 것. 나머지는 이전 리뷰 통과분 | 사람 | 통과 | woogle 2026-09-29 "테스트 결과 문제 없네요. 완료하고 제출합시다" (직접 쓰기 전환 전 통과: woogle 2026-09-29 "테스트 결과 잘 되고 이상 없네요. 아까 얘기했던 MVVM 원칙에 맞게 뷰모델 재설계하는 것은 새 일감으로 만들고, 일단 이 일감은 완료 처리합시다.") |

## 구현 · 2026-09-29

- 계획의 1~6번을 적용했다(30개 파일, 추가 179줄·삭제 706줄). 변경 파일: WxUI `MVVM/WxViewModel_Ability·Effect·Attribute·AbilitySystem·Character·Subtitle·Quest·QuestObjective·Indicator`, `WxMVVMConversionLibrary`, `Indicator/WxIndicator.cpp`, WxGame `MVVM/WxViewModelResolver_Ability·AbilitySystem·PlayerCharacter·BossCharacter·Quest`, `WxViewModel_Inventory`, `Controller/WxNameplateManagerComponent.cpp`, WxQuest `WxQuestComponent`.
- 계획에서 구현 중 바꾼 점 (승인 범위 안, `CanActivate` 삭제에 따른 결과)
  - 계획은 "발동 가능 판정은 CheckCost 계산에 계속 쓴다"였다. 하지만 `CanActivate` 필드가 없으면 `CanActivateAbility`를 먼저 부르는 것은 단축이 아니라 추가 비용이다. 그래서 `CheckCost`만 부르도록 바꾸고 함수 이름도 `RefreshActivationState`에서 `RefreshCheckCost`로 바꿨다. `Effect.IgnoreCosts`는 `UWxAbilityBase::CheckCost`가 직접 처리하므로 결과가 같다. 결과가 달라지는 경우는 엔진 전역 치트 `AbilitySystem.IgnoreCosts`를 켰을 때뿐이다.
  - 같은 이유로 쿨다운 적용·충전 수 변화 때의 재평가 호출 3곳을 지웠다. `CheckCost`는 비용 자원과 태그에만 달려 있다.
  - `CooldownDuration` 필드를 지우면서 `QueryCooldownStacks`의 회복 시간 출력 인자도 지웠다. 진행률 분모는 `CachedCooldownTime`을 바로 쓴다.
- 조사 중 Content 검색에서 grep이 만든 `Content/grep.exe.stackdump`(추적되지 않는 파일)는 지웠다.
- 9번(베이스 VM 제거) 구현: 계획대로 적용했다. 계획 밖으로 더 줄인 것: Ability의 `ActionPhaseChangedHandle`, Attribute의 `CachedASC`, Inventory의 `ReadyHandle`·`EndedHandle`은 파괴 시 해제용으로만 쓰여 지웠다. AbilitySystem의 private `Initialize` 안 null 검사는 `GetOrCreate`가 이미 거르므로 지웠다. Character `Deinitialize`는 직접 대입 뒤 통지하던 것을 `UE_MVVM_SET_PROPERTY_VALUE`로 바꿨다(이미 빈 값이면 통지하지 않음).
- 코드 리뷰 중 수정(woogle 2026-09-29 "GetOrCreate<T> 함수를 리뷰해주세요" → "코드가 좀 많이 어렵네요. 이렇게 해야되는 부분인가요?" → "제안해주신 코드로 수정해주세요"): `GetOrCreate<T>`의 `!Source`를 `IsValid(Source)`로 바꿨다. 그 결과 필요 없어진 `Garbage` 제외 플래그와 사실과 다른 주석을 지웠다(수거 대기 중인 Unreachable은 엔진이 기본으로 빼고, VM에 가비지 표시를 하는 코드는 없다). 헤더에 빠져 있던 include 4개(Casts·Object·SoftObjectPtr·UObjectGlobals)를 넣었다. AI가 처음 제안했던 조기 종료 순회(`ForEachObjectWithOuterBreakable`)와 `TFunctionRef`는 얻는 게 거의 없고 읽기만 어려워져 철회했다.
- 이어서 woogle(2026-09-29): "GetObjectWithOuter로 찾으면 너무 많이 나오지 않나요? 원하는 뷰모델만 바로 찾아서 얻으면 더 좋을 것 같아요" → 조회를 엔진 `FindObjectWithOuter(Source, T::StaticClass())`로 바꿨다. 직접 inner를 돌다 첫 일치에서 멈추고 하나만 돌려주는 함수라, 임시 배열과 루프가 없어진다. 클래스 비교가 정확한 일치에서 IsA로 바뀌었지만, VM끼리 상속하는 클래스가 C++·BP 모두 0건이라 결과는 같다. 그래서 "정확히 일치" 주석과 `Casts.h` include를 지웠다. 결과의 `static_cast<T*>`는 IsA 필터가 보장하고, 엔진도 이 함수의 결과를 같은 방식으로 캐스팅한다(`AnimDataModel.cpp:184` 등).
- GetOrCreate 직접 쓰기 전환(woogle 2026-09-29 "네. 그렇게 합시다."): `WxViewModel::GetOrCreate<T>` 템플릿을 지웠다. 세 VM의 `GetOrCreate`는 `IsValid(소스)` 검사 → `static_cast<T*>(FindObjectWithOuter(소스, StaticClass()))`로 공유본 조회 → 없으면 `NewObject<T>(소스)`와 `Initialize`로 흐름을 직접 쓴다. 각 블록에 "데이터 소스를 Outer 로 만들어 두므로, 소스의 자식 중에서 찾으면 공유본이다" 주석을 두었다. `WxViewModelUtils.h`는 `RequestImageAsync` 하나만 남아 규칙 3 예외가 없어졌고, include는 CoreMinimal·SoftObjectPtr만 남겼다.
- 검증 메모: nullrhi에서는 인디케이터 투영이 실패해 위젯 컴포넌트가 화면에 붙지 않고, 위젯도 생성되지 않는다. 그래서 테스트는 이 경우 VM에 값을 직접 넣고 위젯을 뷰포트에 붙여 바인딩만 확인했고, 액터 투영 경로는 -RenderOffscreen 실행으로 확인했다. 퀘스트 목표 엔트리 위젯도 nullrhi에서는 생성되지 않아 ListView 항목으로 확인했다.

## 조사 · 2026-09-29

- 범위: WxUI·WxGame의 VM·리졸버 44개 파일(약 3,800줄)을 정적으로 읽었다. 엔진 동작은 UE 5.8 소스와 대조했고, WBP 사용 여부는 Content uasset 문자열 검색으로 확인했다.
- 확인한 엔진 사실
  - `UMVVMViewModelBase::SetPropertyValue`는 FText를 `IdenticalTo`로 비교하고, 바뀌었는지 bool로 돌려준다(`MVVMViewModelBase.h:93`).
  - `FActiveGameplayEffect::GetTimeRemaining(WorldTime)`이 있다(`GameplayEffect.h:1372`).
  - MVVM은 말단 프로퍼티를 `GetValue_InContainer`로 읽어서, 네이티브 Getter가 있으면 Getter를 부른다(`MVVMBindingHelper.cpp:705`).
- Setter 함수를 클래스 밖에서 부르는 곳: `SetPresentation`뿐이다.
- 확인했지만 유지하는 것
  - VM별 월드 타이머: 40558da4c·0773a3588·eb92e99a7에서 일부러 옮긴 방식이다.
  - `GetActiveEffectViewModels`의 지연 초기화: 이펙트를 표시하지 않는 ASC가 구독하지 않게 하려는 것이다.
  - AbilitySystem·Ability의 다음 틱 합치기 중복: 공용 헬퍼로 묶지 않는다는 선호에 따라 둔다.
- 이미 올라가 있는 정확성 결함: 원격 클라이언트의 슬롯 재매칭 신호(`module_review_WxUI.md` 1번)는 이번 범위가 아니다.

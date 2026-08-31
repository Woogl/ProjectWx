# WxCore — 코드 리뷰

> foundation 모듈로서 지켜야 할 것들을 거의 그대로 지키고 있다. 도메인 타입 유입 없음, 태그 선언은 한 쌍의 파일에만 존재하며 선언 103개와 정의 103개가 완전히 일치하고, 심볼명↔태그 문자열도 전부 규칙대로다. 두 인터페이스의 모든 가상 함수는 호출자와 구현체가 실재해 죽은 계약이 없다. 이번 리뷰는 소스 11파일 전부를 읽고, 태그 103개를 `Plugins`·`Source`(C++)와 `Content`·`Config`(에셋 바이너리) 양쪽에서 참조 대조했으며, `FWxLocatorUtils`가 의존하는 엔진 `FActorLocatorFragment` 구현까지 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 참조가 하나도 없는 태그 선언이 공용 어휘에 남아 있다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:209`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:115` (그 외 `WxGameplayTags.h:202-205`, `WxGameplayTags.cpp:110-113`)
- **범주**: 중복/복잡도
- **문제**: `SetByCaller_Magnitude`("SetByCaller.Magnitude")는 C++ 코드(`Plugins`·`Source` 전체)에서도, 에셋·ini(`Content`·`Config` 바이너리 문자열 스캔)에서도 참조가 0이다. WxCore의 유일한 산출물이 "이 프로젝트가 쓰는 어휘 목록"인데, 실제로 쓰이는 SetByCaller 키는 `SetByCaller_Duration`(3개 GE)·`SetByCaller_Coeff_ATK`·`SetByCaller_MoveSpeedScale` 셋뿐이라, 새 GE를 만드는 사람이 "일반 크기 값은 SetByCaller.Magnitude로 넣는 게 관례인가" 하고 잘못 집는 함정이 된다.
  같은 스캔에서 `Ability_Pattern_6`~`Ability_Pattern_9`도 C++·에셋 어디에서도 참조가 없다(1~5는 에셋에서 쓰인다).
- **제안**: `SetByCaller_Magnitude`는 삭제한다(에셋 참조가 없으므로 태그 리다이렉트도 불필요). `Ability.Pattern.6~9`는 보스 패턴 슬롯을 미리 열어 둔 것으로 보이므로, 남길 거면 "미사용 예약 슬롯"이라는 한 줄 주석을 달아 데드 선언과 구분되게 한다.
- **확신도**: `SetByCaller_Magnitude`는 높음 / `Ability.Pattern.6~9`는 낮음(의도된 예약일 수 있음)

### 2. 🟢 `SetByCaller_Duration` doc-comment가 실제 사용처와 어긋난다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:211`
- **범주**: 버그/정확성(문서 정확성)
- **문제**: 주석은 "NoCooldown/InfiniteMP/DrainGP 등 Duration 모디파이어에서 공용으로 사용"이라고 적었지만, `UWxEffect_DrainGP`는 이 키를 전혀 쓰지 않는다 — 지속시간을 `WxAbility_Groggy.cpp:154`에서 `SpecHandle.Data->SetDuration(GroggyDuration, true)`로 직접 잠가서 넣는다(`WxEffect_DrainGP.h`의 클래스 주석도 그렇게 설명한다). 실제 사용처는 `WxEffect_Cooldown.cpp:11`, `WxEffect_InfiniteMP.cpp:12`, `WxEffect_NoCooldown.cpp:14`와 `WxAbilityBase.cpp:317` 뿐이다. README가 "태그 doc-comment가 곧 각 시스템의 계약 요약"이라고 선언한 모듈이므로, 이 어긋남은 다음 사람이 DrainGP를 SetByCaller 경로로 오해해 잘못 고치게 만든다.
- **제안**: 주석에서 DrainGP를 빼고 Cooldown/NoCooldown/InfiniteMP만 남긴다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxSavable.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`
- **교차 확인(참고용으로만 읽은 모듈 밖 파일)**: `Config/DefaultEngine.ini`(채널 등록), `Source/WxGame/Character/WxCharacterBase.cpp`(`ECC_WxAttack` override), `Plugins/WxCombat/.../WxEffect_DrainGP.cpp`·`WxAbility_Groggy.cpp`(SetByCaller 사용처), 각 도메인 `*.Build.cs`·`*.uplugin`(WxCore 의존 선언), 엔진 `ActorLocatorFragment.h/.cpp`
- **확인했고 문제 없었던 항목**: 코딩 규칙 — 11파일 전부 첫 줄 저작권 문구 존재, `FORCEINLINE`·인라인 함수 정의·람다·`BlueprintCallable`·`UFUNCTION` 사용 0건(`WxCollisionChannels.h:15`의 `inline constexpr`은 변수이므로 규칙 6 대상 아님), Wx 접두사 준수, 델리게이트 자체가 없어 `Handle` 규칙 해당 없음. 모듈 규칙 — `WxCore.Build.cs`는 엔진 모듈만 의존하고 Wx 참조가 없으며, `UniversalObjectLocator`는 `bBuildEditor` 게이트 + `WITH_EDITOR` 구현으로 정합하고 이를 쓰는 모듈 5곳 모두 자기 Build.cs에 같은 의존을 선언한다. 계약 — `IWxInteractable`·`IWxSavable`의 가상 함수 7개 모두 호출자와 구현체가 실재(죽은 방어적 선언 없음). 태그 — 선언/정의 심볼 집합 103개 완전 일치, 심볼명↔문자열 전 항목 일치, 프로젝트 전체에서 네이티브 태그 선언은 WxCore 밖에 단 한 건도 없음. 콜리전 — `ECC_WxAttack = ECC_GameTraceChannel1`이 `DefaultEngine.ini`의 `WxAttack` 등록과 일치하고, 헤더가 서술한 "메시 Overlap / 캡슐 Ignore" override도 `WxCharacterBase.cpp:30,34`와 일치.
- **미검토 / 한계**: `Device.*` 태그가 실제로 어떤 State Tree 상태에 붙어 있는지는 에셋 내부 구조라 확인하지 않았다(문자열 참조 존재만 확인). `FWxLocatorUtils`의 World Partition 미로드 액터·PIE 상황별 표시 결과는 정적 분석만 했고 에디터에서 실측하지 않았다.

---
*문서 기준 커밋 `ba33d69e` · 리뷰일 2026-09-01 · 소스 11파일 — `/module-review`로 갱신*

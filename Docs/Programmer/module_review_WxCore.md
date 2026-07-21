# WxCore — 코드 리뷰

> 선언·상수·계약만 담는 foundation 모듈답게 매우 깨끗하다. 8개 소스 전부(Build.cs·uplugin·헤더·cpp)를 통독했고, 콜리전 채널 상수와 인터페이스가 실제 소비 도메인(WxWorld/WxSave)에서 규약대로 쓰이는지까지 교차 확인했다. 심각·개선 등급 결함은 없으며 문서/주석 정합성 관련 사소 항목만 남는다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟢 인터페이스 주석이 존재하지 않는 메서드명 `GetWxSaveId()`를 지칭
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxSavable.h:14`
- **범주**: 규칙/정합성 (문서-코드 불일치)
- **문제**: 클래스 주석은 "`UPROPERTY(SaveGame)` 필드가 `GetWxSaveId()` 의 WxSaveId 키로 슬롯에 기록된다"라고 설명하지만, 실제 선언된 순수 가상 함수는 `GetSaveId()`(라인 36)다. `GetWxSaveId`라는 심볼은 이 모듈은 물론 저장소 전체에 존재하지 않는다(소비처 `WxSave`/`WxWorld`도 모두 `GetSaveId()`를 오버라이드·호출). 같은 오표기가 `Plugins/WxCore/README.md`와 `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h:96` 주석에도 전파되어 있어, 계약 이름을 이 주석으로 학습하는 사람을 오도한다.
- **제안**: WxSavable.h 주석의 `GetWxSaveId()`를 `GetSaveId()`로 정정한다(동작 변화 없음). 필요 시 README·WxSaveGame.h 주석도 동일하게 맞춘다.
- **확신도**: 높음 (코드 grep으로 심볼 부재 확인).

### 2. 🟢 콜리전 채널의 ini 순서 의존이 컴파일 타임에 강제되지 않음
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h:20-21`
- **범주**: 설계/안전 (단일 출처 취약성)
- **문제**: `ECC_WxAttack = ECC_GameTraceChannel1`, `ECC_WxInteractable = ECC_GameTraceChannel2`로 하드코딩되어 있고, 이 매핑이 `Config/DefaultEngine.ini`의 `DefaultChannelResponses` 등록 순서와 일치해야만 정상 동작한다. 현재 값은 ini와 정확히 일치(GameTraceChannel1=WxAttack, GameTraceChannel2=WxInteractable)해 문제없으나, ini에서 채널을 재정렬·삽입하면 코드는 그대로 컴파일되면서 프로젝트 전역의 히트/상호작용 판정이 조용히 엉키는 고파급 회귀가 발생한다. 두 출처를 잇는 컴파일 타임·기동 시 검증 장치가 없다.
- **제안**: 낮은 우선순위. UE 특성상 ini는 런타임 데이터라 `static_assert`로는 묶기 어렵지만, 모듈 `StartupModule()`(현재 빈 구현)에서 `UCollisionProfile`로 채널명↔인덱스를 조회해 상수와 대조하는 개발 빌드 전용 `ensure`를 두면 재정렬 회귀를 조기에 잡을 수 있다. 현 상태가 정상이므로 필수는 아니다.
- **확신도**: 낮음 (의도된 설계이자 UE의 알려진 제약일 수 있음. 결함이 아니라 취약성 지적).

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`(선언·정의 쌍 대조), `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxCore/Source/WxCore/Public/WxInteractionSource.h`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxAbilityComponent.h`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`
- **교차 확인**: `Config/DefaultEngine.ini`(채널 등록 순서), `Plugins/WxWorld`·`Plugins/WxSave`의 `IWxSavable`/`IWxInteractionSource` 구현·소비처(계약 준수 확인)
- **점검했으나 문제 없음**:
  - 모듈 경계 — Build.cs 의존은 `Core`/`CoreUObject`/`Engine`/`GameplayTags` 엔진 모듈뿐, uplugin의 Plugins 의존 0. 다른 Wx 플러그인 미참조 규칙 준수.
  - CLAUDE.md 코딩 규칙 — 8개 소스 전부 첫 줄 `// Copyright Woogle. All Rights Reserved.` 존재, `Wx` prefix 일관, `BlueprintCallable` 오용 없음, 불필요한 람다 없음, `Handle`/`Super::` 위반 대상 코드 자체가 없음(순수 선언 모듈).
  - Gameplay Tag — `WxGameplayTags.h`의 `UE_DECLARE_..._EXTERN`와 `.cpp`의 `UE_DEFINE_...`가 1:1로 완전 대응(선언만 있고 정의 누락, 또는 그 반대인 태그 없음). 변수명↔문자열 치환 규칙(`_`↔`.`)도 일관. 크로스 모듈 참조용 `WXCORE_API` 부착 정상.
- **미검토 / 한계**: 없음 (모듈 규모가 작아 전량 검토).

---
*문서 기준 커밋 `9661edf` · 리뷰일 2026-07-21 · 소스 8파일 — `/module-review`로 갱신*

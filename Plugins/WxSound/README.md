# WxSound — BGM/음악 시스템

> 게임 상태(플레이어 상태 태그 + BGM 분류 태그)를 Chooser 테이블로 평가해 상황에 맞는 배경음악을 골라 크로스페이드로 재생한다. 완전 이벤트 구동, 로컬 전용.

## 책임
**담당**
- 상태→곡 선택: `FWxBGMChooserContext`(플레이어 ASC owned-tag + BGM 분류 태그 + 소스 소유자)를 Chooser 테이블에 넣어 `UWxBGMData` 를 고른다.
- 재생/전환: 선택된 곡이 직전과 다를 때만 이전 곡 페이드아웃 + 새 곡 페이드인(같으면 no-op, `nullptr` 이면 페이드아웃). 페이드 동안 직전 컴포넌트를 보유해 GC 를 막는다.
- 완전 이벤트 구동 재평가: `OnLocalPlayerAddedEvent` → 컨트롤러 교체 → 폰 교체(`OnPossessedPawnChanged`) → 폰 ASC owned-tag 변경(`RegisterGenericGameplayTagEvent`), BP `StartBGM` 주입, 상태 기반 소스 등록/해제 — 이 이벤트들만 재평가를 트리거하며 주기 폴링/타이머가 없다.
- 상태 기반 국지 BGM 소스(`UWxBGMSourceComponent`) 등록·우선순위 조정과 베이스라인 자동 폴백.
- Blueprint 진입점(`StartBGM`/`StopBGM`)과 프로젝트 설정(Chooser 테이블 지정) 제공.

**경계 (비담당)**
- 언제 어떤 BGM 분류를 켤지 결정 — 게임플레이/BP 가 `StartBGM(태그)` 호출로 주도.
- 플레이어 상태(전투/보스/지역 등) 판정과 태그 부여 — 이 모듈은 플레이어 ASC 의 owned-tag 를 읽기만 하며, 부여는 ASC 소유자([[WxCombat]] 등 도메인)가 한다.
- 사운드 에셋 자체의 루프/믹싱 — Sound Cue/Wave 에 위임한다.
- 데디케이티드 서버 오디오 — BGM 은 로컬 전용이라 서버에서는 아무 것도 하지 않는다.

## 의존성
- **주요 의존**: `WxCore` · `GameplayAbilities`(ASC owned-tag 이벤트/조회) · `Chooser`(테이블 평가) · `GameplayTags` · `DeveloperSettings`
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (`.Build.cs`·`.uplugin` 모두 Wx 계열은 `WxCore` 만, 나머지는 엔진 모듈/플러그인)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxMusicLibrary` | BP 진입점. World 서브시스템으로 위임하는 thin wrapper(`StartBGM`/`StopBGM`) | `Plugins/WxSound/Source/WxSound/Public/WxMusicLibrary.h` |
| `UWxMusicSubsystem` | 모든 입력 이벤트가 수렴하는 재평가·재생 허브(WorldSubsystem). 이벤트 바인딩·크로스페이드가 여기 | `Plugins/WxSound/Source/WxSound/Public/System/WxMusicSubsystem.h` |
| `UWxBGMSourceComponent` | 소유 액터 상태 태그(예: `State.InCombat`)가 켜진 동안 자기 분류 태그를 서브시스템에 기여하는 소스 | `Plugins/WxSound/Source/WxSound/Public/WxBGMSourceComponent.h` |
| `FWxBGMChooserContext` | Chooser 평가 입력 struct. `PlayerStateTags`(ASC 스냅샷) + `BGMTag`(BP 주입) + `SourceOwner`(승자 소스 액터) | `Plugins/WxSound/Source/WxSound/Public/WxBGMChooserContext.h` |
| `UWxBGMData` | 한 BGM 트랙 정의이자 Chooser 결과 타입. `Sound` + 곡별 `FadeInTime`/`FadeOutTime` | `Plugins/WxSound/Source/WxSound/Public/WxBGMData.h` |
| `UWxMusicSettings` | 프로젝트 설정(Project Settings > Wx > Wx Music Settings). `DefaultBGMChooser` 테이블 지정 | `Plugins/WxSound/Source/WxSound/Public/System/WxMusicSettings.h` |

## 확장 포인트 / 규약
- **새 곡 추가**: `UWxBGMData` 에셋을 만들어 `Sound`·페이드 값을 넣고, Chooser 테이블의 한 행 결과로 연결한다. 페이드 길이는 곡별로 트랙 자신이 소유한다.
- **Chooser 규약**: 테이블의 Result Class = `UWxBGMData`, Parameter = `FWxBGMChooserContext`. 컬럼은 `PlayerStateTags`(Gameplay Tag)·`BGMTag`(Gameplay Tag)·`SourceOwner` 클래스(Object Class, 예: SubClassOf 보스 캐릭터)에 바인딩된다.
- **새 입력 키 추가**: `FWxBGMChooserContext` 에 `UPROPERTY` 멤버를 추가하고 `UWxMusicSubsystem::EvaluateBGM` 에서 채운 뒤 테이블 컬럼에 바인딩한다. (컬럼 대상이 되려면 멤버가 `UPROPERTY` 로 노출돼야 한다.)
- **새 상태를 BGM 에 반영**: 별도 감지 배선 없이 해당 상태를 플레이어 ASC 에 GameplayTag 로 부여하면 `PlayerStateTags` 로 자동 유입되어 재평가를 트리거한다.
- **국지적 BGM(보스 등)**: 액터에 `UWxBGMSourceComponent` 를 붙이고 `ActivationTag`(감시할 상태 태그)·`MusicTag`(기여 분류)·`Priority` 를 설정. 활성 소스 중 최고 우선순위(동률은 최근)가 이기고, 소스가 빠지면 `StartBGM` 베이스라인으로 자동 폴백된다.

## 여기서부터 읽어라
1. `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp` — 이벤트 구동 재평가 파이프라인의 심장. `OnWorldBeginPlay` 부트스트랩 → `Handle*` 델리게이트 체인 → `Reevaluate`/`EvaluateBGM`/`ApplyBGM` 흐름을 먼저 잡는다.
2. `Plugins/WxSound/Source/WxSound/Public/WxMusicLibrary.h` — 외부에서 이 모듈을 두드리는 유일한 BP 진입점(`StartBGM`/`StopBGM`).
3. `Plugins/WxSound/Source/WxSound/Public/WxBGMChooserContext.h` + `Plugins/WxSound/Source/WxSound/Public/WxBGMData.h` — Chooser 의 입력 계약과 결과 타입. 선택 로직이 무엇을 읽고 무엇을 내놓는지의 양 끝.
4. `Plugins/WxSound/Source/WxSound/Public/WxBGMSourceComponent.h` — 상태 기반 국지 BGM 이 우선순위로 서브시스템에 끼어드는 방식.

## 관련
- 상위: 게임플레이 BP/코드가 `UWxMusicLibrary`(BlueprintCallable)로 진입해 BGM 분류를 켠다. 상태 태그 공급원은 플레이어 ASC — [[WxCombat]] 등 도메인 모듈이 부여한 owned-tag 를 그대로 읽는다.

---
*문서 기준 커밋 `1a693b0` · 생성일 2026-07-02 · 소스 12파일 — `/readme-writer`로 갱신*

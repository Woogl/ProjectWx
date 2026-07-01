# WxSound — 오디오/배경음악 시스템

> 게임 상태(전투/보스/플레이어 상태태그/BGM 분류 태그)를 Chooser 테이블로 평가해 적절한 BGM 을 골라 크로스페이드 재생한다. 로컬 전용으로 동작한다.

## 책임
**담당**
- BGM 분류 태그 + 로컬 플레이어 ASC 의 owned-tag 를 입력으로 Chooser 테이블을 평가해 곡 선택
- 선택된 곡으로의 크로스페이드 전환(곡별 페이드 인/아웃 시간), 페이드 동안 직전 컴포넌트 보유
- 주기 타이머/이벤트(폰 교체) 기반 재평가, StartBGM/StopBGM 보류 상태 관리
- 트랙 정의(`UWxBGMData`) 및 프로젝트 설정(`UWxMusicSettings`) 데이터 주도 구성

**경계 (비담당)**
- 전투/보스/지역 등 "상태" 자체의 판정은 안 함 — 해당 상태를 플레이어 ASC 에 태그로 부여하면 `PlayerStateTags` 로 그대로 잡힘
- BGM 분류 태그를 언제 바꿀지(전투 진입 등)는 호출 측이 `UWxMusicLibrary::StartBGM` 으로 결정
- SFX/사운드 에셋 루프 자체는 Sound Cue/Wave 에 위임

## 의존성
- **주요 의존**: `WxCore` / `Chooser`(테이블 평가) / `GameplayAbilities`(로컬 ASC owned-tag 조회) / `GameplayTags` / `DeveloperSettings`
- 규칙: 플러그인 의존은 `WxCore`, `GameplayAbilities`, `Chooser` 뿐 — WxCore 외 Wx 플러그인 참조 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxMusicLibrary` | Blueprint 진입점. StartBGM/StopBGM 을 서브시스템으로 위임하는 thin wrapper | `Plugins/WxSound/Source/WxSound/Public/WxMusicLibrary.h` |
| `UWxMusicSubsystem` | 핵심 엔진. 재평가→선택→크로스페이드의 전 과정을 담당하는 월드 서브시스템 | `Plugins/WxSound/Source/WxSound/Public/System/WxMusicSubsystem.h` |
| `FWxBGMChooserContext` | Chooser 에 넘기는 Struct Parameter. 평가 입력(PlayerStateTags/BGMTag) | `Plugins/WxSound/Source/WxSound/Public/WxBGMChooserContext.h` |
| `UWxBGMData` | 한 BGM 트랙 정의이자 Chooser 의 결과 타입(Sound + 페이드 시간) | `Plugins/WxSound/Source/WxSound/Public/WxBGMData.h` |
| `UWxMusicSettings` | 프로젝트 설정. 사용할 Chooser 테이블과 재평가 주기 | `Plugins/WxSound/Source/WxSound/Public/System/WxMusicSettings.h` |

## 확장 포인트 / 규약
- 새 곡 추가: `UWxBGMData` 데이터 에셋을 만들고 Sound/페이드 시간을 채운 뒤, Chooser 테이블의 행 결과로 연결한다.
- 선택 규칙 변경: `UWxMusicSettings::DefaultBGMChooser` 가 가리키는 Chooser 테이블을 편집한다. (Result Class = `UWxBGMData`, Parameter = `FWxBGMChooserContext`)
- 새 입력 키 추가: `FWxBGMChooserContext` 에 `UPROPERTY` 멤버를 추가하고 서브시스템의 `EvaluateBGM` 에서 채운 뒤, 테이블 컬럼에 바인딩한다.
- 상태 반영: 별도 감지 코드 없이 플레이어 ASC 에 태그를 부여하면 `PlayerStateTags` 로 흘러든다.

## 여기서부터 읽어라
1. `Plugins/WxSound/Source/WxSound/Public/System/WxMusicSubsystem.h` — 전체 흐름(재평가/선택/크로스페이드/보류)의 골격이 여기 다 있음
2. `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp` — `EvaluateBGM`/`ApplyBGM` 의 실제 Chooser 호출·페이드 처리
3. `Plugins/WxSound/Source/WxSound/Public/WxBGMChooserContext.h` — 데이터 계약(테이블 컬럼이 무엇에 바인딩되는지)

## 관련
- 상위: 호출 측(게임플레이/레벨/[[WxCombat]] 등)이 [[WxCore]] 의 태그로 `UWxMusicLibrary::StartBGM` 을 호출해 BGM 분류를 전환한다.

---
*문서 기준 커밋 `9e49a09` · 생성일 2026-06-27 · 소스 11파일 — `/readme-writer`로 갱신*

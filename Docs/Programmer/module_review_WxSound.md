# WxSound — 코드 리뷰

> 12파일 규모의 작고 응집도 높은 모듈로, 프로젝트 코딩·모듈 규칙 위반이 없고 델리게이트 수명·서브시스템 정리도 정갈하다. 남은 문제는 오디오 API 사용 디테일, 이벤트 커버리지 공백, 에셋 로딩 쪽에 몰려 있다. 이번 리뷰는 `Source/` 전 12파일을 읽었고 `WxMusicSubsystem.cpp`·`WxBGMSourceComponent.cpp` 는 엔진 소스(UE 5.8)와 대조해 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 SpawnSound2D 직후 FadeIn 은 곡을 한 번 재생했다가 정지·재시작한다
- **위치**: `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp:323`, `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp:326`
- **범주**: 버그/정확성
- **문제**: `UGameplayStatics::SpawnSound2D` 는 컴포넌트를 만든 뒤 곧바로 `AudioComponent->Play(StartTime)` 를 호출한다(엔진 `GameplayStatics.cpp:1691-1699`). 그 상태에서 `FadeIn` 을 부르면 `PlayInternal` 이 `IsActive()` 를 참으로 보고 `Stop()` 후 처음부터 다시 재생한다(엔진 `AudioComponent.cpp:690-700`·`962-971`, 활성 플래그 설정은 `928`). 즉 곡 전환 1회마다 ActiveSound 를 만들어 즉시 버리고 다시 만든다. 첫 재생은 페이드 없이 풀 볼륨이라 오디오 스레드가 두 명령 사이에 렌더하면 어택이 새어나갈 수 있고, 스트리밍 음원이면 스트림 요청도 두 번 발생한다.
- **제안**: `UGameplayStatics::CreateSound2D(...)`(재생 없이 컴포넌트만 생성) 로 바꾼 뒤 `FadeIn` 을 호출한다.
- **확신도**: 높음 (이중 재생 자체는 확정. 가청 여부는 오디오 스레드 타이밍에 따라 달라짐)

### 2. 🟡 SourceOwnerTags 변경이 재평가를 트리거하지 못한다
- **위치**: `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp:273-281` (읽기 지점) · `Plugins/WxSound/Source/WxSound/Private/WxBGMSourceComponent.cpp:41` (구독 지점)
- **범주**: 버그/정확성 (미처리 이벤트 경로)
- **문제**: `EvaluateBGM` 은 활성 소스 소유자 액터 ASC 의 owned-tag 를 매 평가마다 읽어 Chooser 컬럼(`SourceOwnerTags`)으로 노출하지만, 재평가를 유발하는 이벤트는 (a) 로컬 플레이어 ASC 태그 변경 (b) `StartBGM`/`StopBGM` (c) 소스 등록/해제뿐이다. 소스 컴포넌트는 `ActivationTag` 존재 여부만 구독하므로, 보스 자신의 태그가 바뀌어도(예: `Enemy.Boss.Phase2` 부여) 재평가가 걸리지 않는다. `SourceOwnerTags` 컬럼으로 페이즈 BGM 을 분기하면 무관한 플레이어 태그 변경이 우연히 발생할 때까지 곡이 갱신되지 않는다. "완전 이벤트 구동"이라는 모듈 계약에서 유일하게 이벤트가 없는 입력이다.
- **제안**: 소유자 태그를 정적 식별자(`Enemy.Boss.Dragon`)로만 쓰는 것이 의도라면 그 제약을 `FWxBGMChooserContext` 주석과 README 에 못박는다. 동적 반영이 필요하면 `RegisterBGMSource` 시점에 소유자 ASC 의 generic tag 이벤트도 구독하고 `UnregisterBGMSource` 에서 해제한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 실전에서는 플레이어 태그 변동이 잦아 체감 지연이 작을 수 있다)

### 3. 🟡 Chooser 테이블 동기 로드가 전체 BGM 에셋 그래프를 함께 끌어온다
- **위치**: `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp:35`, `Plugins/WxSound/Source/WxSound/Public/WxBGMData.h:24`
- **범주**: 성능/안전
- **문제**: `OnWorldBeginPlay` 에서 `DefaultBGMChooser.LoadSynchronous()` 로 테이블을 즉시 로드한다. Chooser 의 기본 결과 타입 `FAssetChooser` 는 결과를 하드 참조하고(`TObjectPtr<UObject> Asset`, 엔진 `ObjectChooser_Asset.h:24-25`), `UWxBGMData::Sound` 역시 하드 `TObjectPtr<USoundBase>` 다. 따라서 테이블 로드 한 번에 게임 내 모든 `UWxBGMData` 와 그 사운드 에셋이 레벨 시작 시점에 동기 로드되어 상주한다 — 오픈월드에서 지역·보스별 곡이 늘어날수록 진입 히칭과 상주 메모리가 함께 커진다.
- **제안**: `Sound` 를 `TSoftObjectPtr<USoundBase>` 로 바꿔 `ApplyBGM` 시점 로드(또는 사전 비동기 프리로드)하거나, 테이블 행 결과를 "Asset (Soft Reference)"(`FSoftAssetChooser`) 로 두고 테이블 자체도 비동기 로드한다. 소프트 행을 쓰면 평가 API 도 soft 변형으로 맞춰야 한다.
- **확신도**: 중간 (현재 `DefaultBGMChooser` 가 미설정이라 실제 에셋 그래프로 실측 검증 불가. 곡 수를 적게 유지한다면 의도된 트레이드오프일 수 있음)

### 4. 🟢 태그 이벤트 1건마다 Chooser 전체 평가가 돈다 (부모 태그까지 곱해짐)
- **위치**: `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp:232`, `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp:212`, `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp:237`
- **범주**: 성능/안전
- **문제**: `RegisterGenericGameplayTagEvent` 는 태그 카운트가 0↔N 으로 바뀔 때 **해당 태그와 그 부모 태그 각각에 대해** 브로드캐스트한다(엔진 `GameplayEffectTypes.cpp:869-879`). `State.Combat.Attacking` 하나를 붙였다 떼는 것만으로 최대 6회 콜백이 오고, 매 회 `EvaluateBGM` 이 ① owned 태그 컨테이너 복사(`:248`) ② 활성 소스마다 ASC 조회 + 임시 컨테이너 복사(`:275-280`) ③ `MakeChooserEvaluationContext`/`MakeEvaluateChooser` 힙 할당(`:285-291`) ④ 테이블 전체 평가를 수행한다. 결과는 거의 항상 직전 곡과 같아 `ApplyBGM` 이 조기 반환하므로 청각 영향은 없고 순수 CPU 낭비다.
- **제안**: 실측 후 문제가 되면 콜백에서 dirty 플래그만 세우고 다음 틱에 한 번만 `Reevaluate` 하도록 코얼레싱하거나, 제네릭 이벤트 대신 테이블이 실제로 읽는 태그만 `RegisterGameplayTagEvent` 로 개별 구독한다.
- **확신도**: 중간 (호출 빈도 배수는 확정이나, 현 규모에서 실제 병목인지는 프로파일 전엔 불명)

### 5. 🟢 곡이 끝나면 같은 곡의 재선택이 영구 no-op 이 된다
- **위치**: `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp:299`, `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp:323`
- **범주**: 버그/정확성
- **문제**: `SpawnSound2D` 는 `bAutoDestroy` 기본값이 true 라 비루프 사운드는 끝나면 컴포넌트가 스스로 파괴된다. 그런데 `CurrentBGM` 은 그대로 남으므로, 이후 어떤 재평가가 같은 `UWxBGMData` 를 골라도 `NewBGM == CurrentBGM` 조기 반환에 걸려 다시 재생되지 않는다. Sound Cue 에 Looping 을 깜빡한 트랙은 한 번 끝난 뒤 `StopBGM`+`StartBGM` 을 명시 호출하기 전까지 영구 무음이 되고, 원인을 추적할 단서가 없다.
- **제안**: `OnAudioFinished` 를 구독해 `CurrentBGM`/`CurrentComponent` 를 함께 비우거나, 조기 반환 조건에 `CurrentComponent != nullptr` 를 함께 본다.
- **확신도**: 중간 (README·`WxBGMData` 주석이 "루프는 Sound 에셋 책임"이라 못 박고 있어 의도된 계약일 수 있음)

### 6. 🟢 실패 경로가 전부 무성·무로그다
- **위치**: `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp:32-36`, `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp:239`, `Plugins/WxSound/Source/WxSound/Private/WxMusicLibrary.cpp:14-20`
- **범주**: 버그/정확성 (미처리 실패 경로)
- **문제**: 모듈 전체에 `UE_LOG` 가 한 줄도 없다. `DefaultBGMChooser` 미설정·로드 실패, Chooser 결과가 `UWxBGMData` 가 아님, `StartBGM` 이 서브시스템을 못 찾음 — 전부 조용히 no-op 이다. 실제로 현재 `Config/` 어디에도 `DefaultBGMChooser` 항목이 없어 `Chooser` 는 항상 null 이고 BGM 이 절대 재생되지 않는 상태인데, 기획자나 다음 세션이 이를 알아낼 방법이 코드 열람뿐이다.
- **제안**: 최소한 `Chooser` 미설정/로드 실패에 `UE_LOG(..., Warning, ...)` 한 줄을 남긴다.
- **확신도**: 높음

### 7. 🟢 `UWxBGMSourceComponent` 클래스 주석이 구현과 어긋난다
- **위치**: `Plugins/WxSound/Source/WxSound/Public/WxBGMSourceComponent.h:16-17`
- **범주**: 중복/복잡도 (문서-구현 불일치)
- **문제**: 주석은 "서브시스템은 활성 소스 중 가장 최근 등록된 MusicTag 를 분류 태그로 골라 Chooser 를 평가"한다고 설명하지만, 실제 `EvaluateBGM` 은 활성 소스 전부의 `MusicTag` 를 합집합으로 넣고 승자를 고르지 않는다(`Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp:264-283`). 우선순위는 Chooser Row 순서가 결정한다. 이 주석을 믿으면 "등록 순서를 조작해 우선순위를 바꾸려는" 잘못된 수정으로 이어진다.
- **제안**: 합집합 + Row 순서 우선순위로 주석을 정정한다(README 와 `FWxBGMChooserContext` 주석은 이미 올바르다).
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp`, `Plugins/WxSound/Source/WxSound/Public/System/WxMusicSubsystem.h`, `Plugins/WxSound/Source/WxSound/Private/WxBGMSourceComponent.cpp`, `Plugins/WxSound/Source/WxSound/Public/WxBGMSourceComponent.h`
- **훑은 파일**: `Plugins/WxSound/Source/WxSound/Private/WxMusicLibrary.cpp`, `Plugins/WxSound/Source/WxSound/Public/WxMusicLibrary.h`, `Plugins/WxSound/Source/WxSound/Public/WxBGMChooserContext.h`, `Plugins/WxSound/Source/WxSound/Public/WxBGMData.h`, `Plugins/WxSound/Source/WxSound/Public/System/WxMusicSettings.h`, `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSettings.cpp`, `Plugins/WxSound/Source/WxSound/Public/WxSoundModule.h`, `Plugins/WxSound/Source/WxSound/Private/WxSoundModule.cpp`, `Plugins/WxSound/Source/WxSound/WxSound.Build.cs`, `Plugins/WxSound/WxSound.uplugin`, `Plugins/WxSound/README.md`
- **규칙 점검(위반 없음)**: 전 파일 첫 줄 Copyright ✅ / `Wx` prefix ✅ / 델리게이트 콜백 `Handle` prefix ✅(`HandleLocalPlayerAdded`·`HandlePlayerControllerChanged`·`HandlePawnChanged`·`HandleOwnedTagsChanged`·`HandleActivationTagChanged`) / override 의 `Super::` 호출 ✅(`OnWorldBeginPlay`·`Deinitialize`·`BeginPlay`·`EndPlay`) / `BlueprintCallable` 은 `UWxMusicLibrary`(BP Function Library) 에만 ✅ / 람다 0건 ✅ / Wx 계열 의존은 `WxCore` 뿐 ✅ (다만 `WxCore` 헤더를 실제로 include 하는 파일은 하나도 없어 선언만 남은 의존이다).
- **미검토 / 한계**: Chooser 테이블 에셋이 아직 프로젝트에 없어 컬럼 바인딩·Row 우선순위 계약은 실증 검증하지 못했다(발견 3의 확신도가 중간인 이유). 델리게이트 수명(`AddUObject`/`AddDynamic` 의 weak 참조), 서브시스템 `Deinitialize` 정리, 네트워크 클라이언트의 `OnRep_Pawn` → `OnPossessedPawnChanged` 브로드캐스트 경로(엔진 `Controller.cpp:566-573`)는 엔진 소스로 대조해 문제없음을 확인했다. 이전 리뷰가 제기했던 두 항목은 이번엔 발견에서 제외했다 — ① 스플릿스크린 시 단일 `BoundController` 가 마지막 로컬 플레이어로 덮이는 문제(싱글플레이어 전제상 무해) ② `StopBGM` 보류 중 소스 등록이 무시되는 동작(README 에 명시된 의도된 시맨틱). 이전 리뷰의 `PlayerStateTags.Reset()` 중복 지적은 현재 코드(`:245-254`)에서 이미 해소되었다.

---
*문서 기준 커밋 `c42b5fec` · 리뷰일 2026-07-25 · 소스 12파일 — `/module-review`로 갱신*

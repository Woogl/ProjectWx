# WxSound — 코드 리뷰

> 이벤트 구동 Chooser 기반 BGM 모듈로, 델리게이트 수명주기·크로스페이드 GC 방지·모듈 경계·명명 규칙 모두 정갈하다. 심각한 결함(🔴)은 없고, 이벤트 커버리지 공백과 멀티 로컬플레이어 가정 등 설계 스코프 성격의 개선점 위주다. 12개 소스 전부와 핵심 로직(Subsystem·SourceComponent)을 정독했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 소스 소유자 태그(SourceOwnerTags) 변경이 재평가를 트리거하지 못한다
- **위치**: `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp:259-278` (읽기 지점) · `Plugins/WxSound/Source/WxSound/Private/WxBGMSourceComponent.cpp:41` (구독 지점)
- **범주**: 버그/정확성 — 미처리 이벤트 경로
- **문제**: `EvaluateBGM` 은 활성 소스 소유자 액터의 ASC owned-tag(`SourceOwnerTags`)를 매 평가 시점에 읽어 Chooser 컬럼으로 노출한다. 그러나 재평가를 유발하는 이벤트는 (a) 로컬 플레이어 ASC 의 owned-tag 변경, (b) `StartBGM`/`StopBGM`, (c) 소스 등록/해제뿐이다. 소스 소유자(보스 등) 자신의 ASC 태그가 바뀌어도(예: `Enemy.Boss.Phase2` 부여) 서브시스템은 그 이벤트를 구독하지 않아 즉시 재평가되지 않는다. 따라서 소유자 태그로 페이즈별 BGM 을 분기하려 하면 다음 로컬 플레이어 태그 변경 등 무관한 트리거가 우연히 발생할 때까지 곡이 갱신되지 않는다. `UWxBGMSourceComponent` 는 `ActivationTag` 존재 여부만 감시하고 소유자의 다른 태그 변화는 서브시스템에 전달하지 않는다.
- **제안**: 소유자 태그가 정적 식별자(`Enemy.Boss.Dragon`)로만 쓰인다면 현 설계로 충분하니 의도임을 README/주석에 못박는다. 페이즈 전환 같은 동적 반영이 필요하면 `RegisterBGMSource` 시점에 소유자 ASC 의 generic tag 이벤트도 구독하고 해제 시 정리하거나, 소스 컴포넌트가 소유자 태그 변화를 감지해 재등록하도록 배선한다.
- **확신도**: 낮음 (정적 식별자 태그만 의도한 설계일 수 있음. 실전에서 전투 중 플레이어 태그가 빈번히 바뀌어 우발적으로 재평가가 자주 일어나므로 체감 영향은 작을 수 있다.)

### 2. 🟡 멀티 로컬 플레이어(스플릿스크린)에서 단일 BoundController/BoundASC 가 마지막 플레이어로 덮어써진다
- **위치**: `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp:46-50, 185-204`
- **범주**: 설계/구조 — 상태 관리 가정
- **문제**: `OnWorldBeginPlay` 는 GameInstance 의 모든 로컬 플레이어와 이후 추가되는 플레이어 각각에 `HandlePlayerControllerChanged` 를 건다. 그런데 서브시스템은 단일 `BoundController`/`BoundASC` 만 추적하므로, 로컬 플레이어가 둘 이상이면 나중에 델리게이트가 발화한 플레이어가 앞선 플레이어의 바인딩을 덮어쓴다(이전 컨트롤러의 `OnPossessedPawnChanged` 해제 포함). 결과적으로 "현재 플레이어"가 플레이어 간에 뒤바뀌어 어느 플레이어의 상태가 BGM 을 몰지 비결정적이 된다.
- **제안**: 단일 로컬 플레이어(대상 게임의 전제)라면 첫 로컬 플레이어(인덱스 0)만 바인딩하도록 좁히거나, `GetGameInstance()->GetFirstLocalPlayerController()` 기준으로 명시하여 다중 로컬 플레이어에서의 flip-flop 을 제거한다. 스플릿스크린을 지원할 계획이 없다면 그 전제를 코드/README 에 명시한다.
- **확신도**: 낮음 (오픈월드 싱글플레이어 전제라면 실질 무해. 다만 모든 로컬 플레이어에 바인딩하는 현재 코드는 전제와 어긋나 오해를 부른다.)

### 3. 🟢 owned-tag generic 이벤트가 무관한 태그 변화(쿨다운·GE 스택 등)마다 전체 Chooser 평가를 유발한다
- **위치**: `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp:212-215, 232`
- **범주**: 성능/안전
- **문제**: `RegisterGenericGameplayTagEvent` 는 ASC 의 **모든** owned-tag 카운트 변화에 발화한다. 전투 중에는 쿨다운·버프·GE 스택 태그가 초당 여러 번 바뀔 수 있어, 그때마다 `EvaluateBGM`(플레이어 태그 스냅샷 + 활성 소스별 owner ASC 조회 + 소스당 임시 `FGameplayTagContainer` 할당)이 실행된다. `ApplyBGM` 이 결과 동일 시 no-op 이라 청각 영향은 없지만 평가 자체는 매번 돈다.
- **제안**: 실측 후 문제가 되면 BGM 관련 태그 접두(예: `State.`·`BGM.`)로 특정 태그 이벤트만 구독하거나, 재평가를 다음 틱으로 코얼레싱(중복 제거)한다. 현 규모에선 저비용이므로 실측 전 선최적화는 지양.
- **확신도**: 낮음 (이벤트 구동·저비용. 실제 프로파일 없이는 개선 불요일 수 있음.)

### 4. 🟢 StopBGM 보류 중에는 소스 등록/해제가 재평가되지 않아 보스 진입 BGM 이 무시된다
- **위치**: `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp:103-107, 160-168`
- **범주**: 버그/정확성 — 상태 상호작용
- **문제**: `StopBGM` 이 `bSuspended=true` 로 두면 이후 `RegisterBGMSource`/`UnregisterBGMSource` 가 호출하는 `Reevaluate` 가 조기 반환한다. 즉 BGM 보류 상태에서 보스전이 시작(소스 등록)되어도 다음 명시 `StartBGM` 전까지 곡이 나오지 않는다.
- **제안**: 이것이 "보류는 명시 StartBGM 까지 완전 침묵"이라는 의도면 그대로 두되 주석/README 에 명시한다. 상태 기반 소스는 보류를 관통해야 한다면 소스 등록 시 보류를 해제하거나 소스 경로를 `bSuspended` 검사에서 제외한다.
- **확신도**: 낮음 (의도된 보류 시맨틱일 가능성이 높음.)

### 5. 🟢 PlayerStateTags.Reset() 이 중복이다
- **위치**: `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp:245-249`
- **범주**: 중복/복잡도
- **문제**: `ChooserContext.PlayerStateTags.Reset()` 직후 `GetOwnedGameplayTags(ChooserContext.PlayerStateTags)` 를 호출하는데, `GetOwnedGameplayTags` 는 인자 컨테이너를 내부에서 Reset 후 채운다(273-274행 주석이 스스로 이 동작을 인지). 앞선 Reset 은 불필요하다. 단, ASC 가 없을 때(early skip) 이전 스냅샷을 비우는 역할은 하므로 완전 제거보다는 `else` 로 옮기는 편이 명확하다.
- **제안**: ASC 유효 시엔 Reset 을 생략하고, ASC 부재 시에만 컨테이너를 비우도록 정리한다(사소).
- **확신도**: 높음 (동작 영향 없는 미세 중복.)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp`, `Plugins/WxSound/Source/WxSound/Public/System/WxMusicSubsystem.h`, `Plugins/WxSound/Source/WxSound/Private/WxBGMSourceComponent.cpp`, `Plugins/WxSound/Source/WxSound/Public/WxBGMSourceComponent.h`
- **훑은 파일**: `Plugins/WxSound/Source/WxSound/Private/WxMusicLibrary.cpp`, `Plugins/WxSound/Source/WxSound/Public/WxMusicLibrary.h`, `Plugins/WxSound/Source/WxSound/Public/WxBGMChooserContext.h`, `Plugins/WxSound/Source/WxSound/Public/WxBGMData.h`, `Plugins/WxSound/Source/WxSound/Public/System/WxMusicSettings.h`, `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSettings.cpp`, `Plugins/WxSound/Source/WxSound/Public/WxSoundModule.h`, `Plugins/WxSound/Source/WxSound/Private/WxSoundModule.cpp`, `Plugins/WxSound/Source/WxSound/WxSound.Build.cs`
- **미검토 / 한계**: Chooser 테이블(`.uasset`)·`UWxBGMData` 에셋의 실제 Row 구성과 페이드 값은 데이터 자산이라 코드 리뷰 범위 밖. `SpawnSound2D`/`FadeOut` 의 bAutoDestroy 상호작용은 코드상 UPROPERTY 보유로 GC 방지가 성립함을 확인했으나 런타임 오디오 동작은 실행 검증하지 않음. 모듈 경계(WxCore 외 Wx 참조 금지), Wx/Handle prefix, Super:: 호출, BlueprintCallable 사용처, Copyright 첫 줄은 전부 위반 없음.

---
*문서 기준 커밋 `9661edf` · 리뷰일 2026-07-21 · 소스 12파일 — `/module-review`로 갱신*

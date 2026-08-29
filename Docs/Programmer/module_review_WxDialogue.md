# WxDialogue — 코드 리뷰

> 11파일 규모의 작고 응집도 높은 모듈이다. 수명·소유권 근거와 데이터 오류 경고가 잘 갖춰져 있고 프로젝트 코딩·모듈 규칙 위반은 확인되지 않았다. 이번 리뷰는 README, Build.cs, 모든 Public/Private `.h`·`.cpp`와 세션의 소비 지점(`WxGame` 뷰모델·상호작용 어빌리티)을 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 세션 교체가 앞선 `Play Dialogue` 태스크를 정상 완료로 오인시킨다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:124`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:214`
- **범주**: 버그/정확성
- **문제**: 새 세션을 열 때 활성 세션을 `EndDialogue()`로 닫고, 이 함수는 종료 사유와 관계없이 `OnDialogueEnded`를 broadcast한다. 앞 세션을 연 `FWxStateTreeTask_PlayDialogue`는 이 신호를 받으면 `Succeeded`로 끝나므로, 다른 대화가 끼어든 경우에도 앞 퀘스트 단계가 대사를 끝까지 읽지 않은 채 다음 단계로 진행한다.
- **제안**: 세션 교체를 중단으로 구분한다. 교체 시 종료 델리게이트를 발행하지 않거나, 정상 완료와 중단 신호를 나눠 StateTree 태스크가 중단을 `Failed` 또는 명시적 취소로 처리하게 한다.
- **확신도**: 중간

### 2. 🟡 대화 세션의 권위 모델이 리슨 호스트 전제를 벗어나면 즉시 실패하거나 게이트가 무력화된다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp:40`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp:43`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:149`
- **범주**: 설계/구조
- **문제**: StateTree 태스크는 `ClientStartDialogue` RPC를 보낸 직후 서버 측 `HasActiveDialogue()`를 검사한다. 데디케이티드 서버에서는 클라이언트의 로컬 상태가 아직 서버 컴포넌트에 없으므로 이 검사가 `Failed`가 된다. 또한 `State.Dialogue`는 클라이언트 ASC에만 loose tag로 설정되어 서버 전용 상호작용 어빌리티의 차단 태그로는 기능하지 않는다. README에 v1 싱글/리슨 호스트 전제가 있으나, 전제가 깨지면 조용한 실패 또는 재상호작용으로 나타난다.
- **제안**: 현 전제를 유지한다면 전제 불일치 시 경고를 남긴다. 멀티플레이 확장 시에는 서버가 세션 상태와 상호작용 차단 태그를 소유하고, 태스크 완료는 서버 확인 뒤 처리하게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — README에 v1 전제가 명시돼 있다)

### 3. 🟢 세션 주입 누락과 잘못된 Interactor가 상호작용에서 조용히 실패한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp:21`
- **범주**: 버그/정확성
- **문제**: `StartDialogueWith`는 Interactor가 Pawn이 아니거나 Controller에 `UWxDialogueSessionComponent`가 없으면 로그 없이 반환한다. 시작 행·데이터 오류는 모두 경고하는 모듈이라 Experience의 세션 컴포넌트 주입이 빠진 조립 오류만 "상호작용해도 아무 일 없음"으로 남는다.
- **제안**: Interactor와 세션을 얻지 못한 경우 대상·Interactor 이름을 포함한 Warning 로그를 남긴다.
- **확신도**: 높음

### 4. 🟢 공개 헤더에 필요 없는 모듈 의존성이 전파된다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs:16`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs:20`
- **범주**: 중복/복잡도
- **문제**: `GameplayAbilities`와 `UniversalObjectLocator`는 Public 헤더에서 쓰이지 않는다. 특히 `UniversalObjectLocator`는 WxDialogue 소스 전체에서 참조가 없어 링크·빌드 그래프에만 남아 있고, `GameplayAbilities`도 cpp 구현에만 필요해 WxDialogue를 참조하는 모든 모듈에 불필요하게 Public 의존성으로 전파된다.
- **제안**: `UniversalObjectLocator` 의존성을 제거하고 `GameplayAbilities`를 `PrivateDependencyModuleNames`로 옮긴다. 함께 `GameplayTags`도 Public 헤더 노출 여부를 재확인해 Private으로 내린다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp`
- **훑은 파일**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/WxDialogue.uplugin`
- **미검토 / 한계**: DataTable 에셋과 BP/WBP 이벤트 그래프는 범위 밖이다. 원격 Client RPC의 DataTable NetGUID 해소와 데디케이티드 서버 동작은 실행 검증하지 않아, 발견 2는 문서화된 v1 전제를 고려한 낮은 확신도로 기록한다.

---
*문서 기준 커밋 `b48c1930` · 리뷰일 2026-08-29 · 소스 11파일 — `/module-review`로 갱신*

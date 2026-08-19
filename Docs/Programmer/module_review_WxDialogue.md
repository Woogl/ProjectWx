# WxDialogue — 코드 리뷰

> 9파일 825줄의 작고 밀도 높은 모듈로, 실패 경로마다 로그를 남기고 수명·GC·네트워크 전제를 헤더 주석에 명시해 둔 편이라 전반적으로 건강하다. 이번 리뷰는 `Build.cs`·`.uplugin`·공개 헤더 전부와 세션/대화 컴포넌트/ST 태스크 cpp 전량을 읽었고, 소비자 쪽(`AWxNpc`·`UWxViewModel_Dialogue`·`IWxInteractable`)과 엔진 `FinishTask` 구현까지 대조해 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 포즈를 얹을 스켈레탈 메시를 `FindComponentByClass` 로 고른다 — 다중 메시 대상에서 비결정적
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:335`
- **범주**: 버그/정확성
- **문제**: `Target->FindComponentByClass<USkeletalMeshComponent>()` 는 액터의 `OwnedComponents`(TSet)를 훑어 **처음 걸린 것**을 답하므로 순서가 정해져 있지 않다. 대상이 메타휴먼 구성이면 후보가 여럿이다 — `UWxMetaHumanComponent` 가 등록 시점에 Body·Face·Outfit 스켈레탈 메시를 오너에 추가로 생성해 붙인다(`Source/WxGame/Character/WxMetaHumanComponent.cpp:170`, 멤버는 `WxMetaHumanComponent.h:91-100`). 포즈는 실제로 포즈를 만드는 리더 메시에 얹혀야 하는데, Body 가 뽑히면 그쪽은 `SetLeaderPoseComponent` 로 구동되는 표시 전용이라 애님 인스턴스가 없어 `PlayPendingPose` 의 경고만 남고 포즈가 사라지고, Face 가 뽑히면 `FaceAnimClass` 가 있어 얼굴 리그에 몸 포즈 몽타주가 걸린다.
  현재 배치 NPC(`AWxNpc`)는 `AActor` 파생이고 `UWxMetaHumanComponent::OnRegister` 가 오너를 `ACharacter` 로 캐스팅해 실패하면 조기 반환하므로(`Source/WxGame/Character/WxMetaHumanComponent.cpp:37`) 아직 후보가 하나뿐이지만, 그 조합은 `AWxNpc` 가 메타휴먼 부착물을 갖게 되는 순간(헤더가 명시한 의도, `Source/WxGame/Character/WxNpc.h:41-43`) 바로 성립한다. `StartDialogueRow(Row, Target)` 의 `Target` 이 임의 액터라는 점도 같은 위험을 넓힌다.
- **제안**: 포즈 대상 메시를 대상 쪽이 직접 지목하게 한다 — `UWxDialogueComponent` 가 `AreaMesh` 처럼 포즈 메시 프로퍼티를 들고 세션이 그것을 묻거나, 최소한 `ACharacter::GetMesh()` 를 먼저 시도한 뒤 `FindComponentByClass` 로 떨어지게 한다.
- **확신도**: 중간

### 2. 🟡 ASC 를 못 찾으면 세션이 조용히 열리고 다시는 닫히지 않는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:142-150`
- **범주**: 버그/정확성
- **문제**: 폰이나 ASC 가 없으면 `if (UAbilitySystemComponent* ASC = ...)` 가 통째로 건너뛰어지는데 else 분기도 로그도 없다. 이 파일의 다른 모든 실패 경로가 경고를 남기는 것과 대조적이라 진단 단서가 0 이다.
  그런데 이 태그가 대화 창을 여는 유일한 신호이고(헤더 31행의 관찰 규약), 대사를 넘기는 `Advance()` 를 부르는 곳은 그 창의 뷰모델뿐이다(`Source/WxGame/MVVM/WxViewModel_Dialogue.cpp:48`). 즉 태그 발행이 빠지면 **창이 안 뜨고 → `Advance()` 를 부를 주체가 없고 → 세션이 스스로 끝날 길이 없다.** 외부에서 강제 종료할 진입점도 없어(`EndDialogue` 는 private, 다음 `ClientStartDialogue` 만이 접는다) 그대로 굳는다. `Play Dialogue` 태스크가 연 대화였다면 `OnDialogueEnded` 가 영영 안 와서 퀘스트 스텝이 Running 으로 멈춘다.
  폰 없는 창이 실재하는 전제도 있다 — Experience 지연 스폰으로 폰이 컨트롤러보다 늦게 서므로, 레벨 초입에 퀘스트 ST 가 여는 나레이션 대사가 정확히 그 창에 걸린다.
- **제안**: else 에 경고를 남기고, 태그 발행 실패를 시작 실패로 취급해 `ClientStartDialogue_Implementation` 의 롤백(128-135행과 같은 모양)을 타게 한다. 그러면 ST 태스크의 `HasActiveDialogue()` 검사가 Failed 로 잡아내 조립 오류가 드러난다.
- **확신도**: 중간

### 3. 🟡 `GetCurrentRowHandle()` · `GetCurrentDialogueTarget()` 은 호출자가 없다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:63`, `:69` (정의는 `Private/WxDialogueSessionComponent.cpp:93`, `:98`)
- **범주**: 중복/복잡도
- **문제**: 저장소 전체에서 이 둘을 부르는 코드가 없다. `UFUNCTION` 도 아니라 BP·StateTree 바인딩에서도 닿지 않으므로 완전히 도달 불가다. 원래 소비자는 있었으나 둘 다 사라졌다 — 대화 카메라가 세션 내부로 들어오면서 `GetCurrentDialogueTarget` 은 `CurrentTarget` 직접 참조로 대체됐고(`BeginDialogueCamera`, cpp:229), 행 신원을 비교하던 `Wait Dialogue Completed` 태스크는 `OnDialogueEnded` 일회성 신호로 대체돼 없어졌다(WxQuest 전체에 Dialogue 참조 0건). 남은 것은 잔재이며, 헤더·README 의 "관찰 규약" 서술이 실재하지 않는 계약을 광고하고 있어 다음 세션을 오도한다.
- **제안**: 둘을 지우고 헤더·README 의 관찰 규약 서술도 `OnLineChanged`/`OnDialogueEnded`/`State.Dialogue` 세 가지로 줄인다. 행 신원 관찰이 다시 필요해지면 그때 되살린다.
- **확신도**: 높음

### 4. 🟢 `PlayerCameraManager` 만 널 검사 없이 역참조한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:237`
- **범주**: 성능/안전
- **문제**: `BeginDialogueCamera` 는 Pawn·CurrentTarget·스폰된 CameraActor 를 모두 가드하는데 `PlayerController->PlayerCameraManager` 만 그대로 역참조한다. 로컬 PC 라면 `PostInitializeComponents` 에서 이미 스폰돼 있어 실사용에선 널이 아닐 것이므로 지금 터지는 문제는 아니고, 이 함수 안의 일관성 차이로만 눈에 띈다.
- **제안**: 널이면 다른 조기 반환들과 같이 카메라 경로를 건너뛴다. 실익이 낮다고 보면 그대로 두어도 무방하다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`
- **훑은 파일**: `Plugins/WxDialogue/README.md`, `Plugins/WxDialogue/WxDialogue.uplugin`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp`
- **대조한 모듈 밖 근거**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Character/WxMetaHumanComponent.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, 엔진 `StateTreeAsyncExecutionContext.cpp`
- **미검토 / 한계**:
  - 대화 창을 여닫는 `State.Dialogue` 관찰자가 C++ 에 없다(BP/WBP 쪽 구현으로 보임). 발견 2 의 "창이 안 뜬다 → 세션이 굳는다" 사슬은 그 BP 동작을 실행으로 확인하지 못한 채 C++ 근거만으로 세운 것이다.
  - 규칙 검사(`Wx` prefix, 저작권 첫 줄, `Handle` prefix, `BlueprintCallable` 오용, WxCore 외 플러그인 참조)는 전 파일 통과. 헤더의 `GetInstanceDataType()` 인라인 정의와 ST 태스크의 람다는 각각 프로젝트 전역 관례(다른 8개 모듈 26곳 동일)와 엔진 약한 실행 컨텍스트 규약이라 위반으로 보지 않았다.
  - 리플리케이션은 v1 싱글/리슨 호스트 전제(루즈 태그 비복제, 소유 클라 진행)가 헤더 3곳에 명시돼 있어 그 전제 안에서만 검증했다. 데디케이티드 서버 구성은 리뷰하지 않았다.
  - `.uasset`(대화 DataTable, WBP, ST 에셋) 내부 구조는 범위 밖.

---
*문서 기준 커밋 `b3aec4ef` · 리뷰일 2026-08-20 · 소스 9파일 — `/module-review`로 갱신*

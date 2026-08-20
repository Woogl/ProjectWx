# WxQuest — 퀘스트 시스템

> StateTree 에셋 1개를 퀘스트 1개로 실행하고, 그 진행을 저널(제목·목표)로 서버 권위에서 관리한다. 수주·목표·체인·도달판정을 데이터(에셋)가 지정하고 코드는 에셋 불가지로 실행만 위임한다.

## 책임
**담당**
- GameState 에 붙어 퀘스트 StateTree 러너를 권위에서만 생성·실행 (동시 활성 퀘스트 1개, 새 시작은 교체)
- 저널 상태 보관 및 변경 통지 (`OnJournalChanged` 브로드캐스트 → 뷰모델이 pull)
- 데이터 주도 진행 태스크 제공: 제목 설정, 목표 추가/제거, 다음 퀘스트 체인, 목표 지점 도달 대기
- 러너 실행 콜스택 재진입 회피 (다음 틱 예약) 및 종료 3경로(완료·실패·교체)의 저널 정리 수렴

**경계 (비담당)**
- 저널의 화면 표시 — [[WxUI]] HUD 뷰모델이 구독해 렌더
- 컴포넌트 부착 — GameMode 가 고른 Experience 에셋의 주입 목록이 담당 (코드 부착 아님)
- 보상 지급·스폰 등 월드 부수효과 — 퀘스트 StateTree 안의 다른(크로스모듈) 태스크가 담당

## 의존성
- **주요 의존**: `WxCore`. 엔진: StateTree / GameplayStateTree, ModularGameplay(`UGameStateComponent`), UniversalObjectLocator(액터 지정)
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxQuestComponent` | 러너 소유·저널 상태·변경 통지의 중심. 모든 태스크가 오너에서 이걸 찾아 위임 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` |
| `UWxQuestLibrary` | 레벨 트리거 볼륨 등 외부 진입점. GameState 컴포넌트를 찾아 `StartQuest` 위임 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h` |
| `UWxQuestStateTree` | 퀘스트 1개 = 이 에셋 1개. `UStateTree` 서브클래스(표식 역할) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTree.h` |
| `FWxStateTreeTask_SetQuestTitle` | 상태 진입 시 저널 제목 등록(목표 비움) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h` |
| `FWxStateTreeTask_SetQuestObjective` | 상태 수명 = 목표 수명. 진입 시 걸고 이탈 시 핸들로 걷어감 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h` |
| `FWxStateTreeTask_WaitMoveToTarget` | 플레이어 폰이 대상 반경 진입까지 Running. 상태 완료를 내는 대기 태스크 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h` |
| `FWxStateTreeTask_StartNextQuest` | 다음 퀘스트를 다음 틱 예약(체인). 빈 지정은 종점 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h` |

## 확장 포인트 / 규약
- **새 진행 태스크**: `FStateTreeTaskCommonBase` 를 상속한 `FWxStateTreeTask_*` USTRUCT 을 추가한다. 오너(GameState)에서 `UWxQuestComponent` 를 `FindComponentByClass` 로 찾아 위임하고, 없으면 잘못된 조립이므로 `Failed` 를 낸다. `GetInstanceDataType()` 헤더 정의는 코딩 규칙 6 의 명시적 예외(엔진 StateTree 관례).
- **완료 판정 규약**: 즉시 끝나는 태스크(제목·목표·체인)는 코드에서 완료 판정에 빠져 있어(`bConsideredForCompletion=false`) 진입 즉시 `Succeeded` 를 내도 상태를 끝내지 않는다. 상태의 실제 완료는 짝이 되는 대기 태스크(`WaitMoveToTarget` 등)가 낸다 — 스스로 끝나야 하는 상태에는 대기 태스크를 반드시 하나 둔다.
- **데이터 주도 설정**: 퀘스트 = `UWxQuestStateTree` 에셋. 수주는 레벨 배치 트리거가 `UWxQuestLibrary::StartQuest` 호출, 체인은 `StartNextQuest` 태스크의 `Quest` 소프트 참조, 도달 대상은 `WaitMoveToTarget` 의 `FUniversalObjectLocator`(배치 액터 직접 지정). 코드는 어떤 에셋도 알지 않는다.
- **권위 규약**: 러너는 권위(BeginPlay `HasAuthority`)에서만 생성 → 비-권위 호출은 자연 노옵. 러너 콜스택 안에서의 활성화는 `RequestActivateQuest`(다음 틱), 밖에서는 `ActivateQuest`(정지→교체→시작).

## 여기서부터 읽어라
1. `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` — 클래스 헤더 주석이 모듈 전체 설계(에셋 불가지·권위·저널 수명·부착 경로)를 담은 지도다.
2. `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp` — 정지→교체→시작 순서와 종료 3경로 저널 정리(`HandleStateTreeRunStatusChanged`)의 실제 구현.
3. `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp` — 태스크가 컴포넌트를 찾아 위임하는 표준 패턴(진입/이탈 핸들 걷기).

## 관련
- 상위: [[WxUI]] (저널 표시), GameFeature/Experience 에셋 (컴포넌트 주입·수주 트리거 배치). 퀘스트 StateTree 안에서 보상·스폰을 내는 크로스모듈 태스크는 각 도메인 모듈 소관.

---
*문서 기준 커밋 `6f60b14` · 생성일 2026-08-14 · 소스 15파일 — `/readme-writer`로 갱신*

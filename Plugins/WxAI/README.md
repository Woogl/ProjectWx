# WxAI — AI 시스템

> 적 폰의 감지·정찰·행동 결정을 담당한다. 언리얼 AIPerception 과 BehaviorTree 를 확장한 컴포넌트·BT 노드 모음으로, 감지 결과를 Blackboard 에 실어 트리 흐름을 굴린다.

## 책임
**담당**
- 시각·청각·피격 감지를 Blackboard `TargetActor` 로 동기화하고, 사망·소실·리시 복귀에서만 타겟을 해제 (`UWxAIPerceptionComponent`)
- 정찰 경로 데이터(스플라인)와 순회 규칙 제공, 진행 커서는 BT 태스크가 폰별로 소유 (`UWxPatrolComponent` + `UWxBTTask_Patrol`)
- 커스텀 BT 노드 — 무작위 선택 Composite, 리시 이탈 Decorator, 어트리뷰트 비율 Decorator, 어빌리티 발동/배회/복귀 Task
- Blackboard 키 이름·타입을 한 곳에 묶는 accessor 계약 (`WxBlackboardKeys`)

**경계 (비담당)**
- AIController·BehaviorTree 에셋·Blackboard 에셋 저작 자체 — 모듈 밖. `SelfActor`·`HomeLocation` 키를 채우는 것도 AIController 몫이다
- 전투 어트리뷰트·GameplayEffect 정의 → [[WxCombat]] (WxAI 는 WxCombat 에 코드 의존하지 않으며, `FGameplayAttribute`·`TSubclassOf<UGameplayEffect>` 는 디자이너가 BT 에디터에서 직접 지정)
- 어빌리티 실행 로직 자체는 GAS(GameplayAbilities). WxAI 는 태그로 발동을 걸고 종료만 관측

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAIPerceptionComponent` | 감지 → `TargetActor` 동기화 허브. 대부분의 트리 흐름이 여기서 나오는 타겟에 의존 | `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` |
| `WxBlackboardKeys` | 키 이름·타입·accessor 를 묶은 계약(namespace). 컴포넌트와 BT 노드가 공유 | `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxPatrolComponent` | 상태 없는 정찰 경로 스플라인. 적이 부착된 액터(스포너 등)에 붙는다 | `Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h` |
| `UWxBTComposite_RandomChoice` | 조건 통과 자식 중 가중치 기반 무작위 1개 실행(폴백 없음) | `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxBTTask_ActivateAbility` | 태그로 GAS 어빌리티를 발동하고 종료 결과를 BT 결과로 변환 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxBTDecorator_BeyondLeash` | 앵커에서 리시 반경 이탈 판정(폴링 재평가). 복귀 브랜치 게이팅 | `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` |
| `UWxBTTask_ReturnHome` | 리시 복귀 실행 노드. 이동 시작 시 타겟을 잊는다 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ReturnHome.h` |
| `UWxAnimNotify_ReportNoise` | 애님 프레임에서 청각 소음 이벤트 발생(서버 전용) | `Plugins/WxAI/Source/WxAI/Public/WxAnimNotify_ReportNoise.h` |

## 확장 포인트 / 규약
- 새 BT 노드: 엔진 베이스(`UBTTaskNode`/`UBTService`/`UBTDecorator`/`UBTCompositeNode`)를 상속하고 Blackboard 접근은 반드시 `WxBlackboardKeys` accessor 를 통한다 — `GetValueAs`/`SetValueAs` 직접 호출 금지(타입 오용·키 부재 경고를 accessor 가 대신 처리).
- 새 Blackboard 키 추가: `WxBlackboardKeys` 에 `extern const FName` + accessor 를 선언·정의하고, Blackboard 에셋에 같은 이름 키를 등록해야 한다. Object 키는 null=미설정, Float 키는 `NoTargetDistance`(=무한대) 관용을 따른다.
- 감속·어트리뷰트 게이팅: WxAI 는 WxCombat 에 의존하지 않으므로 `MoveSpeedEffect`(GE 클래스), `Attribute`/`MaxAttribute` 는 디자이너가 BT 에디터에서 직접 지정한다. 미지정 시 감속 없이 동작.
- 무작위 추첨: `UWxBTDecorator_RandomWeight` 를 자식에 붙여 가중치 부여(없으면 1.0, 0 이면 제외). 조건 Decorator 는 후보 필터로 함께 작동한다.
- 리플리케이션: 감지·소음 보고는 서버 권한에서 돈다(`ReportNoiseEvent` 서버 전용). 최대 4인 협동 기준.

## 여기서부터 읽어라
1. `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` — 모듈 전체가 공유하는 데이터 계약. 어떤 키를 누가 채우고 읽는지 여기 주석에 정리돼 있다.
2. `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` — 타겟 획득/해제의 단일 출처. 트리 흐름의 시작점.
3. `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` + `WxBTDecorator_BeyondLeash.h` — 커스텀 BT 노드가 엔진 메모리·재평가 규약과 어떻게 맞물리는지 보여주는 대표 예.

## 관련
- 상위: AIController·BT/Blackboard 에셋(SelfActor·HomeLocation 세팅, 트리 실행)은 [[WxGame]] 쪽. 어트리뷰트·GameplayEffect 정의는 [[WxCombat]](코드 의존 없이 에디터에서 참조).

---
*문서 기준 커밋 `bb06a17` · 생성일 2026-08-30 · 소스 29파일 — `/readme-writer`로 갱신*

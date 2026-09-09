# WxAI — AI 행동 시스템

> 적 폰의 감지·표적 선정·행동을 담당한다. 엔진 Behavior Tree / AIPerception 위에 얹는 커스텀 BT 노드와 퍼셉션·정찰 컴포넌트, 그리고 이들이 공유하는 Blackboard 키 계약을 제공한다.

## 책임
**담당**
- 감지·인식: 시각·청각·피격(Damage) 센스를 묶은 퍼셉션과, 폰별 감각 수치를 컨트롤러에 실어 주는 경로
- 표적 흐름: 감지 결과에서 TargetActor 선정, 거리 갱신, 락온(포커스+strafe 회전) 반영
- 행동 실행: 어빌리티 발동/미러링, 배회, 정찰, 리시(leash) 복귀 등 커스텀 BT Task/Service/Decorator/Composite
- Blackboard 키 계약: 키 이름·값 타입을 accessor 로 묶어 타입 오용을 차단

**경계 (비담당)**
- AIController·캐릭터 본체: `AWxAIController`(Source/WxGame)가 폰을 빙의하고 이 모듈의 컴포넌트/BT를 구동한다
- 어빌리티·어트리뷰트 정의: 실제 GameplayAbility·AttributeSet은 [[WxCombat]]에 있다. WxAI는 태그(`Ability.*`)와 디자이너가 지정한 Attribute만 참조하고 WxCombat에 의존하지 않는다
- 락온 대상 선정: 누구를 겨눌지는 [[WxCombat]]의 `UWxLockOnComponent`가 정하고, 이 모듈의 `UWxBTService_LockOn`은 "어떻게 바라볼지"만 맡는다

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxBlackboardKeys` | 모듈 전체가 공유하는 Blackboard 키·accessor 계약. 노드 간 데이터 흐름의 허브 | `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxAIPerceptionComponent` | 컨트롤러에 붙는 감지 진입점(시각·청각·피격). TargetActor 후보를 만든다 | `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` |
| `UWxAIBehaviorComponent` | 폰에 붙어 BT 자산·감각 수치를 종류별로 공급(퍼셉션이 빙의 시 읽어 감) | `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h` |
| `UWxBTService_UpdateTargetActor` | 감지 결과 → Blackboard TargetActor 발행. 표적 흐름의 시작점(루트에 상주) | `Plugins/WxAI/Source/WxAI/Public/WxBTService_UpdateTargetActor.h` |
| `UWxBTService_LockOn` | TargetActor를 컨트롤러 포커스 + 폰 strafe 회전에 반영 | `Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h` |
| `UWxBTTask_ActivateAbility` | 태그로 어빌리티를 발동하고 종료까지 latent 대기 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxBTComposite_RandomChoice` | 조건·가중치로 자식 하나를 추첨하는 Composite(Selector와 다른 시멘틱) | `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxPatrolComponent` | 스플라인 기반 정찰 경로 데이터(무상태). 커서는 BT Task가 폰별로 소유 | `Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h` |

## 확장 포인트 / 규약
- 새 행동 노드는 엔진 베이스(`UBTTaskNode`/`UBTService`/`UBTDecorator`/`UBTCompositeNode` 또는 `UBTTask_MoveTo` 파생)를 상속해 추가한다. 노드 간 데이터는 반드시 `WxBlackboardKeys`의 accessor로 주고받고(직접 `GetValueAs`/`SetValueAs` 금지), 사용하는 키는 Blackboard 에셋에 같은 이름으로 등록돼 있어야 한다.
- 데이터 주도 구동: 폰의 `UWxAIBehaviorComponent`에 BT 자산·시야/청각 수치를 지정해 종류별 차이를 낸다. 정찰은 폰이 부착된 액터(스포너 등)의 `UWxPatrolComponent`를 따르며, 경로가 없으면 정찰하지 않는다.
- 어빌리티 연동은 태그 계약으로만 이뤄진다 — 어빌리티는 활성 구간에 식별 태그 `Ability.X`를 소유 태그로 발행하고, `ActivateAbility`/`MirrorAbility`가 그 태그로 시작·종료를 추적한다. `AttributeRatio` 데코의 Attribute/MaxAttribute는 디자이너가 BT 에디터에서 직접 지정한다.
- 권한 모델: 소음 발생(`UWxAnimNotify_ReportNoise`)은 서버 전용이다.

## 여기서부터 읽어라
1. `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` — 노드들이 공유하는 데이터 계약. 모듈 전체 데이터 흐름의 지도다
2. `Plugins/WxAI/Source/WxAI/Public/WxBTService_UpdateTargetActor.h` — 감지→표적 발행. 표적 흐름이 여기서 시작한다
3. `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` — 세 센스가 어떻게 후보를 만들고 컨트롤러/폰이 어떻게 얽히는지
4. `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` — 추첨 Composite의 후보 필터·통지 규칙(가장 비자명한 노드)

## 관련
- 상위: `AWxAIController`(Source/WxGame)가 이 모듈을 구동한다. 어빌리티·어트리뷰트·락온 대상은 [[WxCombat]], 공용 정의는 [[WxCore]]

---
*문서 기준 커밋 `e0e3ecc` · 생성일 2026-09-09 · 소스 38파일 — `/readme-writer`로 갱신*

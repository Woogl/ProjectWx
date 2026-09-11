# WxAI — 코드 리뷰

> 변경된 어그로 필터와 에디터 시야 표시에서 확정적인 신규 결함은 발견하지 않았다. README를 시작점으로 현재 작업 트리의 변경 파일, 감지·타겟 선정·락온, 두 어빌리티 태스크와 RandomChoice의 핵심 로직을 검토했으며 기존 발견은 현재 코드와 다시 대조했다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 어빌리티 발동·중단·종료 프로토콜이 두 태스크에 중복된다

- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:42`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:102`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:73`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:141`
- **범주**: 중복/복잡도
- **문제**: 종료 델리게이트 선등록, 목록 잠금, 발동 핸들 선기록, 동기 종료와 재발동 판별, 취소 요청 중 재진입 방지, 취소 불가 인스턴스 처리, 종료 시 구독 해제까지 같은 프로토콜이 두 파일에 반복된다. 태그의 출처와 MirrorAbility의 대상 추적은 다르지만 수명주기 수정은 양쪽에 동기화해야 하므로 한쪽만 수정될 유지보수 위험이 있다. 현재 양쪽의 취소 거부 가드는 존재하므로 이를 실행 중 결함으로 분류하지 않는다.
- **제안**: 독립된 두 태스크는 유지하되 공통 발동·종료 프로토콜만 공유하는 방안을 검토한다. 독립 구현을 유지한다면 두 파일이 함께 검토되어야 한다는 유지보수 규약을 명시한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 2. 🟢 RandomChoice의 가중치 전원 0 설정을 진단하지 않는다

- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:98`, `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h:26`, `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_RandomWeight.h:31`
- **범주**: 설계/구조
- **문제**: 모든 자식의 조건이 통과하면서 가중치가 모두 0이면 후보도 차단 자식도 없어 바로 부모로 반환한다. 헤더는 이 경우 실패 결과를 만들지 못하는 한계를 명시하지만, 가중치 저작 속성은 0을 정상적인 제외 값으로 허용하며 해당 조합에 대한 검증이나 경고는 없다. 여러 공격을 잠시 제외하는 저작에서 이 한계에 도달했는지 개발자가 알아보기 어렵다.
- **제안**: 가중치 전원 0인 조합을 에셋 검증에서 알리거나, 해당 경로에서 반복 출력을 제한한 진단을 제공한다. 경고를 위해 런타임 선택 의미를 변경할 필요는 없다.
- **확신도**: 중간

## 검토 범위

- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`.
- **훑은 파일**: `Plugins/WxAI/README.md`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h`, `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h`, `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_RandomWeight.h`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, 인터페이스 계약 확인용 `Plugins/WxCore/Source/WxCore/Public/Minion/WxMinion.h`. 나머지 파일은 목록·관련 심볼 검색 범위이다.
- **기존 발견 재판정**: 첫 유효 감지 대상을 선택하는 동작은 `Plugins/WxAI/Source/WxAI/Public/WxBTService_UpdateTargetActor.h:30`에 명시된 계약이다. 최근접 우선이라는 요구 근거가 없어 기존의 타겟 선정 버그 항목을 철회한다. 세 노드의 `NodeName` 미설정은 현재도 존재하지만, 기능 결함이나 명시된 프로젝트 규칙 위반은 아니므로 액션 목록에서 제외한다. 프로토콜 중복과 가중치 전원 0의 진단 부족은 유지한다.
- **변경 검증**: 어그로 필터는 기존 타겟과 신규 후보 모두에 적용되며 `IsValid` 검사 후 WxCore 인터페이스를 호출한다. 새 시야 드로우는 에디터 월드·선택된 Pawn/부모로 제한되고 BeginPlay에서 Tick을 끈다. Build.cs와 descriptor의 Wx 플러그인 의존은 WxCore뿐이다.
- **미검토 / 한계**: 전체 38개 소스를 통독하지 않았다. 이번 실행에서 엔진 소스를 다시 대조하거나 빌드·PIE를 실행하지 않았다. BT/Blackboard/BP 에셋, 실제 시야 드로우 표시, 네트워크 실행과 데이터별 재현은 범위 밖이다. 기준 SHA 이후 현재 작업 트리 변경도 포함한 정적 리뷰이며 소스는 수정하지 않았다.

---
*문서 기준 커밋 `1fab89cf4` · 리뷰일 2026-09-12 · 소스 38파일 — `/module-review`로 갱신*

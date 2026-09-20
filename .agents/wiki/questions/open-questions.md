# 미결정과 재검토 목록

상태: current · 범위: 아래 근거의 불일치와 미확인 사항 등록 · 2026-09-20

| ID | 확인할 질문 | 근거 | 상태 |
| --- | --- | --- | --- |
| Q-001 | 피니시 대상은 가까운 적 우선인가, 락온 우선인가? | [피니시 기획](../../../Docs/CombatDesign/그로기_피니시_시스템_기획서.md) 3.1절과 4절 | 기획 결정 필요 |
| Q-002 | 피니시 위치 조정은 PC와 적 중 누가 이동하는가? | 같은 기획의 3.2절과 4절 | 기획 결정 필요 |
| Q-003 | 현광 자원 예외를 공통화할 것인가? | [공통 기획](../../../Docs/SystemDesign/Core_Combat_System.md), [회의자료](../../../Docs/Meeting/2026-09-19 회의자료.md) | 회의자료의 제안, 결정 미확인 |
| Q-004 | WxCombat의 태그 선언을 WxCore로 통합할 것인가? | [실제 선언](../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_IgnoreAbilityTags.h), [리뷰](../../reports/module_review_WxCombat.md) | 코드 현황 확인, 개선안 채택 미확인 |

기획 결정자는 아직 지정하지 않았다. 답이 확정되면 근거와 날짜를 decisions에 기록하고 해당 시스템 페이지를 갱신한다. 구현 변경이 필요한지는 별도로 판단한다.

기존 모듈 설명 9개는 이관 시점에 전체 재검증하지 않아 `needs-review`다. 특히 WxDialogue는 이관 시 미커밋 소스 변경이 있으므로 과거 README만으로 현재 구조를 확정하지 않는다.

관련: [피니시](../systems/finisher.md), [전투 자원](../systems/combat-resources.md), [목차](../index.md).

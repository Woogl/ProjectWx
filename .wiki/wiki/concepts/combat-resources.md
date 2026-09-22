---
title: "전투 자원과 현광의 예외"
category: concept
sources:
  - "raw/notes/2026-09-22-current-resources.md"
  - "raw/notes/2026-09-22-current-combat.md"
  - "raw/notes/2026-09-22-current-damage.md"
created: 2026-09-22
updated: 2026-09-22
tags: [wx, resources]
aliases: []
confidence: medium
volatility: warm
summary: "공통 자원 기획·현광 구현 보고·현재 속성 처리에는 서로 다른 적용 범위가 있다."
---

# 전투 자원과 현광의 예외

공통 자원 기획·현광 구현 보고·현재 속성 처리에는 서로 다른 적용 범위가 있다.

## 공통 자원

코드는 HP/SP/GP/MP/UP와 각각의 최대값을 제공한다. 속성 변경은 음수를 제한하고 현재 자원은 대응 최대값 이하로 제한한다. ASPD의 하한은 0.001이다. 최대값 변경 시에는 권위 측에서 기존 최대값이 양수인 경우 자원의 기본값을 비율로 보정한다. 지속형 보정을 기본값에 굳히지 않기 위해 현재값 대신 기본값을 사용한다.

[공통 기획](../../../Docs/SystemDesign/Core_Combat_System.md)은 스킬 MP, 궁극기 UP 사용을 설명한다. 현재 어빌리티 테이블은 비용 종류를 Custom/SP/MP/UP 중 선택할 수 있다. 기획의 공통 기조, 코드의 표현 가능 범위, 각 에셋의 실제 비용은 구분해야 한다.

## 현광의 보고된 예외

[2026-09-19 회의자료](<../../../Docs/Meeting/2026-09-19 회의자료.md>)는 궁극기 1 비용 MP 3, 궁극기 2 비용 UP 100을 보고한다. 분신 협공마다 MP 1, 도플갱어가 있을 때 적에게 피해를 줄 때마다 UP 10 회복이라고 설명한다. 이는 회의자료의 구현 보고이며 이번에 BP·DataTable에서 추출한 값은 아니다.

**Q-003: 현광 자원 예외를 공통화할 것인가?** 같은 자료는 고유 자원·비용 예외가 늘면 재설계를 논의하자고 제안한다. 제안만으로 공통 규칙 변경이나 테이블 비용 제거를 확정하지 않는다. 기존 미결정을 유지하며 결정자·답은 아직 확인되지 않았다.

## 적용 시 주의

HP 회복 아이템처럼 MP/UP와 무관한 요구에 현광의 예외 논의를 적용하지 않는다. 최대 HP 변경 규칙과 단순 HP 회복도 별개다.

## 관련 문서

- [[combat|WxCombat — 전투 시스템]] ([WxCombat — 전투 시스템](../topics/combat.md))
- [[combat-abilities|전투 어빌리티와 이펙트]] ([전투 어빌리티와 이펙트](../concepts/combat-abilities.md))
- [[combat-damage|피해 처리와 전투 연출]] ([피해 처리와 전투 연출](../concepts/combat-damage.md))
- [[inventory|WxInventory — 아이템 소유와 사용]] ([WxInventory — 아이템 소유와 사용](../topics/inventory.md))

## Sources

- [근거 1](../../raw/notes/2026-09-22-current-resources.md)
- [근거 2](../../raw/notes/2026-09-22-current-combat.md)
- [근거 3](../../raw/notes/2026-09-22-current-damage.md)

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. 인간 검증일 `verified`는 새로 부여하지 않았다.

빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이번에 검증하지 않았다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

</details>

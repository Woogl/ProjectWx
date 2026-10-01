# WX BT 기능 정리

- 자료: `Docs/CombatDesign/WX_BT_기능정리.md` (8745d9c05)
- 종류: 사양서(자체 BT 기능 설명)
- 상태: 대체됨. "자체 태스크 없음"은 이제 사실이 아니다.

## 요지
- 자체 기능 문서는 BT·AM·GA 3종으로 둔다. GA 문서는 없다.
- RandomChoice: Selector처럼 동작하되 자식 하나를 균등 확률로 고른다. AvoidRepeat를 켜면 직전 자식을 뺀다.
- CompareAttributeRatio: 분자·분모 어트리뷰트 비율을 상수와 비교해 하위 실행을 허용한다(예: HP/MaxHP ≤ 50%).
- 문서 작성 규칙은 개요 / 기본 동작 / 세부 옵션 / 사용 목적 / 사용 예시다.

## 반영한 문서
- [기획 작업 도구](../concepts/기획-작업-도구.md)

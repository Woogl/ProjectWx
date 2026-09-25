---
title: "UI 표시 연결 회귀 검증과 제거된 슬롯 정리"
source: ".agents/workflow/tasks/ui-data-interface-removal.md; Source/WxGame/Tests/WxUIPresentationTests.cpp; Saved/Logs/UIDataAutomationFinal.log"
type: notes
ingested: 2026-09-25
tags: [wx, ui, testing, lifetime]
summary: "IWxUIData 제거 후 회귀 3개와 Blueprint 97개 컴파일을 확인했다. Garbage로 무효화된 어빌리티의 슬롯 정리 누락을 재현하고 IsExplicitlyNull 분기로 수정했다."
---

# UI 표시 연결 회귀 검증과 제거된 슬롯 정리

## 확인한 결과

- WxEditor Win64 Development 최종 빌드 성공(exit 0): `Saved/Logs/BuildDoctor/build_2026-09-25_220818_573_38732.log`.
- 어빌리티·PlayerCharacter 리졸버를 저장한 WBP 4개를 재저장했다. 임시 ClassRedirect를 제거한 새 프로세스에서 관련 Blueprint 97개를 로드하고 경고를 오류로 처리해 컴파일했다: `Saved/Logs/UIDataReload.log`, `Saved/UIDataRemoval_verify.json`.
- `Wx.UI.Presentation` 자동화 3개 성공·0개 실패: `Saved/Automation/UIDataRemovalFinal/index.json`, `Saved/Logs/UIDataAutomationFinal.log`.
  - `AbilityRebind`: 최초 표시 값 전달, 공유 슬롯, GE 지속시간과 구별되는 충전 주기, 마지막 후보 제거, 새 후보 연결.
  - `EffectLateConnection`: 이미 조회된 목록에 늦게 연결, 표시 필드, 공유 자식 유지, 아이콘 없는 GE 제외, 효과 제거 시 자식 정리.
  - `CharacterSharing`: AS VM별 공유, 조회 시 표시 유지, 자기 필드로 재초기화, 부모 표시 정리가 공유 AS VM을 종료하지 않음.

## 제거된 어빌리티와 명시적 빈 슬롯

첫 실행의 `AbilityRebind`에서 마지막 후보를 회수한 뒤 제목·충전 수가 남았다. 엔진 `OnRemoveAbility`가 인스턴스를 Garbage로 표시하므로 `CachedAbility.Get()`은 null이 된다. 다음 후보도 null이면 기존의 포인터 비교가 변경 없음으로 판단했다.

`UWxViewModel_Ability::RefreshBoundAbility`는 후보가 같더라도 후보가 null일 때 `CachedAbility.IsExplicitlyNull()`까지 확인한다. 이전 참조가 무효화된 상태면 비용 구독·타이머·표시를 정리한다. 처음부터 빈 슬롯일 때만 기존 조기 반환을 유지한다. 최종 자동화에서 회수·정리·다음 후보 연결을 확인했다.

쿨다운 수치 갱신은 기존대로 GE 추가 콜백 다음 월드 타이머 틱에 이루어진다. 테스트도 타이머를 진행한 뒤 검사한다. 원격 클라이언트 스펙 복제 후 슬롯 재매칭 신호 누락은 이번 수정과 별개이며 미해결로 유지한다. 실제 플레이·화면 품질·네트워크 복제는 검증하지 않았다.

## 구현 식별

기준 HEAD `39f3629a4fa8a5454267fec2b6e4ac9af5b66377`의 미커밋 후속 수정이다.

| 파일 | SHA-256 |
|---|---|
| [슬롯 VM](../../../Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp) | `6117ca5bb5d50359079e7e8679bbd1c4111b285d63f93502fba76babf1b3fe4b` |
| [회귀 테스트](../../../Source/WxGame/Tests/WxUIPresentationTests.cpp) | `1f061fa6b42dbb7ecb2626603eacc20ae69a4826fd5e4b52a928dbb7d57c8841` |

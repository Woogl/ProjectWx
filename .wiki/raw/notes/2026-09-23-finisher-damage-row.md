---
title: "피니셔 피해 행 참조 복구"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, finisher, damage]
summary: "일반 피니셔와 뒤잡의 피해 행을 실제 DT_Damage 행 AM_Shared_Finisher로 연결했다."
---

# 피니셔 피해 행 참조 복구

- 사용자 증상: 몽타주는 재생되지만 HP가 감소하지 않는다.
- `Saved/Logs/Wx.log`의 2026-09-23 02:52 UTC 기록은 ApplyDamageRequest에서 DT_Damage의 AM_Finisher 행 조회 실패를 반복한다.
- Unreal Python으로 GA_Shared_Finisher CDO를 조회한 결과 FinisherVariant와 BackstabVariant의 DamageDataRow가 모두 존재하지 않는 AM_Finisher였다.
- DT_Damage에 존재하는 AM_Shared_Finisher는 CoeffATK=3이며 두 Variant의 참조를 이 행으로 수정했다. 몽타주와 피해 수치는 변경하지 않았다.
- 대상 에셋: `Content/Character/Template/Shared/Abilities/Finisher/GA_Shared_Finisher.uasset`.
- Blueprint 컴파일·저장 후 별도 UnrealEditor-Cmd 프로세스로 두 참조와 실제 행 존재를 재확인했다. 저장 실행은 기존 SourceControl 경로 오류 때문에 exit 1이었지만 저장 성공 기록이 있으며, 새 프로세스 재조회는 exit 0이다. 로그는 `Saved/Logs/FinisherFix.log`, `Saved/Logs/FinisherVerification.log`다.
- 실제 플레이에서 HP 감소는 아직 재확인하지 않았다. 합성 DataTable 기반 GAS 자동화 성공만으로 저작된 Blueprint의 행 참조 유효성을 보장할 수 없다.

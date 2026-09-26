---
title: "Wiki 최신화: 39f3629a4 이후 커밋 추적"
source: "MANUAL"
type: notes
ingested: 2026-09-26
tags: [wx, refresh, combat]
summary: "직전 Wiki 최신화 커밋 39f3629a4 이후 bf6596012까지 19건과 작업 트리를 기사와 대조했다. 지식이 바뀐 커밋은 이미 편찬돼 있었고, 새로 반영할 것은 방어 상수 설정의 입력 제한 메타 제거(계산 보정 유지) 하나였다. 생성 목록 세 개는 스크립트 재실행 결과 바뀌지 않았다."
---

# 범위

2026-09-26 사용자 요청("LLM 위키 최신화해주세요")으로 직전 최신화 커밋 `39f3629a4`(2026-09-25 20:36) 이후 HEAD `bf6596012`까지의 커밋 19건(병합 1건 포함)과 현재 작업 트리를 추적했다. 소스는 정적으로 대조했고 빌드·PIE·에셋 편집기 검증은 하지 않았다.

# 이미 반영된 변경

- `c9e2efec6` UI 표시 데이터 연결의 WxGame 리졸버 분리와 `IWxUIData` 제거: 같은 커밋에서 foundation·modules·ui·editor-tools에 편찬됐다.
- `64fcd9285` 캐릭터 재진입 시 어빌리티 소실·이벤트 중복 수정, `519f929fb` 새 게임 체크포인트 삭제 전 선택 Pawn 검증, `b6f1e9e8c` 어빌리티 방향 선택 공용화와 콤보 몽타주 배열: `raw/notes/2026-09-26-module-review-contracts.md`가 `ad0db6de0` 작업 트리 기준으로 다뤄 game·combat에 편찬됐다.
- `ac6db7670` Exclusive 어빌리티 차단의 GAS 공통 태그 규칙 전환: 같은 커밋에서 편찬됐다.
- `1689d999f`·`4c9f3934e` AnimNotify 역할별 색상 분류와 Editor 설정: `raw/notes/2026-09-26-animnotify-categories.md`로 편찬됐다.
- `11047815a`·`33733d1bb`·`81c5e03dc` Workflow 개선·모듈 리뷰·작업 기록: `bf6596012`에서 편찬됐다. 작업 트리의 구 워크플로우 잔재 제거와 문서 이미지 기능 제거도 `raw/notes/2026-09-26-workflow-legacy-removal.md`·`2026-09-26-workflow-image-removal.md`로 편찬돼 있다.

# 반영 대상이 아닌 변경

- `d644364be` 작업 기록만 바꿨다. `566fb1167`은 WxGame의 UI 표시 자동화 테스트 소스(`WxUIPresentationTests.cpp`)를 지웠고, Wiki 기사는 이 테스트를 서술하지 않는다.
- `7da389b85`(3층 리워크)·`6eb5200de`(적 추가)는 레벨 배치 외부 액터·레벨 인스턴스만 바꿨고 `ad0db6de0`은 병합이다. Wiki는 레벨 배치를 다루지 않으며, 새 캐릭터 에셋은 없어 캐릭터 목록도 바뀌지 않았다.
- `264a71740`은 AGENTS.md 코딩 규칙(엔진 순정 매크로 인라인 예외)이다. Wiki 기사에 해당 서술이 없고 규칙 정본은 AGENTS.md다.
- `184a76fec` 2026-09-26 회의자료는 공유·예정 안건이며 구현 사실이 아니다.
- 작업 트리의 `checkpoint-savegame` 기록 변경은 대시보드로 전달한 사람 테스트 결과로, 작업 기록만 바뀌었다.

# 새로 반영한 변경

- `6adb657b9`: `UWxCombatDeveloperSettings::DefenseConstant`의 `ClampMin`·`UIMin` 메타를 뺐다. 설정 창은 입력값을 제한하지 않는다. 피해 계산 `CalculateDefenseMultiplier`(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`)가 `FMath::Max(DefenseConstant, UE_SMALL_NUMBER)`로 보정해 0 이하 값이 계산에 들어가지 않는 동작은 그대로다.

# 생성 목록

`.agents/scripts/Export-AbilitySystemLists.ps1`을 다시 실행했다. 어빌리티·이펙트·캐릭터 목록 세 개 모두 unchanged였다(어빌리티 40, 세트 9, 캐릭터 7, 몽타주 54, C++ 이펙트 26, GE_ 에셋 6, 피해 테이블 1).

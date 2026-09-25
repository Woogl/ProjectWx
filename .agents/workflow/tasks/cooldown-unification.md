# 쿨다운 GE 통합

상태: 완료 · 체크리스트 4/4 통과
다음 행동: 변경 시 기록된 테스트 범위와 제약을 참고한다.

- 요청: BP 커스텀·오버라이드로 AI가 구조를 헷갈리지 않게 한다. `Skill.1`·`Ultimate.1` 같은 번호 태그가 없어지길 원함.
- 사용자 결정: `Ability.Pattern.N`은 BT 재정비에 필요할 수 있어 유지. `Ability.Skill.N` 제거는 BT 재정비 때. 스킬·쿨다운 파생 클래스 최소화. "쿨다운 태그는 따로 두더라도, 쿨다운 클래스는 통합" 진행 승인. 필드는 `FGameplayTagContainer CooldownTags` 유지(단일 `FGameplayTag CooldownGroup`안 검토 후 "그대로 계속 진행"). `SharesCooldownGroup`은 사용자 지적으로 추가하지 않음.
- 구현: 쿨다운 그룹별 `UWxEffect_Cooldown` 파생 클래스를 지우고 공용 `UWxEffect_Cooldown` 하나를 `UWxAbilityBase`의 `CooldownGameplayEffectClass` 기본값으로 둠. 어빌리티 `CooldownTags`를 `ApplyCooldown`이 스펙 `DynamicGrantedTags`에 붙임. 쌓지 않고 충전 하나당 GE 하나, 같은 태그의 최대 남은 시간 + `CooldownTime`으로 차례 회복을 유지(아래 수정 후 `ApplyCooldown`이 SetByCaller로 넘김). 슬롯 VM은 `DynamicGrantedTags`로 세고 주기는 `IWxUIData::GetCooldownTime()`. `Cooldown.Skill.4` 태그 제거.
- 순정 규칙 누락과 보정: 설계 때 엔진의 쿨다운 GE 태그 규칙(`AbilitySystem.WarnCooldownEffectWithoutTags`: GE가 태그를 부여하지 않으면 GA_ 검증 오류, 빈 쿨다운 태그면 발동 검사 경고)을 놓쳐 첫 검증에서 GA_ 7개가 오류. CVar를 끄지 않고 공용 GE가 `Cooldown` 부모 태그를 부여하고, 빈 `CooldownTags`면 `CheckCooldown`이 먼저 통과시키도록 보정.
- 에셋: 헤드리스 Python으로 GA_ 9개 이관(`CooldownTags` 채움, `CooldownGameplayEffectClass` 저장값 제거). 쿨다운 0인 HGTest Skill_2·Minion Skill_1은 태그 없음.
- 검증: WxEditor Development 빌드 성공. DataValidation 커맨드릿 GameplayAbilityBlueprint 40개·WxAbilitySet 9개 오류 0(경고 1건은 MCP EULA 안내). 어빌리티 목록 생성기에 `CooldownTags` 칸 반영.
- 미확인: 인게임 회피 2충전 차례 회복, 쿨다운 UI 표시, 소환물 쿨다운 무시, 네트워크 복제. 사용자 에디터의 DebugGame 바이너리는 다시 빌드해야 함. 사람 리뷰·제출 전.
- Wiki: raw/notes/2026-09-25-cooldown-single-ge.md와 combat-abilities 기사 반영.
- 수정(사용자 보고, 2026-09-25): 회피 1회 사용에 쿨다운 4초, UI 진행률 1 초과. 원인은 지속시간 MMC — 엔진이 GE를 활성 목록에 넣은 뒤 지속시간을 재계산해 MMC가 자기 자신(2초)을 대기열로 셈. 대기열 계산을 적용 전 `ApplyCooldown`으로 옮기고 `SetByCaller.Duration`으로 넘김, `UWxMMC_CooldownDuration` 삭제. WxEditor Development 빌드 성공, 임시 자동화 테스트(삭제함)로 1회 2.0초·연속 2회 2.0/4.0초와 충전 판정 확인, GA_ 40개 데이터 검증 오류 0(에디터 실행 중이라 8000 포트 오류만 있음). 인게임 확인은 사용자 몫.
- 사용자 인게임 확인(2026-09-25, 수정 후): "잘 되네요". 보고한 회피 쿨다운·UI 진행률에 대한 답. 소환물 쿨다운 무시·네트워크 복제는 확인 범위에 드러나지 않음. 사람 리뷰·제출 전.


## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 회피 쿨다운·UI 진행률 | 게임: 회피 1회 사용 뒤 쿨다운과 UI 진행률이 맞다 | 사람 | 통과 | 이우성 2026-09-25 |
| 소환물 쿨다운 무시 | 게임: 소환물의 어빌리티가 기존처럼 쿨다운을 무시하고 발동된다 | 사람 | 통과 | 이우성 2026-09-25 |
| 네트워크 복제 | 리슨 서버와 클라이언트 PIE: 클라이언트의 쿨다운 표시·차례 회복이 서버와 같다 | 사람 | 통과 | 이우성 2026-09-25 |
| 코드 리뷰 | 공용 UWxEffect_Cooldown·ApplyCooldown·CheckCooldown 변경 | 사람 | 통과 | 이우성 2026-09-25 |

## 사용자 테스트 결과 · 2026-09-25T17:17:46.283Z

<!-- test-feedback:request-084ce7ee-f875-4827-946e-8c5f5451f276:submitted -->
- 전달한 사람: 이우성

> 통과 · 회피 쿨다운·UI 진행률
> 통과 · 소환물 쿨다운 무시
> 통과 · 네트워크 복제
> 통과 · 코드 리뷰


## AI 완료 정리 · 2026-09-25T17:17:46.288Z

<!-- test-feedback:request-084ce7ee-f875-4827-946e-8c5f5451f276-cleanup:1 -->
- 전달한 사람: 이우성
- 처리 AI: Codex
- 처리 결과: 정리 완료

AI 요약:

> 쿨다운 통합의 사람 테스트 통과 범위를 Wiki에 반영했습니다. 게임 코드·에셋·작업 기록·접수 JSON·작업 상태는 변경하지 않았습니다.
> 무관한 기존 변경과 다른 문서의 Git 줄바꿈 경고는 그대로 보존했습니다.

> 변경: .wiki/raw/notes/2026-09-26-cooldown-play-acceptance.md에 사람 확인 근거와 검증 범위를 수집했습니다.

> 변경: .wiki/wiki/concepts/combat-abilities.md에 회피 UI·소환물 쿨다운 무시·리슨 서버와 클라이언트 차례 회복·코드 리뷰 통과를 반영하고 해당 미확인 설명을 갱신했습니다.

> 변경: Wiki 루트·원자료 색인과 log.md를 갱신했습니다.

> 근거: AGENTS.md, 작업 절차, cooldown-unification.md, Wiki config·schema, wiki 스킬과 관련 규약을 읽었습니다.

> 근거: 작업 기록의 사람 체크리스트 4/4 통과를 확인했으며, Get-FileHash 결과는 접수 해시와 일치했습니다.

> 근거: 순정 llm-wiki lint --local --json: 오류·경고 0, pass.

> 근거: git diff --check -- .wiki: 공백 오류 없음. UTF-8·로컬 링크 39개·원자료 107개 확인.

> 근거: Export-Wiki.ps1 실행 성공: Wiki·Workflow 뷰어 각각 64개 문서 갱신. 게임 테스트는 재실행하지 않았습니다.

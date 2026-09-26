# AnimNotify 카테고리와 색상 정리
상태: 완료 · 체크리스트 6/6 통과·AI 정리 완료
다음 행동: 변경 시 기록된 테스트 범위와 제약을 참고한다.

## 결정
- 2026-09-26 사용자: "네, 그렇게 해주세요. UWxCombatDeveloperSettings의 불필요 데이터는 제거해주세요."
- 색상 설정은 WxCore에 유지하고 Config=Editor, DefaultConfig로 변경한다. 기존 에디터 전용 전처리 보호는 유지한다. WxCombatDeveloperSettings의 색상 데이터는 이미 제거되어 있으며, 남은 DefenseConstant는 실제 피해 계산에서 사용하므로 유지한다.
- 2026-09-26 사용자: "제출해주세요." — 이번 변경의 커밋·푸시 요청. 사람 테스트 결과는 별도 확인 전까지 대기로 유지한다.
- 2026-09-25 사용자: "네, 적용해주세요."
- Attack #E86666: WeaponAttack, AreaDamage, SpawnProjectile, FinisherDamage.
- AbilityFlow #E8BE55: ComboWindow, StartRecovery, FinisherVictim.
- Effect #B58AE6: ApplyGameplayEffect, UseItem.
- Movement #62A9E8: Rush, SnapToTarget.
- Presentation #63C49A: CameraMove, SlowTime, SkillCutscene.
- Misc #929DAA: SpawnMinion, DespawnMinion, ReportNoise.
- 색상은 sRGB에서 선형 색상으로 변환한다. 도메인 간 의존 없이 공유할 수 있도록 WxCore에 색상 설정 정의를 둔다. 실행 로직과 Details 속성 분류는 변경하지 않는다.
- 2026-09-25 사용자: "이 노티파이 색상 데이터는 #if 으로 에디터 전용으로 만들어버리는게 나을까요?"
- 엔진 AnimNotify/AnimNotifyState의 NotifyColor는 WITH_EDITORONLY_DATA로 보호된다. 공용 색상 필드와 초기화도 같은 조건으로 보호하고, 17종 GetEditorColor 선언·정의와 설정 include는 WITH_EDITOR로 제한한다.

## 테스트 체크리스트
| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 분류와 색상 | 17종 매핑, 6개 sRGB 값, 미리보기 색상 참조 정적 검사 | AI | 통과 | 17종 전체 매핑과 AreaDamage·SnapToTarget 미리보기 참조 확인, 기존 색상 필드 참조 없음 |
| 에디터 전용 분리 | 색상 필드·초기화와 17종 접근 경로의 전처리 조건 확인 | AI | 통과 | WITH_EDITORONLY_DATA 필드·초기화 확인, 34개 파일 선언·정의·include 보호 검사 통과. 패키지 빌드는 미실행 |
| Editor 빌드 | WxEditor Win64 Development 빌드 | AI | 통과 | Saved/Logs/BuildDoctor/build_2026-09-26_000757_357_2060.log: Result Succeeded, 종료 코드 0 |
| 설정 저장 위치·잔존 데이터 | Editor config 선언과 기존 색상 설정 잔존·방어 상수 사용처 확인 | AI | 통과 | DefaultConfig 유지, Config 내 이전 색상 키 없음, WxEffect_Damage.cpp에서 DefenseConstant 사용 |
| 코드 리뷰 | 공용 색상 설정 이동과 17종 GetEditorColor 매핑 확인 | 사람 | 통과 | 이우성 2026-09-25 |
| 타임라인 표시 | 에디터 재시작 후 6개 색상과 라벨 가독성, 설정 변경의 DefaultEditor.ini 저장·재로드 확인 | 사람 | 통과 | 이우성 2026-09-25 |

## 구현
- 공용 설정: Plugins/WxCore/Source/WxCore/Public/WxAnimNotifySettings.h 및 Private/WxAnimNotifySettings.cpp.
- 프로젝트 설정의 Wx → Wx Anim Notify Settings에서 6개 색상을 조정한다. 기존 Wx Combat Settings의 색상 3개는 제거했다.
- WxCore에 엔진 DeveloperSettings 의존성만 추가했다. WxAI·WxInventory가 WxCombat을 참조하지 않는다.
- 타임라인 라벨, 노티파이 실행 로직, 에셋은 변경하지 않았다.
- 변경 범위 git diff --check 통과. Wiki 정리는 사람 확인 후 완료 단계에서 진행한다.


## 사용자 테스트 결과 · 2026-09-25T15:49:48.558Z

<!-- test-feedback:request-cdab0f99-7316-4f63-807d-654f9a6f4f0b:1 -->
- 전달한 사람: 이우성
- 처리 AI: Codex
- AI 처리 상태: 확인 필요

사람 확인:

> 통과 · 코드 리뷰
> 통과 · 타임라인 표시

AI 처리 결과:

> AnimNotify 문서·Wiki 정리를 완료했습니다. 기존 체크리스트 6개 항목의 통과 결과를 보존했습니다.

> 변경: WxCore와 편집기 도구 Wiki에 색상 6개·17종 매핑·Editor 설정·전처리 보호·사람 확인 범위를 반영했습니다.

> 변경: 원자료·색인·Wiki 로그를 갱신하고 뷰어를 재생성했습니다.

> 변경: 게임 코드·에셋·작업 기록·접수 JSON은 수정하지 않았습니다.

> passed · 작업 기준과 기록 보존: AGENTS.md·process/index.md·animnotify-categories.md를 읽었으며 작업 전후 기록 SHA-256이 접수 taskHash와 일치했습니다.

> passed · Wiki 반영 검증: wiki 스킬·config.md·schema.md에 따라 반영했습니다. 대상 문서 3개의 로컬 링크 53개·UTF-8 검사와 git diff --check를 통과했습니다.

> failed · 전체 Wiki 검사: llm-wiki lint --local --json 결과 범위 밖 module-review-contracts 원자료에 경고 1개·제안 1개가 남았습니다.

> passed · 뷰어 재생성: Export-Wiki.ps1 실행으로 Wiki·Workflow 뷰어 65개 문서를 재생성했습니다.

> 남은 확인: 전체 Wiki 검사에는 범위 밖 module-review-contracts 원자료의 색인 누락과 미편찬 지적이 남아 있습니다. 이번 작업 관련 지적은 없습니다.

## 사용자 완료 확정 · 2026-09-26

- 사용자(대화): "지금 완료로 바꿔주세요."
- 근거: 체크리스트 6/6 통과(사람 항목 2개 포함)와 AI의 Wiki 정리·뷰어 재생성 완료. AI 처리의 남은 확인은 다른 세션이 작성 중이던 원자료(module-review-contracts)의 일시적 lint 경고로 이 작업과 무관했고, 확정 시점 lint는 0건이다.
- AI 결과의 plan 칸 문장이 제목 아래 `## 구현 계획` 절로 잘못 기록돼 지웠다. 서버가 구현·테스트 결과 처리에서 온 질문·계획을 받지 않도록 고쳤다.

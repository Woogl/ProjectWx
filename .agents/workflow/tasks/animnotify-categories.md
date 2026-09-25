# AnimNotify 카테고리와 색상 정리
상태: 확인 대기 · Config=Editor 변경과 불필요 데이터 점검을 마치고 Editor 빌드를 통과했다.
다음 행동: 코드 리뷰와 에디터 색상·DefaultEditor.ini 저장 반영을 확인한다.

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
| 코드 리뷰 | 공용 색상 설정 이동과 17종 GetEditorColor 매핑 확인 | 사람 | 대기 | |
| 타임라인 표시 | 에디터 재시작 후 6개 색상과 라벨 가독성, 설정 변경의 DefaultEditor.ini 저장·재로드 확인 | 사람 | 대기 | |

## 구현
- 공용 설정: Plugins/WxCore/Source/WxCore/Public/WxAnimNotifySettings.h 및 Private/WxAnimNotifySettings.cpp.
- 프로젝트 설정의 Wx → Wx Anim Notify Settings에서 6개 색상을 조정한다. 기존 Wx Combat Settings의 색상 3개는 제거했다.
- WxCore에 엔진 DeveloperSettings 의존성만 추가했다. WxAI·WxInventory가 WxCombat을 참조하지 않는다.
- 타임라인 라벨, 노티파이 실행 로직, 에셋은 변경하지 않았다.
- 변경 범위 git diff --check 통과. Wiki 정리는 사람 확인 후 완료 단계에서 진행한다.

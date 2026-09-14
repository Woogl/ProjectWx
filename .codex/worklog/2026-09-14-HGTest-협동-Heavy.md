# HGTest 분신 협동 Heavy Attack

## 계획

- GA_HGTest_Attack_Heavy_2와 전용 몽타주를 기존 Heavy 기반으로 생성한다.
- 기존 입력·비용·공중 및 회피 제한을 유지하고 State.Minion.Active 필수/차단 조건으로 두 Heavy를 교체한다.
- ABS_HGTest에 등록하고 Skill_2의 분신 명령 방식을 재사용한다.
- 블루프린트 컴파일, UE 5.8 WxEditor Development 빌드, 분신 생성·소멸 전후 교체를 검증한다.

## 완료

- 2026-09-15 완료. C++ 변경 없이 에셋 4개로 구현했다.
- GA_HGTest_Attack_Heavy_2 / AM_HGTest_Attack_H_2 생성. 기존 Heavy 입력·비용·태그·공중 및 회피 제한을 유지했다.
- 일반 Heavy는 State.Minion.Active를 차단하고 협동 Heavy는 필수로 요구한다. ABS_HGTest에 협동 Heavy를 추가했다.
- 전용 몽타주 시작(0.0001초)에 WxAnimNotify_CommandMinion으로 Ability.Skill.1을 명령한다. 기존 GA_Minion_Skill_1 / AM_Minion_Skill_1을 재사용하며 분신 소멸 명령은 추가하지 않았다.
- UE 5.8.2 WxEditor Win64 Development 빌드 성공.
- 별도 프로세스에서 저장 에셋 재조회, 두 BP 경고 포함 컴파일, 태그·입력·비용·등록·명령 참조 검증 통과.
- PIE에서 실제 Skill_1 소환 전후 두 Heavy 클래스의 발동 허용/차단과 분신 몽타주 재생, 분신 제거 후 일반 Heavy 복귀를 확인했다. 입력 키 주입 대신 ASC 클래스 발동 API로 검증했다.
- 검증 결과: Saved/Heavy2Verification.json. 검증용 맵 변경은 저장하지 않고 에디터를 종료했다.


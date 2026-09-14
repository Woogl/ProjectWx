# Hit 흐름 최종 정리

## 계획

- 승인한 검수 항목 세 가지에 한정한다. 투사체 반사는 Damage 적용 성공 시에만 허용한다.
- Wrapper의 AdditionalEffects 중간 배열 복사를 제거하되 ExtraSpecs 준비 시점을 유지한다.
- 권한 여부와 반응 함수의 동적 태그 반복 조회를 지역 변수로 정리한다. 별도 타입이나 헬퍼는 추가하지 않는다.
- 투사체 적용 거부 시 반사 방지를 검증하고 기존 전투 자동화 테스트, UE 5.8 WxEditor Development 빌드와 diff 검사를 수행한다.

## 완료

- 투사체의 ApplyDamage 반환값을 보관하여 히트스톱과 반사 분기에 함께 사용한다. 자식 적용 거부 및 사망 대상으로의 반사를 차단하고 기존 파괴 경로를 따른다.
- AdditionalEffects 중간 복사를 제거하고 원본 행에서 ExtraSpecs를 미리 생성한다. 권한 여부와 ProcessDamageTaken의 태그 반복 조회를 지역 변수로 정리했다.
- 실제 BP_Soldier_Projectile 충돌 델리게이트를 호출하는 회귀 검증을 추가했다. 정상 퍼펙트 가드의 소유권 전환, 자식 거부·사망 시 반사 없는 파괴, 회피 시 통과를 모두 확인했다.
- 테스트 준비 과정에서 BeginPlay 없는 월드의 Actor 동적 콜백 실행 제한과 파괴 시 Owner 해제를 반영했다. 게임 동작 코드는 승인된 세 항목에 한정했다.
- UE 5.8.2 WxEditor Win64 Development 최종 빌드 성공(종료 코드 0). 로그: C:/Wx/Saved/Logs/BuildDoctor/build_2026-09-14_223547_123_33716.log.
- Wx.Combat.HitWrapper.Application 최종 성공: 1개 성공, 실패 0, 테스트 경고 0. 보고서: C:/Wx/Saved/Automation/HitFinalCleanup/index.json. 로그: C:/Wx/Saved/Logs/HitFinalCleanup.log.
- git diff --check 통과. 네트워크 PIE는 이번 작업에서 실행하지 않았다.

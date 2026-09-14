# Damage Spec 관찰자 제거

## 계획

- 승인한 설계에 따라 FWxDamageSpecObserver와 델리게이트 구독 및 실행 Spec 전체 복사를 제거한다.
- 기존 DamageResponse의 실행 완료 콜백에서 피해량, 반사량, 반사 출력 여부와 결과 태그를 Context에 기록하고 Wrapper가 적용 직후 읽는다. Hit Cue의 대상 태그 스냅샷도 보존한다.
- 결과는 로컬 전달용으로만 사용하며 기존 복제 형식을 유지한다. Duplicate 및 역직렬화 시 로컬 결과를 초기화한다.
- Damage는 수치 처리·Floater·결과 기록을, Wrapper는 반응과 Hit Cue를 담당한다. 기존 반환값과 이벤트 순서를 유지한다.
- 기존 전투 테스트에 크리티컬 결과 태그와 Cue 태그 전달 검증을 보강하고 UE 5.8 WxEditor Development 빌드, 자동화 테스트, diff 검사를 실행한다.

## 완료

- FWxDamageSpecObserver와 ASC 적용 델리게이트 등록·해제 및 실행 Spec 전체 복사를 제거했다. 두 관찰자 모두 제거된 상태다.
- DamageResponse 실행 완료 콜백이 피해량·반사량·반사 출력 유무·동적 결과 태그·대상 태그 스냅샷을 Context에 기록한다. Wrapper는 자식 적용 직후 결과를 읽고 기존 반응·Cue·추가 효과 순서대로 처리한다.
- 로컬 결과 초기화를 ResetDamageResult로 모아 Wrapper 진입, Duplicate, 역직렬화에서 사용한다. 기존 복제 필드와 직렬화 순서를 유지했다.
- 크리티컬 100% 조건에서 200 피해가 Hit 이벤트에 전달되고 크리티컬 태그가 Hit Cue와 Floater에 보존되는지 검증했다. Hit Cue의 대상 태그 보존 및 Duplicate의 결과 초기화·원본 보존도 검증했다.
- 기존 정상/회피/가드 브레이크/퍼펙트 가드/0 피해/0 반사/적용 거부/사망·그로기 순서/재진입 테스트를 포함하여 Wx.Combat.HitWrapper.Application 성공. 1개 성공, 실패 0, 테스트 경고 0.
- UE 5.8.2 WxEditor Win64 Development 빌드 성공(종료 코드 0). 로그: C:/Wx/Saved/Logs/BuildDoctor/build_2026-09-14_222345_229_8160.log.
- 자동화 보고서: C:/Wx/Saved/Automation/HitNoObserver/index.json. 로그: C:/Wx/Saved/Logs/HitNoObserver.log.
- git diff --check 통과. 네트워크 PIE 및 직렬화 왕복 테스트는 이번 변경에서 실행하지 않았다.

# 대미지 GEComponent 후처리 이관

## 계획

- 승인된 GEComponent 방식으로 ProcessDamageTaken / ProcessPerfectGuard를 AttributeSet에서 제거하고 UWxEffectComponent_DamageResponse로 옮긴다.
- UWxEffect_Damage에 컴포넌트를 부착하고 OnGameplayEffectExecuted에서 Spec의 실행된 메타 속성 기록을 읽는다. 0 반사량도 퍼펙트 가드 결과로 처리한다.
- AttributeSet의 수치 반영·메타 초기화·사망·그로기 처리는 유지한다. 공격 태그 필터와 이벤트 payload, 가드 취소 선행 순서를 보존한다.
- GE 기본 Cue 및 조건부 GE 이후로 후처리 시점이 이동하는 것은 승인된 설계에 따른다. 기존 미커밋 변경을 보존한다.
- UE 5.8 WxEditor Win64 Development 빌드와 관련 분기·호출 순서 검토로 검증한다.

## 완료

- UWxEffectComponent_DamageResponse를 추가하고 UWxEffect_Damage에 부착했다. 두 후처리 함수의 선언·정의·호출을 AttributeSet에서 제거했다.
- Spec의 ModifiedAttributes에서 적용량을 읽으며, 반사량 0도 기록 존재로 처리한다. 가드 취소·이벤트 payload·플로터 로직은 유지했다.
- AttributeSet의 HP·사망·그로기 처리와 메타 초기화는 유지하고 관련 주석을 갱신했다.
- UE 5.8 WxEditor Win64 Development 빌드 성공(종료 코드 0). 추가 테스트의 FNativeGameplayTag 비교 컴파일 오류를 수정한 뒤 통과했다.
- 빌드 로그: C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-08_002612_368_8396.log
- Wx.Combat.DamageResponse.Execution 자동화 테스트 성공: 실제 GE Modifier 적용에서 그로기→사망→피격 순서, 초과 피해량, 공격 태그 필터, 반사량 0과 payload 태그 검증.
- 테스트 로그: C:\Wx\Saved\Logs\DamageResponseAutomation.log
- 변경 파일 diff 공백 검사 통과. 기존 미커밋 변경 보존.
- 한계: 기본 Cue·조건부 GE 뒤로 이동한 후처리 시점의 멀티플레이 연출과 실제 가드 어빌리티 상호작용은 플레이 검증하지 않았다. 자동화 테스트는 계산식을 고정 Modifier로 대체하여 GE 실행 훅을 독립 검증한다.

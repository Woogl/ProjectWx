# Hit 적용 결과 전달 단순화

## 계획

- 승인한 설계에 따라 FWxDamageApplicationObserver를 삭제하고 Wrapper가 Context의 로컬 bDamageApplied에 자식 적용 성공 여부를 기록한다. ApplyDamage는 이를 반환한다.
- 실행 Spec 결과가 필요한 FWxDamageSpecObserver는 유지한다. 기존 복제 형식, 전투 처리 순서와 반환 계약을 보존한다.
- 로컬 결과는 Context 복제와 역직렬화에서 초기화한다. 기존 전투 자동화 테스트와 Context 복사 검증, UE 5.8 WxEditor Development 빌드 및 diff 검사를 수행한다.

## 완료

- FWxDamageApplicationObserver와 해당 델리게이트 구독을 제거했다. Wrapper가 자식 적용 Handle의 성공 여부를 공유 Context에 기록하고 ApplyDamage가 직접 반환한다.
- bDamageApplied는 복제하지 않는다. Duplicate와 역직렬화에서 초기화하며 기존 복제 필드와 순서는 유지했다.
- 기존 전투 테스트 및 재진입 타격 격리 검증을 통과했다. Context Duplicate에서 원본 성공값 보존과 복사본 결과 초기화를 추가 검증했다.
- UE 5.8.2 WxEditor Development 빌드 성공. 로그: C:/Wx/Saved/Logs/BuildDoctor/build_2026-09-14_221859_610_14148.log.
- Wx.Combat.HitWrapper.Application 성공: 1개 성공, 실패 0, 테스트 경고 0. 보고서: C:/Wx/Saved/Automation/HitResultSimple/index.json.
- git diff --check 통과. 네트워크 PIE는 실행하지 않았다.
- FWxDamageSpecObserver 제거는 후속 설계 제안 단계이며 이번 변경에서는 유지했다.

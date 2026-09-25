# IWxUIData 제거

- 상태: **확인 대기** — 구현·자체 검증 완료. 사용자 코드 리뷰와 실제 플레이 확인이 남았다.
- 현재 결정: WxCombat의 원본 데이터·규칙, WxUI의 VM·GAS 공통 갱신, WxGame 리졸버의 데이터 연결로 분리한다.
- 다음 행동: 인게임에서 플레이어 스킬·퀵슬롯 아이콘/충전, 버프 추가·제거, 보스와 적 이름표의 이름·초상화 표시를 확인한다. 원격 클라이언트의 기존 스펙 복제 후 재매칭 신호 누락은 별도 미해결 사항이다.

- 요청·합의(2026-09-25): IWxUIData를 제거한다. WxCombat은 어빌리티·이펙트 데이터와 규칙, WxUI는 VM과 GAS 공통 구독·갱신, WxGame은 리졸버의 데이터 연결을 담당한다. 별도 중계 서브시스템이나 공용 대체 인터페이스를 추가하지 않는다. 사용자 "네, 그렇게 고쳐주세요"로 구현 요청.
- 조사 완료: 사용처는 어빌리티 슬롯, 활성 효과 목록, 캐릭터 이름·초상화, WxEditor BP 썸네일이다. 슬롯·캐릭터 리졸버 경로를 저장한 WBP 4개를 확인했다. 생성 목록은 Export-AbilitySystemLists.ps1로 갱신했으며 변경 없었다.
- 구현: 어빌리티 변경 델리게이트와 효과 표시 구성 델리게이트를 WxUI VM에 두고 WxGame 리졸버의 정적 함수로 연결. 어빌리티·PlayerCharacter 리졸버를 WxGame으로 이동하고 AbilitySystem 리졸버를 추가. 캐릭터 VM은 AS VM·이름·초상화를 직접 받는다. 썸네일은 WxEditor에서 WxCombat·WxGame 타입을 직접 읽는다. 공용 IWxUIData h/cpp 삭제.
- 수명: 슬롯 최초 매칭 전에 연결하고 Deinitialize에서 해제한다. 공유 AS VM의 효과 연결은 한 번만 설정하며 연결 전 조회된 빈 목록은 현재 활성 GE로 보충한다. 캐릭터 공유 VM의 Outer는 AS VM이며 공유본을 재조회해도 초기화·이미지 요청을 반복하지 않는다.
- 빌드: 최종 WxEditor Win64 Development 성공(exit 0). 로그 `Saved/Logs/BuildDoctor/build_2026-09-25_220818_573_38732.log`. UHT·컴파일·링크 확인. 테스트 추가 과정의 리플렉션 API 이름 오류는 `FindFProperty`로 수정했다.
- 에셋: WBP_Ability·WBP_ItemQuickSlot·WBP_Nameplate_Player·WBP_PlayerSkills 4개를 재저장했다. 임시 ClassRedirect를 제거한 새 프로세스에서 WBP·캐릭터 BP·GA·GE 97개를 경고를 오류로 처리해 로드·컴파일했다. 로그 `Saved/Logs/UIDataReload.log`, 결과 `Saved/UIDataRemoval_verify.json`. DefaultEngine.ini의 최종 내용 변경과 리다이렉트는 없다.
- 자동화: `Wx.UI.Presentation` 3개 성공·0개 실패. `AbilityRebind`(최초 값 전달·공유·충전 주기·제거·재매칭), `EffectLateConnection`(늦은 연결·아이콘 없는 효과 제외·제거 정리), `CharacterSharing`(공유·재초기화·공유 자식 수명). 최종 로그 `Saved/Logs/UIDataAutomationFinal.log`, 보고서 `Saved/Automation/UIDataRemovalFinal/index.json`.
- 자체 검증 중 수정: 어빌리티 제거 시 엔진이 인스턴스를 Garbage로 표시해 `CachedAbility.Get()`이 null이 된다. 후보도 null이면 기존 조기 반환이 제목·충전 초기화를 생략했다. `IsExplicitlyNull()`로 처음부터 빈 슬롯과 무효화된 슬롯을 구분했다. 테스트 첫 실행에서 재현했고 수정 후 통과했다. 쿨다운 테스트는 기존 다음 틱 갱신 계약에 맞춰 타이머 틱 후 검사한다.
- 정적 검증: 소스의 IWxUIData/UWxUIData 참조 0건, 도메인 간 Build.cs 의존성 추가 없음, diff 공백 검사 통과. WxEditor에만 WxCombat·WxGame 의존성을 추가했다.
- 기록: Wiki 원자료를 수집하고 ui·foundation·modules·combat-abilities·editor-tools에 통합했다. 순정 Lint와 저장소 링크 검사 통과, 뷰어 재생성. 인간 리뷰·플레이 수용은 아직 없다.
- 기존 작업 트리의 다른 모듈 리뷰·체크포인트·Wiki 변경은 별도 작업이며 보존한다.

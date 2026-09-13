# Nameplate 뷰모델 직접 주입

## 계획

- 앞서 제안한 접근 방식에 대한 사용자의 승인에 따라 NameplateComponent가 Owner의 ASC로 Character GetOrCreate를 호출하고 위젯에 직접 주입한다.
- 공유 뷰모델 정책, 거리·스케일 처리, 기존 위젯 바인딩을 유지하고 위젯 생성·교체에 연결한다.
- 관련 위젯 에셋을 Manual로 전환하고 Resolver 참조와 클래스를 제거한다.
- UE 5.8 WxEditor Development 빌드 및 실제 에셋 생성·교체·공유 유지 검증을 수행한다.

## 완료

- UWxNameplateComponent의 InitWidget에서 새로 생성된 위젯에만 Owner ASC의 공유 Character VM을 주입하고, SetWidget에서 교체된 위젯에도 연결한다. 매 Tick의 Nameplate 전체 탐색은 없으며 기존 거리·스케일 로직은 그대로 유지했다.
- 기존 GetOrCreate를 재사용하고 MVVM View가 참조를 유지한다. 컴포넌트는 공유 VM을 직접 Deinitialize하지 않는다.
- WBP_Nameplate_Enemy의 Character 소스를 Manual, Optional, Setter 생성으로 변경하고 Resolver 참조를 제거해 컴파일·저장했다. Nameplate Resolver 헤더와 cpp를 삭제했다. Content/UI 및 WxUI 소스에서 해당 클래스 참조가 남지 않은 것을 확인했다.
- Source/WxGame/Tests/WxNameplateViewModelTest.cpp에 실제 BP_Sandbag ASC와 WBP_Nameplate_Enemy를 사용하는 회귀 테스트를 추가했다. 최초 생성, 반복 InitWidget, 위젯 교체, 실제 Slate 해제·재생성, 위젯 제거 후 공유 상태 유지, 위젯 재생성 시 같은 VM 연결을 검증했다.
- UE 5.8.2 WxEditor Win64 Development 최종 빌드 성공(종료 코드 0). 로그: C:/Wx/Saved/Logs/BuildDoctor/build_2026-09-13_210226_729_5940.log
- Wx.UI.Nameplate.ManualViewModel: 성공 1, 실패 0, 경고 0. 결과: C:/Wx/Saved/Automation/NameplateManual/index.json
- 검증 과정에서 테스트 월드 중복 초기화, 추상 캐릭터 생성, MVVM 확장 단독 호출 및 보호된 함수 접근을 수정해 실제 Slate 생성 경로로 검증했다. 초기 에셋 저장 실행의 SourceControl 경로 오류와 별개로 Python 저장·컴파일 성공을 확인했고, Resolver 제거 후 최종 테스트에서도 에셋 로드와 바인딩이 정상 동작했다.
- 기존 다른 작업의 변경은 수정하지 않았다. 플레이 화면의 시각 검증과 대량 생성 성능 측정은 수행하지 않았다.

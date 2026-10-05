# StateTree 편집

조회는 `state_tree_toolset.StateTreeTools`, 상태·태스크·전이 기입은 `ObjectTools.set_properties`, 파라미터·바인딩·링크·컴파일·생성은 `WxToolset.WxStateTreeToolset`이 맡는다.
편집 뒤에는 `CompileStateTree` → `SavePackages` → 재조회 순서로 확인하고, 최종 판정은 PIE 동작으로 한다.

## 조회

- `get_root_states` → `get_children`·`get_transitions`·`get_enter_conditions`·`get_tasks`. 상태는 `/Game/.../ST_X.ST_X:StateTreeEditorData_0.StateTreeState_0` 같은 refPath로 돌아온다.
- 바인딩된 프로퍼티는 조회 값이 `None`으로 보인다. 무엇에 물렸는지는 `GetBindings`나, 노드 `iD`만 채운 `get_node_description`으로 본다.

## 상태·태스크 기입

- 상태 프로퍼티: `transitions`·`children`(순서 포함)·`enterConditions`·`bHasRequiredEventToEnter`+`requiredEventToEnter`·`selectionBehavior`·`TasksCompletion`·`tasks`. 스키마는 상태에 `list_properties`로 본다.
- 상태 신설: `Children` 원소에 클래스 refPath `{"refPath":"/Script/StateTreeEditorModule.StateTreeState"}`를 넣으면 서브오브젝트가 생긴다. 빈 `Children`에 N개를 넣고 각 자식의 `Name`·`TasksCompletion`·`Tasks`·`Transitions`를 한 번에 써도 된다. 원소가 있는 배열에 추가하며 값까지 쓰면 거절되니 `[]`로 비운 뒤 전량 다시 쓴다.
- 태스크 원소: `{node:{_structType:태스크}, instance:{_structType:인스턴스 데이터, 필드...}, iD:GUID}`. `bConsideredForCompletion`은 `node` 쪽이다. 태스크·조건·평가자에 준 `iD`는 유지된다.
- **태스크 배열은 중간 삽입·재정렬하지 않는다.** 원소 타입은 제자리에 남고 `iD`만 옮겨져, `iD`에 매인 바인딩이 엉뚱한 노드로 넘어간다. 추가는 맨 뒤에 붙이고(기존 원소는 조회 결과 그대로), 순서·타입을 바꾸려면 `[]`로 비운 뒤 원래 `iD`를 붙여 전량 다시 쓴다. 원래 `iD`를 유지하면 바인딩도 살아남는다.
- **축소 뒤 남은 원소가 빈 채로 남을 수 있다.** 원소를 원본 그대로 되돌려 넣어도 제거 지점 뒤 원소의 `node`·`instance`가 비고 `iD`만 남은 사례가 있다. 컴파일·저장은 성공을 답한다. 축소 후 `_structType`을 다시 확인하고, 비었으면 지웠다 맨 뒤에 다시 붙인 뒤 바인딩을 다시 건다.
- **`TasksCompletion` 기본값은 Any다.** 즉시 끝나는 태스크와 기다리는 태스크를 섞으면 진입하자마자 상태가 완료돼 트리가 한 프레임에 끝까지 지나간다. 기다리는 상태는 `All`을 명시하고, 계속 Running인 태스크는 `bConsideredForCompletion=false`로 뺀다.
- 레벨 액터를 노드·인스턴스 값에 리터럴로 넣으면 컴파일 오류("Direct Actor references are not allowed")가 난다. 바인딩으로 주입하거나 UOL 파라미터를 쓴다. UOL 파라미터는 `AddRootParameter`의 `MetaJson`에 `{"AllowedLocators":"Actor"}`를 줘야 액터 픽커가 뜬다.
- 유틸리티 선택은 자식마다 Constant 고려 사항이 하나는 있어야 점수가 0이 아니다.

## 전이

- 전이는 타깃 상태의 `ID`로 링크된다. `state` 안에 `linkType`을 명시한다. 옛 상태의 `ID`를 새 상태에 복사하면 전이 정의를 그대로 옮길 수 있다.
- **`NextState`를 `set_properties`로 넣으면 자기 자신으로 가는 `GotoState`로 바뀐다.** 컴파일도 통과하고 런타임에 매 틱 재진입한다. 형제 상태의 `ID`를 읽어 `GotoState`로 명시한다.
- **전이를 한 번에 둘 이상 추가하면 마지막 원소가 기본값(OnStateCompleted·자기 자신 GotoState)으로 초기화되고, 두 번째부터 `iD`가 새로 발급된다.** 같은 배열을 같은 크기로 한 번 더 쓰고, 다시 조회해 실제 `iD`로 바인딩한다.
- On Delegate 전이: `trigger`를 `OnDelegate`로, `requiredEvent.tag`를 비우고, `AddBinding`으로 소스=발행자 노드 `iD`+델리게이트 프로퍼티, 타깃=전이 `iD`+`"DelegateListener"`를 건다. 미연결이면 컴파일 오류가 나므로 컴파일 통과가 곧 연결 확인이다.

## 바인딩·파라미터·링크

- 바인딩 소스 ID와 경로 규칙은 `AddBinding` 설명에 있다. 경로는 표시명이 아닌 프로퍼티 이름이다.
- Context Actor 소스는 그 프로퍼티가 스키마의 컨텍스트 클래스에 있고 편집 플래그(`EditAnywhere`·`EditInstanceOnly`)가 있어야 풀린다. 안 풀리면 루트 파라미터를 소스로 두고, 배치마다 다른 값은 컴포넌트 `StateTreeRef`의 파라미터 오버라이드(`SetReferenceParameterValues`)로 준다.
- 루트 파라미터를 같은 이름으로 지웠다 다시 만들면 그 파라미터를 소스로 쓰던 바인딩이 그대로 이어진다. 타입을 바꿔 다시 정의해도 다시 걸 필요가 없다.
- 링크 상태(Linked Asset)는 `LinkStateToAsset` 후 `SetStateParameterValues`로 값을 쓴다. 다시 링크하면 값이 기본값으로 돌아간다. 루트 파라미터→링크 상태 바인딩이 남아 있으면 런타임에 오버라이드 값을 덮어쓴다.

## 에셋 생성

- 일반 트리는 `CreateStateTree`, AI 트리는 `CreateAIStateTree`로 만든다.
- 기존 트리를 복제하면 스키마의 `ContextActorClass`가 원본 것으로 남아 시작이 거부된다. 복제 후 다시 설정한다.

## 진단

- 실행 흐름은 `LogStateTree`를 VeryVerbose로 올려 러너 오너(예: `WxGameState_0`) 줄로 거른다.
- 링크 에셋 오버라이드가 걸린 상태는 Enter state 로그의 상태 이름이 기본 링크 에셋 기준으로 잘못 찍힌다. Exit 줄은 맞으니 동작 문제가 아니다.

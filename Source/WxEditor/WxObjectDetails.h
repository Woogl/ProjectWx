// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "PropertyEditorDelegates.h"

class IDetailLayoutBuilder;

// 모든 오브젝트의 디테일 패널에서 "Wx" 카테고리(하위 "Wx|…" 포함)를 최상단에 올리는 "Object" 클래스 커스터마이제이션.
// "Object" 자리는 엔진 DetailCustomizations 의 FObjectDetails(CallInEditor 버튼·Experimental 경고)가 차지하고 있고 재등록은 교체라, 원본을 안에 품고 먼저 호출한 뒤 정렬만 덧붙인다.
// 하위 카테고리는 부모 "Wx" 안에 중첩 렌더링되므로 부모만 옮기면 따라온다. 하위를 직접 EditCategory 하면 최상위로 튀어나오니 건드리지 않는다.
class FWxObjectDetails : public IDetailCustomization
{
public:
	/** 모듈의 섹션 매핑도 같은 이름을 쓴다. */
	static const FName WxCategoryName;

	/** InnerFactory 는 이 등록이 대체한 원본 "Object" 커스터마이제이션의 팩토리. 바인딩이 없으면 정렬만 수행한다. */
	static TSharedRef<IDetailCustomization> MakeInstance(FOnGetDetailCustomizationInstance InnerFactory);

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	virtual void CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder) override;
	virtual void PendingDelete() override;

private:
	void MoveWxCategoryToTop(IDetailLayoutBuilder& DetailBuilder);

	TSharedPtr<IDetailCustomization> Inner;
};

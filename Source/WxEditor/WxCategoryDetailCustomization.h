// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

// Wx 로 시작하는 카테고리를 디테일 패널 최상단으로 정렬한다.
// UObject::StaticClass() 에 등록되어 모든 디테일 패널의 클래스 계층 위에서 누적 실행된다.
class FWxCategoryDetailCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};

// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "View/MVVMViewModelContextResolver.h"

#include "WxViewModel_Subtitle.generated.h"

class UUserWidget;
class UMVVMView;

/**
 * 화면 자막 한 줄을 노출하는 범용 뷰모델.
 *
 * 자막은 화면당 하나뿐이라 위젯별로 만들지 않고 MVVM 글로벌 컬렉션(UMVVMGameSubsystem 소유)에 하나만 둔다 — 표시하는 위젯과 문구를 거는 ST 노드가 같은 인스턴스를 찾아가야 하기 때문이다.
 * 값은 외부 소스(퀘스트·장치의 ST 노드 등)가 push 하며, 슬롯이 하나라 나중 요청이 이긴다.
 */
UCLASS()
class WXUI_API UWxViewModel_Subtitle : public UWxViewModel
{
	GENERATED_BODY()

public:
	static UWxViewModel_Subtitle* GetOrCreate(const UObject* WorldContextObject);

	/** 자막을 이 화자·문구로 바꾸고, 나중에 회수할 때 쓸 핸들을 발급한다. */
	int32 ShowSubtitle(const FText& InSpeakerText, const FText& InSubtitleText);

	/** 발급 핸들이 지금 걸린 자막의 것일 때만 화면에서 걷어간다. */
	void HideSubtitle(int32 InSubtitleHandle);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Subtitle")
	bool bHasSubtitle = false;

	/** 화자 없는 나레이션이면 비어 있다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Subtitle")
	FText SpeakerText;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Subtitle")
	FText SubtitleText;

private:
	/** 자막이 없으면 INDEX_NONE. */
	int32 CurrentHandle = INDEX_NONE;

	/** 재사용하지 않으므로 뒤늦은 회수 요청이 엉뚱한 자막을 걷어가지 않는다. */
	int32 NextHandle = 0;
};

UCLASS(EditInlineNew, CollapseCategories)
class WXUI_API UWxViewModelResolver_Subtitle : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};

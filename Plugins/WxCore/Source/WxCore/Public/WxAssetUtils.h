// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

/**
 * 비동기 에셋 로드 유틸리티.
 *
 * TSoftObjectPtr을 받아 이미 로드되어 있으면 즉시 콜백을 호출하고,
 * 미로드 시 비동기 로드 후 콜백을 호출한다.
 * 콜백 대상 객체는 WeakPtr로 안전하게 관리된다.
 *
 * 사용 예:
 *   WxAssetUtils::AsyncLoad(SoftPtr, Receiver, &UMyClass::SetTexture);
 */
namespace WxAssetUtils
{
	/**
	 * TSoftObjectPtr을 비동기 로드하여 멤버 함수에 전달한다.
	 *
	 * @param SoftPtr       로드할 소프트 레퍼런스
	 * @param Receiver      콜백을 받을 UObject
	 * @param Callback      로드된 에셋을 전달받을 멤버 함수 (예: &UMyClass::SetIcon)
	 */
	template<typename AssetType, typename ReceiverType>
	void AsyncLoad(const TSoftObjectPtr<AssetType>& SoftPtr, ReceiverType* Receiver, void (ReceiverType::*Callback)(AssetType*))
	{
		if (SoftPtr.IsNull() || !Receiver)
		{
			return;
		}

		if (AssetType* LoadedAsset = SoftPtr.Get())
		{
			(Receiver->*Callback)(LoadedAsset);
			return;
		}

		TWeakObjectPtr<ReceiverType> WeakReceiver = Receiver;
		TSoftObjectPtr<AssetType> SoftPtrCopy = SoftPtr;

		UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(
			SoftPtrCopy.ToSoftObjectPath(),
			FStreamableDelegate::CreateLambda([WeakReceiver, SoftPtrCopy, Callback]()
			{
				if (ReceiverType* Recv = WeakReceiver.Get())
				{
					(Recv->*Callback)(SoftPtrCopy.Get());
				}
			})
		);
	}
}

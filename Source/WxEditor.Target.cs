// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class WxEditorTarget : TargetRules
{
	public WxEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("WxGame");
	}
}

#include "CommandsHelpers.h"

FIntPoint2DJson::FIntPoint2DJson()
{
}

FIntPoint2DJson::FIntPoint2DJson(const FIntPoint IntPoint) : x(IntPoint.X), y(IntPoint.Y)
{
}
 
FIntPoint2DJson::FIntPoint2DJson(int32 x, int32 y)	: x(x), y(y)
{
}

FRotatorJson::FRotatorJson()
{
	
}

FRotatorJson::FRotatorJson(const FRotator& Rotator)
	: Pitch(Rotator.Pitch), Yaw(Rotator.Yaw), Roll(Rotator.Roll)
{
	
}

FRotatorJson::FRotatorJson(FRotator&& Rotator)
	: Pitch(Rotator.Pitch), Yaw(Rotator.Yaw), Roll(Rotator.Roll)
{
	
}

FVectorJson::FVectorJson()
{
}

FVectorJson::FVectorJson(const FVector Vec) : x(Vec.X), y(Vec.Y), z(Vec.Z)
{
}

FVectorJson::FVectorJson(float x, float y, float z)
	: x(x), y(y), z(z)
{
}

FTransformJson::FTransformJson()
{
}

FTransformJson::FTransformJson(const FTransform& Transform)
	: rotation(Transform.Rotator()), translation(Transform.GetTranslation()), Scale3D(Transform.GetScale3D())
{
	
}

FTransformJson::FTransformJson(FTransform&& Transform)
: rotation(Transform.Rotator()), translation(Transform.GetTranslation()), Scale3D(Transform.GetScale3D())
{
	
}

FLinearColorJson::FLinearColorJson()
{
}

FLinearColorJson::FLinearColorJson(const FLinearColor Color) : r(Color.R), g(Color.G), b(Color.B), a(Color.A)
{
}

FLinearColorJson::FLinearColorJson(float r, float g, float b, float a) : r(r), g(g), b(b), a(a)
{
}

//Payloads

FPayloadJson::FPayloadJson()
{
	switch (FEngineVersion::Current().GetMajor())
	{
	case 4: unrealVersion = "V_4"; break;
	case 5: unrealVersion = "V_5"; break;
	default: break;
	}
}

FSelectProductPayloadJson::FSelectProductPayloadJson()
{
	type = EAssetType::PRODUCT;
}

FLoadLevelPayloadJson::FLoadLevelPayloadJson()
{
	type = EAssetType::ENVIRONMENT;
}

FSetMaterialPayloadJson::FSetMaterialPayloadJson()
{
	type = EAssetType::MATERIAL;
}

// ----------------------- LIGHT --------------------------------
#pragma region LightRegion

FAddPanelLightPayloadJson::FAddPanelLightPayloadJson()
{
	type = EAssetType::LIGHT;
}

FAddRectLightPayloadJson::FAddRectLightPayloadJson()
{
	type = EAssetType::LIGHT;
}

FUpdatePanelLightPayloadJson::FUpdatePanelLightPayloadJson()
{
	type = EAssetType::LIGHT;
}

FUpdateRectLightPayloadJson::FUpdateRectLightPayloadJson()
{
	type = EAssetType::LIGHT;
}

FSet3PointLightPayloadJson::FSet3PointLightPayloadJson()
{
	type = EAssetType::LIGHT;
}

FSetHdrPayloadJson::FSetHdrPayloadJson()
{
	type = EAssetType::LIGHT;
}

FSetSunOrientationPayloadJson::FSetSunOrientationPayloadJson()
{
	type = EAssetType::LIGHT;
}

FTimeUDSPayloadJson::FTimeUDSPayloadJson()
{
	type = EAssetType::LIGHT;
}

FEnableUDSPayloadJson::FEnableUDSPayloadJson()
{
	type = EAssetType::LIGHT;
}

#pragma endregion

// ----------------------- SEQUENCE --------------------------------
#pragma region SequenceRegion

FSettingsSequencePayloadJson::FSettingsSequencePayloadJson()
{
	type = EAssetType::LEVEL_SEQUENCE;
}

FInitSequencePayloadJson::FInitSequencePayloadJson()
{
	type = EAssetType::LEVEL_SEQUENCE;
}

FPlaySequencePayloadJson::FPlaySequencePayloadJson()
{
	type = EAssetType::LEVEL_SEQUENCE;
}

FJumpToSequencePayloadJson::FJumpToSequencePayloadJson()
{
	type = EAssetType::LEVEL_SEQUENCE;
}

FDragSequencePayloadJson::FDragSequencePayloadJson()
{
	type = EAssetType::LEVEL_SEQUENCE;
}

#pragma endregion

// ----------------------- CAMERA --------------------------------
#pragma region CameraRegion

FCameraSettingsJson::FCameraSettingsJson()
{
}

FCameraClampsJson::FCameraClampsJson()
{
}

FCameraTransformJson::FCameraTransformJson()
{
}

FApplyCameraPresetPayloadJson::FApplyCameraPresetPayloadJson()
{
	type = EAssetType::CAMERA;
}

#pragma endregion


// ----------------------- SCREENSHOT --------------------------------
#pragma region ScreenshotRegion

FScreenshotPayloadJson::FScreenshotPayloadJson()
{
	type = EAssetType::CAMERA;
}

FEnable360PayloadJson::FEnable360PayloadJson()
{
	type = EAssetType::CAMERA;
}

F360VerticalRotatePayloadJson::F360VerticalRotatePayloadJson()
{
	type = EAssetType::CAMERA;
}

#pragma endregion


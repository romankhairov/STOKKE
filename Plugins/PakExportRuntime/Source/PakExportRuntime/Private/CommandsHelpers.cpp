#include "CommandsHelpers.h"

FIntPoint2DJson::FIntPoint2DJson()
{
}

FIntPoint2DJson::FIntPoint2DJson(const FIntPoint& IntPoint) : x(IntPoint.X), y(IntPoint.Y)
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

FLinearColorJson::FLinearColorJson(const FLinearColor& Color) : r(Color.R), g(Color.G), b(Color.B), a(Color.A)
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
	type = EAssetType::product;
}

FLoadLevelPayloadJson::FLoadLevelPayloadJson()
{
	type = EAssetType::environment;
}

FSetMaterialPayloadJson::FSetMaterialPayloadJson()
{
	type = EAssetType::material;
}

// ----------------------- LIGHT --------------------------------
#pragma region LightRegion

FLightPayload::FLightPayload()
{
	type = EAssetType::light;
}

FSet3PointLightPayloadJson::FSet3PointLightPayloadJson()
{
	type = EAssetType::light;
}

FSetHdrPayloadJson::FSetHdrPayloadJson()
{
	type = EAssetType::light;
}

FEnableUDSPayloadJson::FEnableUDSPayloadJson()
{
	type = EAssetType::light;
}

#pragma endregion

// ----------------------- SEQUENCE --------------------------------
#pragma region SequenceRegion

FSettingsSequencePayloadJson::FSettingsSequencePayloadJson()
{
	type = EAssetType::level_sequence;
}

FInitSequencePayloadJson::FInitSequencePayloadJson()
{
	type = EAssetType::level_sequence;
}

FPlaySequencePayloadJson::FPlaySequencePayloadJson()
{
	type = EAssetType::level_sequence;
}

FJumpToSequencePayloadJson::FJumpToSequencePayloadJson()
{
	type = EAssetType::level_sequence;
}

FDragSequencePayloadJson::FDragSequencePayloadJson()
{
	type = EAssetType::level_sequence;
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

FApplyCameraPresetPayloadJson::FApplyCameraPresetPayloadJson()
{
	type = EAssetType::camera;
}

#pragma endregion


// ----------------------- SCREENSHOT --------------------------------
#pragma region ScreenshotRegion

FScreenshotPayloadJson::FScreenshotPayloadJson()
{
	type = EAssetType::camera;
}

FEnable360PayloadJson::FEnable360PayloadJson()
{
	type = EAssetType::camera;
}

F360VerticalRotatePayloadJson::F360VerticalRotatePayloadJson()
{
	type = EAssetType::camera;
}

#pragma endregion


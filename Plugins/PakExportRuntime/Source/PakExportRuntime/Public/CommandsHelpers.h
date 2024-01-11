#pragma once

#include "CoreMinimal.h"
#include "CommandsHelpers.generated.h"

///A vector in 2-D space composed of components (X, Y) with floating point precision.
USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FVector2DJson
{
	GENERATED_BODY()
public:
	///Vector's X component.
	UPROPERTY() float x = 0.0f;

	///Vector's Y component.
	UPROPERTY() float y = 0.0f;
};

///Structure for integer points in 2-d space.
USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FIntPoint2DJson
{
	GENERATED_BODY()
public:
	FIntPoint2DJson();
	FIntPoint2DJson(const FIntPoint IntPoint);
	FIntPoint2DJson(int32 x, int32 y);

	///Holds the point's X-coordinate.
	UPROPERTY() int32 x = 0;

	///Holds the point's Y-coordinate.
	UPROPERTY() int32 y = 0;
};

///Implements a container for rotation information.
///All rotation values are stored in degrees.
USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FRotatorJson
{
	GENERATED_BODY()
public:
	FRotatorJson();
	FRotatorJson(const FRotator& Rotator);
	FRotatorJson(FRotator&& Rotator);
	
	///Rotation around the right axis (around Y axis), Looking up and down (0=Straight Ahead, +Up, -Down)
	UPROPERTY() float Pitch = 0.0f;

	///Rotation around the up axis (around Z axis), Running in circles 0=East, +North, -South.
	UPROPERTY() float Yaw = 0.0f;

	///Rotation around the forward axis (around X axis), Tilting your head, 0=Straight, +Clockwise, -CCW.
	UPROPERTY() float Roll = 0.0f;
};

///A vector in 3-D space composed of components (X, Y, Z) with floating point precision.
USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FVectorJson
{
	GENERATED_BODY()
public:
	FVectorJson();
	FVectorJson(const FVector Vec);
	FVectorJson(float x, float y, float z);
	
	///Vector's X component.
	UPROPERTY() float x = 0.0f;

	///Vector's Y component.
	UPROPERTY() float y = 0.0f;

	///Vector's Z component.
	UPROPERTY() float z = 0.0f;
};

///Transform composed of Scale, Rotation (as a rotator), and Translation.
USTRUCT()
struct PAKEXPORTRUNTIME_API FTransformJson
{
	GENERATED_BODY()
public:
	FTransformJson();
	FTransformJson(const FTransform& Transform);
	FTransformJson(FTransform&& Transform);
	
	///Rotation of this transformation, as a rotator
	UPROPERTY() FRotatorJson rotation;
	
	///Translation of this transformation, as a vector
	UPROPERTY() FVectorJson translation;

	///3D scale (always applied in local space) as a vector
	UPROPERTY() FVectorJson Scale3D = FVectorJson(1.0f, 1.0f, 1.0f);
};

//Scalar material parameter
USTRUCT()
struct PAKEXPORTRUNTIME_API FScalarJson
{
	GENERATED_BODY()
public:
	///scalar parameter name in material
	UPROPERTY() FString parameterName = "print_switch";

	///value of the scalar material parameter
	UPROPERTY(meta = (ClampMin = "0.0", ClampMax = "1.0")) float value = 1.0f;
};

//LinearColor
USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FLinearColorJson
{
	GENERATED_BODY()
public:
	FLinearColorJson();
	FLinearColorJson(const FLinearColor Color);
	FLinearColorJson(float r, float g, float b, float a);

	///Color's Red component.
	UPROPERTY(BlueprintReadWrite, Category = Color) float r = 1.0f;

	///Color's Green component.
	UPROPERTY(BlueprintReadWrite, Category = Color) float g = 1.0f;

	///Color's Blue component.
	UPROPERTY(BlueprintReadWrite, Category = Color) float b = 1.0f;

	///Color's Alpha component.
	UPROPERTY(BlueprintReadWrite, Category = Color) float a = 1.0f;
};

///Material texture
USTRUCT()
struct PAKEXPORTRUNTIME_API FTextureJson
{
	GENERATED_BODY()
public:
	///parameter for texture in material
	UPROPERTY() FString parameterName = "print";

	///url to download texture from
	UPROPERTY() FString url = "https://i.ibb.co/tsjtsDf/Pngtree-duck-202201.png";

	///enable canvas for texture or no
	UPROPERTY() bool canvas = true;

	///size of the texture
	UPROPERTY() FVector2DJson size = {1024, 1024};
	
	///position of the texture in relative space
	UPROPERTY() FVector2DJson position = {0.5, 0.5};

	///rotate angle in degrees
	UPROPERTY() float Rotate = 0;

	///pivot of the texture in relative space
	UPROPERTY() FVector2DJson pivot = {0.5, 0.5};
	
	///grid position
	UPROPERTY() FVector2DJson coordinatePosition = {0, 0};

	///grid size
	UPROPERTY() FVector2DJson coordinateSize = {1, 1};
};

///Material
USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FMaterialJson
{
	GENERATED_BODY()
public:
	///Name of the asset root folder(almost always like this)
	UPROPERTY() FString pak = "PakE";

	///Name of the asset relative to Content folder in project
	UPROPERTY() FString name = "";

	///Material name applying material to
	UPROPERTY() FString slot_name = "";

	///vector parameter color applying to
	UPROPERTY() FString colorParameterName = "";
	
	///color in hex format like FF0000 of #FF0000
	UPROPERTY() FString colorHex = "";

	///List of textures in the material
	UPROPERTY() TArray<FTextureJson> textures;

	///List of scalars in the material
	UPROPERTY() TArray<FScalarJson> scalars;

	///Relative path to material pak file
	UPROPERTY() FString pakFilePath = "";
};

///Mesh initial state
USTRUCT()
struct PAKEXPORTRUNTIME_API FInitialStateJson
{
	GENERATED_BODY()
public:
	///Name of the asset relative to Content folder in project
	UPROPERTY() FString meshName = "";

	///Product spawn transform
	UPROPERTY() FTransformJson transform;

	///list of materials applying to product
	UPROPERTY() TArray<FMaterialJson> materials;

	///Product animations
	UPROPERTY() TArray<FString> animations;

	///Materials slots
	UPROPERTY() TArray<FString> slots;

	///Founded in product morphs
	UPROPERTY() TArray<FString> morphs;
};

///Object describes path to pak/use file
USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FPakJson
{
	GENERATED_BODY()
public:
	///relative path to pak file
	UPROPERTY() FString pakFilePath = "";

	///Name of the asset root folder(almost always like this)
	UPROPERTY() FString name = "PakE";

	///Name of the pak export plugin (almost always like this)
	UPROPERTY() FString mountPath = "../../../PakExport/Plugins/";

	///path to usd/usdz file overrides pak file path
	UPROPERTY() FString usd_path = "";
};

///Mesh pak file
USTRUCT()
struct PAKEXPORTRUNTIME_API FMeshPakJson : public FPakJson
{
	GENERATED_BODY()
public:
	///Mesh initial state
	UPROPERTY() FInitialStateJson initialState;
};

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FAssetPakJson : public FPakJson
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, Category=PakExportRuntime) FString assetName;
};

UENUM(BlueprintType)
enum class ELightMode : uint8
{
	NONE		UMETA(DisplayName = "None"),
	RECT		UMETA(DisplayName = "Rectangle"),
	PANEL		UMETA(DisplayName = "Panel")
};

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FLightJson
{
	GENERATED_BODY()
public:
	///Location of light
	UPROPERTY(BlueprintReadWrite, Category = Light) FVectorJson location;
	///Rotation of light
	UPROPERTY(BlueprintReadWrite, Category = Light) FRotatorJson rotation;
	///SubLocation of light
	UPROPERTY(BlueprintReadWrite, Category = Light) FVectorJson subLocation;
	///SubRotation of light
	UPROPERTY(BlueprintReadWrite, Category = Light) FRotatorJson subRocation;
	///Size
	UPROPERTY(BlueprintReadWrite, Category = Light) FVector2DJson size;
	///ArmLentgh
	UPROPERTY(BlueprintReadWrite, Category = Light) float armLength = 0.0f;
	///Intensity
	UPROPERTY(BlueprintReadWrite, Category = Light) float intensity = 0.0f;
	///Kelvin
	UPROPERTY(BlueprintReadWrite, Category = Light) float kelvin = 0.0f;
	///ColorTint
	UPROPERTY(BlueprintReadWrite, Category = Light) FLinearColorJson colorTint;
	///Color
	UPROPERTY(BlueprintReadWrite, Category = Light) FLinearColorJson color;
	///Type
	UPROPERTY(BlueprintReadWrite, Category = Light) FString type;
	///LightMode
	UPROPERTY(BlueprintReadWrite, Category = Light) ELightMode lightMode;
	///Pitch
	UPROPERTY(BlueprintReadWrite, Category = Light) float pitch = 0.0f;
	///Yaw
	UPROPERTY(BlueprintReadWrite, Category = Light) float yaw = 0.0f;
	///Temp
	UPROPERTY(BlueprintReadWrite, Category = Light) float temp = 0.0f;
	///TODO rename to EnableGizmo
	UPROPERTY(BlueprintReadWrite, Category = Light) bool disableGizmo = false;

};



//------------------------ Payloads ---------------------------

UENUM()
enum class EAssetType : uint8
{
	NONE,
	PRODUCT,
	MATERIAL,
	ENVIRONMENT,
	CAMERA,
	LEVEL_SEQUENCE,
	SCREENSHOT,
	LIGHT
};

USTRUCT()
struct PAKEXPORTRUNTIME_API FPayloadJson
{
	GENERATED_BODY()
public:
	FPayloadJson();
	
protected:
	UPROPERTY() EAssetType type = EAssetType::NONE;
	UPROPERTY() FString unrealVersion;
};

///Select product command payload
USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FSelectProductPayloadJson : public FPayloadJson
{
	GENERATED_BODY()
public:
	FSelectProductPayloadJson();
	
	///Global(static) product ID in scene (not per instance)
	UPROPERTY() FString productName;

	///Mesh pak file
	UPROPERTY() FMeshPakJson meshPak;

	///Material pak file
	UPROPERTY() FPakJson materialPak;

	///Animations pak file
	UPROPERTY() FPakJson animationsPak;

	///asset ID from front to store
	UPROPERTY() FString assetId;
};

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FLoadLevelPayloadJson : public FPayloadJson
{
	GENERATED_BODY()
public:
	FLoadLevelPayloadJson();
	
	UPROPERTY(BlueprintReadWrite, Category=PakExportRuntime) FString levelName;
	UPROPERTY(BlueprintReadWrite, Category=PakExportRuntime) bool hideAllLevels = false;
	UPROPERTY(BlueprintReadWrite, Category=PakExportRuntime) bool hideLastLevel = false;
	UPROPERTY(BlueprintReadWrite, Category=PakExportRuntime) bool clickable = false;
	UPROPERTY(BlueprintReadWrite, Category=PakExportRuntime) TArray<FString> levelsToHide;
	UPROPERTY(BlueprintReadWrite, Category=PakExportRuntime) FVectorJson location;
	UPROPERTY(BlueprintReadWrite, Category=PakExportRuntime) FRotatorJson rotation;
	UPROPERTY(BlueprintReadWrite, Category=PakExportRuntime) FString optionalLevelName;
	UPROPERTY(BlueprintReadWrite, Category=PakExportRuntime) TArray<FString> levelType;
	UPROPERTY(BlueprintReadWrite, Category=PakExportRuntime) FAssetPakJson levelPak;
	UPROPERTY(BlueprintReadWrite, Category=PakExportRuntime) TArray<FString> slots;
};

USTRUCT()
struct PAKEXPORTRUNTIME_API FSetMaterialPayloadJson : public FPayloadJson
{
	GENERATED_BODY()
public:
	FSetMaterialPayloadJson();
	
	UPROPERTY() FString materialName;
	UPROPERTY() FAssetPakJson materialPak;
};

// ----------------------- LIGHT --------------------------------
#pragma region LightRegion

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FAddPanelLightPayloadJson : public FPayloadJson
{
	GENERATED_BODY()
public:
	FAddPanelLightPayloadJson();

	///light settings
	UPROPERTY(BlueprintReadWrite, Category = Light) FLightJson light;

	///set current ?
	UPROPERTY(BlueprintReadWrite, Category = Light) bool setCurrent = false;
};

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FAddRectLightPayloadJson : public FPayloadJson
{
	GENERATED_BODY()
public:
	FAddRectLightPayloadJson();

	///light settings
	UPROPERTY(BlueprintReadWrite, Category = Light) FLightJson light;

	///set current ?
	UPROPERTY(BlueprintReadWrite, Category = Light) bool setCurrent = false;
};

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FUpdatePanelLightPayloadJson : public FPayloadJson
{
	GENERATED_BODY()
public:
	FUpdatePanelLightPayloadJson();

	///light settings
	UPROPERTY(BlueprintReadWrite, Category = Light) FLightJson light;

	UPROPERTY(BlueprintReadWrite, Category = Light) FString objectName;
};

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FUpdateRectLightPayloadJson : public FPayloadJson
{
	GENERATED_BODY()
public:
	FUpdateRectLightPayloadJson();

	///light settings
	UPROPERTY(BlueprintReadWrite, Category = Light) FLightJson light;

	UPROPERTY(BlueprintReadWrite, Category = Light) FString objectName;
};

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FSet3PointLightPayloadJson : public FPayloadJson
{
	GENERATED_BODY()
public:
	FSet3PointLightPayloadJson();

	///ColorTint
	UPROPERTY(BlueprintReadWrite, Category = Light) FLinearColorJson colorTint;
	///Intensity
	UPROPERTY(BlueprintReadWrite, Category = Light) float intensity;
	///TODO  wtf is this?
	UPROPERTY(BlueprintReadWrite, Category = Light) float dist;
	///TODO  wtf is this?
	UPROPERTY(BlueprintReadWrite, Category = Light) float angle;
	///TODO  wtf is this?
	UPROPERTY(BlueprintReadWrite, Category = Light) float angleZ;

};

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FSetHdrPayloadJson : public FPayloadJson
{
	GENERATED_BODY()
public:
	FSetHdrPayloadJson();

	///ColorTint
	UPROPERTY(BlueprintReadWrite, Category = Light) FString imageUrl;
	///Intensity
	UPROPERTY(BlueprintReadWrite, Category = Light) float intensity;
	///TODO  wtf is this?
	UPROPERTY(BlueprintReadWrite, Category = Light) float rotate;

};

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FSetSunOrientationPayloadJson : public FPayloadJson
{
	GENERATED_BODY()
public:
	FSetSunOrientationPayloadJson();

	///ColorTint
	UPROPERTY(BlueprintReadWrite, Category = Light) FString omageUrl;
	///Intensity
	UPROPERTY(BlueprintReadWrite, Category = Light) float northYaw;
	///
	UPROPERTY(BlueprintReadWrite, Category = Light) float longitude;
	///
	UPROPERTY(BlueprintReadWrite, Category = Light) float latitude;

};

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FTimeUDSPayloadJson : public FPayloadJson
{
	GENERATED_BODY()
public:
	FTimeUDSPayloadJson();
	///TODO WTF rename
	UPROPERTY(BlueprintReadWrite, Category = Light) float val;
};

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FEnableUDSPayloadJson : public FPayloadJson
{
	GENERATED_BODY()
public:
	FEnableUDSPayloadJson();
	///TODO WTF rename
	UPROPERTY(BlueprintReadWrite, Category = Light) bool val;
};

#pragma endregion

// ----------------------- SEQUENCE --------------------------------
#pragma region SequenceRegion

UENUM(BlueprintType)
enum class ESequenceLoopBehavior : uint8
{
	NONE						UMETA(DisplayName = "None"),
	LOOP						UMETA(DisplayName = "Loop"),
	PINGPONG					UMETA(DisplayName = "PingPong"),
	LOOPSECTION					UMETA(DisplayName = "LoopSection"),
	PINGPONGSECTION				UMETA(DisplayName = "PingPongSection")
};

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FSettingsSequencePayloadJson : public FPayloadJson
{
	GENERATED_BODY()
public:
	FSettingsSequencePayloadJson();

	///Loop behavior of sequence playing
	UPROPERTY(BlueprintReadWrite, Category = Sequence) ESequenceLoopBehavior loopBehavior;

	///Rate-speed of sequence playing, TODO make play rate could be negative and remove bool reverse
	UPROPERTY(BlueprintReadWrite, Category = Sequence) float playRate;

	///Is reverse playing of sequence
	UPROPERTY(BlueprintReadWrite, Category = Sequence) bool reverse;

	/// Time period of responding changes for callbacks
	UPROPERTY(BlueprintReadWrite, Category = Sequence) float timeResponseChangeCallBack;
};

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FInitSequencePayloadJson : public FSettingsSequencePayloadJson
{
	GENERATED_BODY()
public:
	FInitSequencePayloadJson();

	UPROPERTY(BlueprintReadWrite, Category= Sequence) TArray<FAssetPakJson> paks;
};

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FPlaySequencePayloadJson : public FPayloadJson
{
	GENERATED_BODY()
public:
	FPlaySequencePayloadJson();

	///Rate-speed of sequence playing, TODO make play rate could be negative and remove bool reverse
	UPROPERTY(BlueprintReadWrite, Category = Sequence) float playRate;

	///Is reverse playing of sequence
	UPROPERTY(BlueprintReadWrite, Category = Sequence) bool reverse;
};

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FJumpToSequencePayloadJson : public FPayloadJson
{
	GENERATED_BODY()
public:
	FJumpToSequencePayloadJson();

	///Section id to jump
	UPROPERTY(BlueprintReadWrite, Category = PakExportRuntime) int32 id;
};

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FDragSequencePayloadJson : public FPayloadJson
{
	GENERATED_BODY()
public:
	FDragSequencePayloadJson();

	///Global time of sequence to drag
	UPROPERTY(BlueprintReadWrite, Category = PakExportRuntime) float time;
};

#pragma endregion

// ----------------------- CAMERA --------------------------------
#pragma region CameraRegion

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FCameraSettingsJson
{
	GENERATED_BODY()
public:
	FCameraSettingsJson();

	UPROPERTY(BlueprintReadWrite, Category = Camera) float armLength = 0;
	UPROPERTY(BlueprintReadWrite, Category = Camera) float focalLength = 0;
	UPROPERTY(BlueprintReadWrite, Category = Camera) float aperture = 0;
	UPROPERTY(BlueprintReadWrite, Category = Camera) float focusOffset = 0;
};


//TODO is it camera clamps or control clamps?
USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FCameraClampsJson
{
	GENERATED_BODY()
public:
	FCameraClampsJson();

	UPROPERTY(BlueprintReadWrite, Category = Camera) float minZoom = 0;
	UPROPERTY(BlueprintReadWrite, Category = Camera) float maxZoom = 0;
	UPROPERTY(BlueprintReadWrite, Category = Camera) float minPitch = 0;
	UPROPERTY(BlueprintReadWrite, Category = Camera) float maxPicth = 0;
	UPROPERTY(BlueprintReadWrite, Category = Camera) float minYaw = 0;
	UPROPERTY(BlueprintReadWrite, Category = Camera) float maxYaw = 0;
};

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FCameraTransformJson
{
	GENERATED_BODY()
public:
	FCameraTransformJson();

	UPROPERTY(BlueprintReadWrite, Category = Camera) FVectorJson location;
	UPROPERTY(BlueprintReadWrite, Category = Camera) FRotatorJson rotation;
};

//TODO split it to separated structs
USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FApplyCameraPresetPayloadJson : public FPayloadJson
{
	GENERATED_BODY()
public:
	FApplyCameraPresetPayloadJson();
	
	UPROPERTY(BlueprintReadWrite, Category=PakExportRuntime) FVectorJson location;
	UPROPERTY(BlueprintReadWrite, Category=PakExportRuntime) FRotatorJson rotation;
	UPROPERTY(BlueprintReadWrite, Category=PakExportRuntime) FString object;
	UPROPERTY(BlueprintReadWrite, Category=PakExportRuntime) float armLength = 0;
	UPROPERTY(BlueprintReadWrite, Category=PakExportRuntime) float focalLength = 0;
	UPROPERTY(BlueprintReadWrite, Category=PakExportRuntime) float aperture = 0;
	UPROPERTY(BlueprintReadWrite, Category=PakExportRuntime) float focusOffset = 0;
};

#pragma endregion

// ----------------------- SCREENSHOT --------------------------------
#pragma region ScreenshotRegion

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FScreenshotPayloadJson : public FPayloadJson
{
	GENERATED_BODY()
public:
	FScreenshotPayloadJson();

	UPROPERTY(BlueprintReadWrite, Category = Screenshot) FIntPoint2DJson size = { 0,0 };
	UPROPERTY(BlueprintReadWrite, Category = Screenshot) int32 width;
	UPROPERTY(BlueprintReadWrite, Category = Screenshot) int32 height;
	UPROPERTY(BlueprintReadWrite, Category = Screenshot) FString name;
	UPROPERTY(BlueprintReadWrite, Category = Screenshot) FString format;
	UPROPERTY(BlueprintReadWrite, Category = Screenshot) FString url;
	UPROPERTY(BlueprintReadWrite, Category = Screenshot) FString fs;
	UPROPERTY(BlueprintReadWrite, Category = Screenshot) FString preset;
	UPROPERTY(BlueprintReadWrite, Category = Screenshot) FString pitchCurve;
	UPROPERTY(BlueprintReadWrite, Category = Screenshot) int32 samples;
	UPROPERTY(BlueprintReadWrite, Category = Screenshot) int32 frames;
	UPROPERTY(BlueprintReadWrite, Category = Screenshot) bool crop = false;
	UPROPERTY(BlueprintReadWrite, Category = Screenshot) bool transparentBackground = false;
	UPROPERTY(BlueprintReadWrite, Category = Screenshot) bool sendByWebRTC = false;
	UPROPERTY(BlueprintReadWrite, Category = Screenshot) bool pathTrace = false;
	UPROPERTY(BlueprintReadWrite, Category = Screenshot) bool preview = false;
};

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API FEnable360PayloadJson : public FPayloadJson
{
	GENERATED_BODY()
public:
	FEnable360PayloadJson();

	UPROPERTY(BlueprintReadWrite, Category = Screenshot) float armLength;
	UPROPERTY(BlueprintReadWrite, Category = Screenshot) bool enable = false;
};

USTRUCT(BlueprintType)
struct PAKEXPORTRUNTIME_API F360VerticalRotatePayloadJson : public FPayloadJson
{
	GENERATED_BODY()
public:
	F360VerticalRotatePayloadJson();

	UPROPERTY(BlueprintReadWrite, Category = Screenshot) float angle;
};

#pragma endregion

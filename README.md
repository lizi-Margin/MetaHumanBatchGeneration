# MetaHuman Parametric Plugin

A comprehensive Unreal Engine 5.6 plugin for programmatically creating and customizing MetaHuman characters using the built-in MetaHuman Creator API.

## Features

- **Parametric Body Control**: Precisely control body measurements (Height, Chest, Waist, Hips, etc.)
- **18 Body Type Presets**: Support for all MetaHuman body types (gender, height, weight combinations)
- **Appearance Customization**: Configure skin tone, eye color, iris patterns, eyelashes
- **Blueprint Export**: Automatically export generated characters to Blueprint assets
- **Batch Creation**: Create multiple characters with different configurations
- **C++ and Blueprint Support**: Full UFUNCTION exposure for Blueprint integration

## Prerequisites

- **Unreal Engine 5.6** or later
- **MetaHuman Plugins** must be enabled in your project:
  - MetaHumanCharacter
  - MetaHumanCharacterEditor
  - MetaHumanSDK
  - MetaHumanCoreTechLib
  - MetaHumanDefaultPipeline

## Installation

### Method 1: Project Plugin (Recommended)

1. Copy the entire `MetaHumanParametricPlugin` folder to your UE project's `Plugins` directory:
   ```
   YourProject/
   ├── Content/
   ├── Plugins/
   │   └── MetaHumanParametricPlugin/    <-- Place here
   ├── YourProject.uproject
   ```

2. Right-click your `.uproject` file and select **"Generate Visual Studio project files"**

3. Open the generated `.sln` file in Visual Studio

4. Build the project (Development Editor configuration)

5. Launch the editor - the plugin will be automatically loaded

### Method 2: Engine Plugin

1. Copy `MetaHumanParametricPlugin` to your UE 5.6 engine's plugin directory:
   ```
   I:/UE_5.6/Engine/Plugins/MetaHumanParametricPlugin/
   ```

2. The plugin will be available to all projects using this engine installation

## Usage

### Example 1: Create a Slender Female Character (C++)

```cpp
#include "MetaHumanParametricGenerator.h"

void CreateMyCharacter()
{
    // Configure body parameters
    FMetaHumanBodyParametricConfig BodyConfig;
    BodyConfig.BodyType = EMetaHumanBodyType::f_med_nrw;  // Female, Medium height, Normal weight
    BodyConfig.bUseParametricBody = true;
    BodyConfig.GlobalDeltaScale = 1.0f;

    // Set precise body measurements (in centimeters)
    BodyConfig.BodyMeasurements.Empty();
    BodyConfig.BodyMeasurements.Add(TEXT("Height"), 168.0f);
    BodyConfig.BodyMeasurements.Add(TEXT("Chest"), 82.0f);
    BodyConfig.BodyMeasurements.Add(TEXT("Waist"), 62.0f);
    BodyConfig.BodyMeasurements.Add(TEXT("Hips"), 88.0f);

    // Configure appearance
    FMetaHumanAppearanceConfig AppearanceConfig;
    AppearanceConfig.SkinToneU = 0.4f;  // Fair skin
    AppearanceConfig.SkinToneV = 0.5f;
    AppearanceConfig.IrisPattern = EMetaHumanCharacterEyesIrisPattern::Iris002;
    AppearanceConfig.IrisPrimaryColorU = 0.25f;  // Brown eyes
    AppearanceConfig.IrisPrimaryColorV = 0.35f;

    // Generate character
    UMetaHumanCharacter* Character = nullptr;
    bool bSuccess = UMetaHumanParametricGenerator::GenerateParametricMetaHuman(
        TEXT("MySlenderFemale"),
        TEXT("/Game/MyCharacters/"),
        BodyConfig,
        AppearanceConfig,
        Character
    );

    if (bSuccess && Character)
    {
        // Export to Blueprint
        UBlueprint* Blueprint = UMetaHumanParametricGenerator::ExportCharacterToBlueprint(
            Character,
            TEXT("/Game/MyCharacters/Blueprints/"),
            TEXT("BP_MySlenderFemale")
        );
    }
}
```

### Example 2: Batch Create Multiple Characters

```cpp
// See MetaHumanParametricGenerator_Examples.cpp for complete batch creation example
void Example4_BatchCreateCharacters();
```

### Example 3: Using from Blueprint

1. Create a new Blueprint (any type)
2. Call the **GenerateParametricMetaHuman** node
3. Configure the Body Config and Appearance Config structures
4. Connect the execution flow

## Body Type Reference

The plugin supports 18 MetaHuman body type presets:

### Naming Convention: `[gender]_[height]_[weight]`

| Code | Gender | Height | Weight |
|------|--------|--------|--------|
| `f_med_nrw` | Female | Medium | Normal |
| `f_med_ovw` | Female | Medium | Overweight |
| `f_med_unw` | Female | Medium | Underweight |
| `f_srt_nrw` | Female | Short | Normal |
| `f_srt_ovw` | Female | Short | Overweight |
| `f_srt_unw` | Female | Short | Underweight |
| `f_tal_nrw` | Female | Tall | Normal |
| `f_tal_ovw` | Female | Tall | Overweight |
| `f_tal_unw` | Female | Tall | Underweight |
| `m_med_nrw` | Male | Medium | Normal |
| `m_med_ovw` | Male | Medium | Overweight |
| `m_med_unw` | Male | Medium | Underweight |
| `m_srt_nrw` | Male | Short | Normal |
| `m_srt_ovw` | Male | Short | Overweight |
| `m_srt_unw` | Male | Short | Underweight |
| `m_tal_nrw` | Male | Tall | Normal |
| `m_tal_ovw` | Male | Tall | Overweight |
| `m_tal_unw` | Male | Tall | Underweight |

## Body Measurement Parameters

Available measurement constraints (all in centimeters):

- **Height** - Total character height
- **Chest** - Chest circumference
- **Waist** - Waist circumference
- **Hips** - Hip circumference
- **ShoulderWidth** - Distance between shoulders
- **ArmLength** - Arm length from shoulder to wrist
- **LegLength** - Leg length from hip to ankle

**Note**: The constraint system is measurement-based, not slider-based. You specify target measurements in centimeters rather than abstract "fat/muscle" values.

## API Reference

### Core Functions

#### `GenerateParametricMetaHuman`

```cpp
static bool GenerateParametricMetaHuman(
    const FString& CharacterName,           // Character asset name
    const FString& OutputPath,              // Save path (e.g., "/Game/Characters/")
    const FMetaHumanBodyParametricConfig& BodyConfig,
    const FMetaHumanAppearanceConfig& AppearanceConfig,
    UMetaHumanCharacter*& OutCharacter      // Output character asset
);
```

**Returns**: `true` if character was created successfully

**Workflow**:
1. Creates base MetaHumanCharacter asset
2. Configures body type and parametric constraints
3. Applies appearance settings (skin, eyes, eyelashes)
4. Generates character assets (meshes, textures, physics)
5. Saves assets to disk

#### `ExportCharacterToBlueprint`

```cpp
static UBlueprint* ExportCharacterToBlueprint(
    UMetaHumanCharacter* Character,         // Source character
    const FString& BlueprintPath,           // Blueprint save path
    const FString& BlueprintName            // Blueprint name
);
```

**Returns**: Created Blueprint asset or `nullptr` on failure

Creates an Actor Blueprint with FaceMesh and BodyMesh skeletal mesh components.

### Configuration Structures

#### `FMetaHumanBodyParametricConfig`

```cpp
USTRUCT(BlueprintType)
struct FMetaHumanBodyParametricConfig
{
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMetaHumanBodyType BodyType;            // Preset body type

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float GlobalDeltaScale;                 // 0.0 - 1.0, overall deformation strength

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUseParametricBody;                // Use parametric vs fixed body

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, float> BodyMeasurements;  // Measurement constraints (cm)
};
```

#### `FMetaHumanAppearanceConfig`

```cpp
USTRUCT(BlueprintType)
struct FMetaHumanAppearanceConfig
{
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SkinToneU;                        // 0.0 - 1.0, skin color U coordinate

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SkinToneV;                        // 0.0 - 1.0, skin color V coordinate

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SkinRoughness;                    // 0.0 - 2.0, skin roughness

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMetaHumanCharacterEyesIrisPattern IrisPattern;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float IrisPrimaryColorU;                // 0.0 - 1.0, iris color U

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float IrisPrimaryColorV;                // 0.0 - 1.0, iris color V

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMetaHumanCharacterEyelashesType EyelashesType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bEnableEyelashGrooms;
};
```

## Troubleshooting

### Plugin fails to load

**Error**: `Plugin 'MetaHumanParametricPlugin' failed to load because module 'MetaHumanParametricPlugin' could not be found`

**Solution**:
1. Ensure you've regenerated Visual Studio project files after adding the plugin
2. Rebuild the project in Development Editor configuration
3. Check that all MetaHuman plugins are enabled in Edit → Plugins

### Missing MetaHuman dependencies

**Error**: Linker errors referencing MetaHumanCharacter, MetaHumanSDK, etc.

**Solution**:
1. Open Edit → Plugins in your project
2. Enable all MetaHuman-related plugins:
   - MetaHuman Character
   - MetaHuman SDK
   - MetaHuman Core Tech Library
   - MetaHuman Default Pipeline
3. Restart the editor and rebuild

### Character generation fails

**Error**: `Failed to create base character` or `Failed to generate character assets`

**Solution**:
1. Ensure the output path exists or uses a valid UE content path (e.g., `/Game/...`)
2. Check that you're running in the editor (not packaged game)
3. Verify UMetaHumanCharacterEditorSubsystem is available (editor-only)
4. Check the Output Log for detailed error messages

### Platform compatibility

**Note**: This plugin is **Windows 64-bit only** and **Editor-only**. It cannot be used in packaged games as it relies on editor-only APIs.

## Examples

Complete usage examples are provided in:
- `Private/MetaHumanParametricGenerator_Examples.cpp`

Examples include:
1. **Slender Female** - 168cm height, slim waist
2. **Muscular Male** - 185cm height, broad chest
3. **Short Rounded Character** - 155cm height, fuller figure
4. **Batch Creation** - Creating 5 different body types in one function

To run examples, call the functions from your game code or expose them to Blueprint.

## Architecture

### Generation Pipeline

```
1. CreateBaseCharacter()
   └─ Creates UMetaHumanCharacter asset
   └─ Initializes with MetaHumanCharacterEditorSubsystem

2. ConfigureBodyParameters()
   └─ Sets body type (18 presets)
   └─ Applies measurement constraints
   └─ Sets global delta scale
   └─ Commits body state

3. ConfigureAppearance()
   └─ Applies skin settings
   └─ Configures eyes (iris, color)
   └─ Sets eyelash type and grooms

4. GenerateCharacterAssets()
   └─ Generates face skeletal mesh
   └─ Generates body skeletal mesh
   └─ Synthesizes textures
   └─ Creates physics asset
   └─ Creates RigLogic assets

5. SaveCharacterAssets()
   └─ Saves to package
   └─ Registers with AssetRegistry
```

### Key Dependencies

- **UMetaHumanCharacter** - Main character data asset
- **UMetaHumanCharacterEditorSubsystem** - Editor API for character manipulation
- **FMetaHumanCharacterBodyIdentity::FState** - Body parametric state
- **FMetaHumanCharacterBodyConstraint** - Measurement constraint system
- **EMetaHumanBodyType** - 18 body type presets

## Limitations

1. **No direct "Fat/Muscle" sliders** - Uses measurement-based constraint system instead
2. **Editor-only** - Cannot be used in packaged games
3. **Windows 64-bit only** - Platform restriction due to MetaHuman dependencies
4. **Requires MetaHuman plugins** - All MetaHuman plugins must be enabled
5. **Constraint-based customization** - Body shape is controlled through measurements (Height, Chest, Waist, etc.) rather than abstract morphs

## License

Copyright Epic Games, Inc. All Rights Reserved.

This plugin uses Unreal Engine's MetaHuman Creator APIs and is subject to Epic Games' licensing terms.

## Support

For issues, questions, or contributions, please refer to the official Unreal Engine documentation:
- [MetaHuman Creator Documentation](https://docs.unrealengine.com/5.6/en-US/metahuman-creator-in-unreal-engine/)
- [Unreal Engine Plugin Development](https://docs.unrealengine.com/5.6/en-US/plugins-in-unreal-engine/)

## Version History

### v1.0.0 (Initial Release)
- Complete parametric body control API
- 18 body type preset support
- Skin, eyes, eyelash customization
- Blueprint export functionality
- Batch character creation examples
- Full C++ and Blueprint integration

# MetaHuman Parametric Plugin - Usage Guide

## Quick Start

The plugin automatically adds a toolbar menu to the Unreal Editor when loaded.

### Accessing the Generator

1. **Open your Unreal Editor** with the MH_header project
2. **Look for the toolbar** at the top of the editor
3. **Find "MetaHuman Examples"** dropdown menu in the toolbar (should appear in the "User" section)
4. **Click the dropdown** to see 4 example options:

```
MetaHuman Examples ▼
├─ 1. Slender Female
├─ 2. Muscular Male
├─ 3. Short Rounded
└─ 4. Batch Generate (5 Characters)
```

### Running Examples

**Method 1: Using the Toolbar Menu** (Easiest!)

Simply click any of the menu items:

- **1. Slender Female** - Creates a 168cm tall, slim female character (62cm waist)
- **2. Muscular Male** - Creates a 185cm tall, muscular male character (110cm chest)
- **3. Short Rounded** - Creates a 155cm tall character with rounded body
- **4. Batch Generate** - Creates 5 different characters at once:
  - Athletic Female (172cm)
  - Average Male (178cm)
  - Tall Slim Female (180cm)
  - Short Stocky Male (165cm)
  - Petite Female (158cm)

**What Happens:**
1. A notification will appear saying "Generating..."
2. The example function runs (check **Output Log** for progress)
3. Character assets are created in `/Game/MyMetaHumans/`
4. Blueprints are exported to `/Game/MyMetaHumans/Blueprints/`
5. A completion notification appears

### Viewing the Output

**Check the Output Log:**
1. Go to **Window → Developer Tools → Output Log**
2. Look for messages starting with "Example 1:", "Example 2:", etc.
3. You'll see detailed logs about:
   - Character creation progress
   - Body measurements being applied
   - Asset generation status
   - Blueprint export results

**Find Generated Assets:**
1. Open the **Content Browser**
2. Navigate to `/Game/MyMetaHumans/`
3. You'll find:
   - **Females/** - Female characters
   - **Males/** - Male characters
   - **Various/** - Mixed characters
   - **Batch/** - Batch-generated characters
   - **Blueprints/** - Exported Blueprint actors

### Understanding the Generated Characters

Each character includes:
- **MetaHuman Character Asset** (`.uasset`) - The main character definition
- **Skeletal Meshes** - Face and body meshes
- **Textures** - Synthesized face and body textures
- **Physics Asset** - Collision and physics setup
- **Blueprint** (`BP_*.uasset`) - Actor blueprint with face and body components

### Customizing Generation

To create your own characters with custom parameters, you can:

**Option 1: Modify the Example Functions**

Edit `MetaHumanParametricGenerator_Examples.cpp` and change:
- Body type (18 presets available)
- Body measurements (Height, Chest, Waist, Hips, etc.)
- Skin tone (U/V coordinates)
- Eye color and iris pattern
- Eyelash type

**Option 2: Call from Blueprint**

All generator functions are `UFUNCTION(BlueprintCallable)`, so you can:
1. Create a Blueprint
2. Call `GenerateParametricMetaHuman` node
3. Configure the structs in the node
4. Execute the Blueprint

**Option 3: Call from C++**

```cpp
#include "MetaHumanParametricGenerator.h"

// Configure body
FMetaHumanBodyParametricConfig BodyConfig;
BodyConfig.BodyType = EMetaHumanBodyType::f_med_nrw;
BodyConfig.bUseParametricBody = true;
BodyConfig.GlobalDeltaScale = 1.0f;
BodyConfig.BodyMeasurements.Add(TEXT("Height"), 170.0f);
BodyConfig.BodyMeasurements.Add(TEXT("Waist"), 65.0f);

// Configure appearance
FMetaHumanAppearanceConfig AppearanceConfig;
AppearanceConfig.SkinToneU = 0.5f;
AppearanceConfig.SkinToneV = 0.5f;

// Generate
UMetaHumanCharacter* Character = nullptr;
UMetaHumanParametricGenerator::GenerateParametricMetaHuman(
    TEXT("MyCharacter"),
    TEXT("/Game/MyCharacters/"),
    BodyConfig,
    AppearanceConfig,
    Character
);

// Export to Blueprint
UBlueprint* BP = UMetaHumanParametricGenerator::ExportCharacterToBlueprint(
    Character,
    TEXT("/Game/MyCharacters/Blueprints/"),
    TEXT("BP_MyCharacter")
);
```

## Body Type Reference

The plugin supports 18 MetaHuman body type presets:

| Type | Gender | Height | Weight |
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

## Body Measurements

Available measurement parameters (all in centimeters):

- **Height** - Total character height (150-195cm typical)
- **Chest** - Chest circumference (80-120cm typical)
- **Waist** - Waist circumference (60-100cm typical)
- **Hips** - Hip circumference (85-115cm typical)
- **ShoulderWidth** - Distance between shoulders (35-50cm typical)
- **ArmLength** - Arm length from shoulder to wrist (55-70cm typical)
- **LegLength** - Leg length from hip to ankle (75-100cm typical)

## Troubleshooting

### Menu doesn't appear
- Make sure the plugin is enabled in **Edit → Plugins → MetaHuman Parametric Generator**
- Restart the editor
- Check Output Log for "MetaHumanParametricPlugin module has been loaded"

### Generation fails
- Ensure all MetaHuman plugins are enabled
- Check Output Log for error messages (look for red text)
- Verify the output paths exist or are valid UE content paths

### Nothing happens when clicking menu
- Check Output Log - the functions log extensively
- Make sure you're in the Editor (not PIE or packaged game)
- Verify UMetaHumanCharacterEditorSubsystem is available (editor-only)

### Characters look wrong
- This is a demonstration of the API - actual MetaHuman generation requires proper template data and calibration
- The examples show the API usage pattern, not production-ready characters
- You'll need official MetaHuman assets and data for full functionality

## Performance Notes

- **Single Character**: ~30-60 seconds (depends on complexity)
- **Batch Generation**: 3-5 minutes for 5 characters
- Generation is **synchronous** (editor will freeze during processing)
- Monitor the Output Log for real-time progress

## Next Steps

1. Try the examples to understand the workflow
2. Read the API documentation in `README.md`
3. Modify example parameters to create custom characters
4. Integrate into your own tools or workflows
5. Create Blueprint utilities for artists to use the generator

## Support

For issues or questions:
- Check the plugin README.md for API documentation
- Review the example code in `MetaHumanParametricGenerator_Examples.cpp`
- See `MetaHumanParametricGenerator.cpp` for implementation details

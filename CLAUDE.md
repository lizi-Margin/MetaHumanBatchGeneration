# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**MetaHuman Parametric Generator** is an Unreal Engine 5.6+ Editor plugin that enables programmatic MetaHuman character creation with parametric body control. The plugin provides a C++ API for creating, customizing, and exporting MetaHuman characters without using the MetaHuman Creator web interface.

**Platform**: Windows 64-bit only
**Engine**: Unreal Engine 5.6+
**Dependencies**: MetaHuman SDK, MetaHumanCharacter, RigLogic, MetaHumanSDK Runtime/Editor

## Build System

This is an Unreal Engine plugin using Unreal Build Tool (UBT). There are no standalone build commands - the plugin is compiled when the host Unreal Engine project is built.

### Module Dependencies

Key dependencies are defined in `Source/MetaHumanParametricPlugin/MetaHumanParametricPlugin.Build.cs`:

**Core MetaHuman Modules:**
- `MetaHumanCharacter` - Core character assets
- `MetaHumanCharacterEditor` - Editor subsystem for character operations
- `MetaHumanCoreTechLib` - Body identity and parametric system
- `MetaHumanSDKRuntime` / `MetaHumanSDKEditor` - SDK functionality
- `RigLogicLib` / `RigLogicModule` - DNA rigging system

**Unreal Engine Modules:**
- `UnrealEd`, `EditorSubsystem` - Editor integration
- `AssetRegistry`, `AssetTools` - Asset management
- `Slate`, `SlateCore`, `ToolMenus` - UI/menu integration

## Architecture

### Two-Step Character Generation Workflow

The plugin uses a non-blocking two-step workflow to avoid freezing the editor during cloud-based AutoRig operations:

1. **Step 1 - Prepare and Rig** (`PrepareAndRigCharacter`):
   - Creates MetaHuman character asset
   - Configures body parameters (height, chest, waist, etc.)
   - Sets appearance (skin tone, eyes, eyelashes)
   - Downloads texture source data
   - Initiates cloud-based AutoRig (async, returns immediately)

2. **Step 2 - Assemble** (`AssembleCharacter`):
   - Called after AutoRig completes
   - Uses native assembly pipeline to generate final assets
   - Produces skeletal meshes, textures, physics assets, animation blueprints
   - Saves character to specified output path

### Core Components

#### 1. MetaHumanParametricGenerator (`MetaHumanParametricGenerator.h/.cpp`)
The main API for character creation. Key functions:
- `PrepareAndRigCharacter()` - Step 1 of workflow
- `AssembleCharacter()` - Step 2 of workflow
- `GetRiggingStatusString()` - Check if AutoRig is complete
- `EnsureCloudServicesLogin()` - Handle authentication

Structures:
- `FMetaHumanBodyParametricConfig` - Body measurements (height, chest, waist, hips, etc.)
- `FMetaHumanAppearanceConfig` - Appearance settings (skin tone, iris pattern, eyelashes)

#### 2. MetaHumanAssemblyPipelineManager (`MetaHumanAssemblyPipelineManager.h/.cpp`)
Wrapper around native `FMetaHumanCharacterEditorBuild`. Handles:
- Quality level pipeline selection (Cinematic, High, Medium, Low)
- Build path management
- Proper ABP (Animation Blueprint) assembly

Key functions:
- `BuildMetaHumanCharacter()` - Main assembly function
- `GetDefaultPipelineForQuality()` - Get pipeline for quality level
- `CreateDefaultBuildParameters()` - Create build params with defaults

#### 3. EditorBatchGenerationSubsystem (`EditorBatchGenerationSubsystem.h/.cpp`)
Editor subsystem for automatic batch generation using FTSTicker (editor timer). Features:
- Runs in editor without playing the game
- State machine: Idle → Preparing → WaitingForRig → Assembling → Complete
- Loop mode for continuous generation
- Random parameter generation

Key functions:
- `StartBatchGeneration()` - Start batch generation with config
- `StopBatchGeneration()` - Stop current batch
- `GetStatusInfo()` - Query current state

#### 4. RandomGenActor (`Actors/RandomGenActor.h/.cpp`)
Actor-based batch generation (alternative to subsystem). Uses `Tick()` to monitor AutoRig status. Can be spawned in editor world for visual debugging.

#### 5. MetaHumanAssetIOUtility (`MetaHumanAssetIOUtility.h/.cpp`)
Asset saving utilities for meshes, textures, and physics assets. Handles package creation and asset registry.

### Plugin UI Integration

The plugin adds menu entries to the Unreal Editor toolbar under `User → MetaHuman Generator`:

1. **Quick Generation Examples** - Preset character presets (slender female, muscular male, etc.)
2. **Cloud Authentication** - Login status, cloud login, authentication test
3. **Batch Generation (Random)** - Start/stop/status for automated batch generation

Menu registration is handled in `MetaHumanParametricPlugin.cpp` via `RegisterMenuExtensions()`.

## Key Workflows

### Manual Two-Step Generation
```cpp
// Step 1: Create and start rigging
UMetaHumanCharacter* Character;
bool Success = UMetaHumanParametricGenerator::PrepareAndRigCharacter(
    "CharacterName", "/Game/MetaHumans/", BodyConfig, AppearanceConfig, Character);

// Step 2: Wait for rigging to complete (check status periodically)
FString Status = UMetaHumanParametricGenerator::GetRiggingStatusString(Character);
if (Status == "Rigged") {
    UMetaHumanParametricGenerator::AssembleCharacter(
        Character, "/Game/MetaHumans/", EMetaHumanQualityLevel::Cinematic);
}
```

### Batch Generation
Use `EditorBatchGenerationSubsystem` for automated batch generation:
```cpp
UEditorBatchGenerationSubsystem* Subsystem =
    GEditor->GetEditorSubsystem<UEditorBatchGenerationSubsystem>();

Subsystem->StartBatchGeneration(
    true,  // Loop mode
    "/Game/MetaHumans/",
    EMetaHumanQualityLevel::Cinematic,
    2.0f,  // Check interval (seconds)
    5.0f   // Loop delay (seconds)
);
```

## Authentication

MetaHuman generation requires cloud authentication for AutoRig services. The plugin handles this via:
- `EnsureCloudServicesLogin()` - Blocks until logged in (tries persistent credentials first)
- `CheckCloudServicesLoginAsync()` - Async check
- `LoginToCloudServicesAsync()` - Async login with callbacks

Users can manually check/login via the "Cloud Authentication" menu.

## Important Constraints

### Body Measurement Ranges
Defined in `FMetaHumanBodyParametricConfig` defaults and used by batch generation:
- Height: 150-195 cm
- Chest: 75-120 cm
- Waist: 60-100 cm
- Hips: 80-120 cm
- ShoulderWidth: 35-55 cm
- ArmLength: 55-75 cm
- LegLength: 75-105 cm

### Body Types
`EMetaHumanBodyType` enum format: `{gender}_{height}_{build}`
- Gender: `f` (female) or `m` (male)
- Height: `srt` (short), `med` (medium), `tal` (tall)
- Build: `nrw` (narrow), `ovw` (overweight), `unw` (underweight)

Example: `f_med_nrw` = female, medium height, narrow build

### Quality Levels
`EMetaHumanQualityLevel`: `Cinematic`, `High`, `Medium`, `Low`
- Affects pipeline configuration and output asset quality
- Default build paths are read from `MetaHumanSDKSettings`

## State Machine Details

Both batch generation systems use identical state machines:

1. **Idle** - Not running
2. **Preparing** - Creating character and starting AutoRig
3. **WaitingForRig** - Polling rig status via ticker/tick
4. **Assembling** - Building final assets
5. **Complete** - Done (waits for loop delay if enabled)
6. **Error** - Error occurred (logs details, stops processing)

## File Organization

```
MetaHumanParametricPlugin/
├── Source/MetaHumanParametricPlugin/
│   ├── Public/
│   │   ├── MetaHumanParametricGenerator.h         # Main API
│   │   ├── MetaHumanAssemblyPipelineManager.h     # Assembly wrapper
│   │   ├── EditorBatchGenerationSubsystem.h       # Batch gen subsystem
│   │   ├── MetaHumanAssetIOUtility.h              # Asset I/O
│   │   ├── MetaHumanParametricPlugin.h            # Module interface
│   │   └── Actors/
│   │       ├── RandomGenActor.h                   # Actor-based batch gen
│   │       ├── BATCH_GENERATION_GUIDE.md          # Batch gen documentation
│   │       └── TODO.md                            # Future plans
│   ├── Private/
│   │   ├── (corresponding .cpp files)
│   │   └── Actors/
│   └── MetaHumanParametricPlugin.Build.cs
├── Content/
├── Resources/
└── MetaHumanParametricPlugin.uplugin
```

## Common Patterns

### Character Name Generation
Batch generators use timestamp-based naming:
```cpp
FDateTime Now = FDateTime::Now();
FString CharacterName = FString::Printf(TEXT("RandomChar_%02d%02d_%02d%02d%02d"),
    Now.GetMonth(), Now.GetDay(), Now.GetHour(), Now.GetMinute(), Now.GetSecond());
```

### Weak Object References
Use `TWeakObjectPtr<UMetaHumanCharacter>` to avoid hard references that prevent garbage collection:
```cpp
TWeakObjectPtr<UMetaHumanCharacter> GeneratedCharacter;
```

### Editor World Operations
For editor-only operations:
```cpp
UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
```

### Accessing Subsystems
```cpp
UMetaHumanCharacterEditorSubsystem* EditorSubsystem =
    GEditor->GetEditorSubsystem<UMetaHumanCharacterEditorSubsystem>();
```

## Documentation References

Detailed implementation guides:
- `Source/MetaHumanParametricPlugin/Public/Actors/BATCH_GENERATION_GUIDE.md` - Complete batch generation architecture and usage
- `Source/MetaHumanParametricPlugin/Public/Actors/TODO.md` - Planned features (FileCmdActor, parallel generation, etc.)

## Known Issues & Workarounds

1. **Texture Source Data**: Must call `DownloadTextureSourceData()` before assembly to avoid "Texture generated without source data" errors
2. **AutoRig Blocking**: Never use synchronous `RigCharacter()` on UI thread - use two-step workflow instead
3. **Win64 Only**: Plugin is Windows 64-bit only due to MetaHuman SDK platform restrictions

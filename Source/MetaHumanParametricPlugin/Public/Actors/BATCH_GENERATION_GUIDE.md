# Batch Generation System - Implementation Guide

## Overview

The batch generation system allows automatic MetaHuman character creation in the Editor without running the game. It uses an Actor-based approach that spawns in the Editor world and runs continuously.

## Architecture

### Components:

1. **RandomGenActor** (`Actors/RandomGenActor.h/.cpp`)
   - Tick-based state machine actor
   - Generates characters with random parameters
   - Monitors AutoRig progress automatically
   - Supports loop mode for continuous generation

2. **Plugin Module Integration** (`MetaHumanParametricPlugin.h/.cpp`)
   - Spawns and manages BatchGenActor in editor world
   - Provides toolbar menu buttons
   - Handles actor lifecycle

## How It Works

### Editor World Spawning

The key to running actors in the editor without playing the game:

```cpp
// Get the editor world (not PIE world)
UWorld* World = GEditor->GetEditorWorldContext().World();

// Spawn actor in editor world
FActorSpawnParameters SpawnParams;
SpawnParams.Name = FName(TEXT("BatchGenActor"));
SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

BatchGenActor = World->SpawnActor<ARandomGenActor>(
    ARandomGenActor::StaticClass(),
    FVector::ZeroVector,
    FRotator::ZeroRotator,
    SpawnParams
);
```

### Automatic State Management

The actor's `Tick()` function runs in the editor and monitors state:

1. **Idle** → User clicks "Start Batch Generation"
2. **Preparing** → Actor creates character, starts AutoRig, immediately continues
3. **WaitingForRig** → Tick() checks rig status every `TickInterval` seconds
4. **Assembling** → When rigged, calls AssembleCharacter()
5. **Complete** → If loop mode enabled, waits `LoopDelay` seconds, then starts again

## Usage

### Menu Location

**Level Editor Toolbar → User → MetaHuman Generator → Batch Generation (Random)**

Three buttons available:
1. **Start Batch Generation** - Spawns actor and begins automatic generation
2. **Check Status** - Shows current state, character name, and count
3. **Stop Batch Generation** - Stops and destroys the actor

### Configuration

The actor is configured when spawned in `OnStartBatchGeneration()`:

```cpp
BatchGenActor->OutputPath = TEXT("/Game/MetaHumans");
BatchGenActor->QualityLevel = EMetaHumanQualityLevel::Cinematic;
BatchGenActor->TickInterval = 2.0f;        // Check status every 2 seconds
BatchGenActor->bLoopGeneration = true;      // Continuous generation
BatchGenActor->LoopDelay = 5.0f;            // 5 second delay between characters
```

### Workflow Example

1. **Start Generation:**
   - Click "Start Batch Generation"
   - Actor spawns in editor world
   - First character generation begins automatically

2. **Monitor Progress:**
   - Click "Check Status" to see current state
   - Watch Output Log for detailed progress
   - See notifications for major events

3. **Continuous Generation:**
   - After first character completes, waits 5 seconds
   - Automatically starts next character
   - Loop continues indefinitely

4. **Stop When Done:**
   - Click "Stop Batch Generation"
   - Current character finishes (if in progress)
   - Actor is destroyed

## Random Parameter Ranges

### Body Parameters:
- **BodyType**: Random selection from 18 types (f_med_nrw, m_tal_ovw, etc.)
- **Height**: 150-195 cm
- **Chest**: 75-120 cm
- **Waist**: 60-100 cm
- **Hips**: 80-120 cm
- **ShoulderWidth**: 35-55 cm
- **ArmLength**: 55-75 cm
- **LegLength**: 75-105 cm

### Appearance Parameters:
- **SkinTone U/V**: 0.0-1.0 (full color space)
- **SkinRoughness**: 0.5-1.5
- **IrisPattern**: Random from 0-10
- **IrisColor U/V**: 0.0-1.0
- **EyelashesType**: Random from 0-2
- **EyelashGrooms**: Random true/false

## Character Naming

Characters are named with timestamps to ensure uniqueness:
```
Format: RandomChar_MMDD_HHMMSS
Example: RandomChar_1218_143052
```

## Output Location

All generated characters are saved to: `/Game/MetaHumans/`

Each character includes:
- Character asset (.uasset)
- Face mesh
- Body mesh
- Textures (synthesized + high-res if available)
- Physics assets
- Rig Logic assets

## State Machine Details

### State Enum:
```cpp
enum class ERandomGenState : uint8
{
    Idle,           // Not running
    Preparing,      // Creating character and starting AutoRig
    WaitingForRig,  // Monitoring AutoRig progress via Tick()
    Assembling,     // Building final character assets
    Complete,       // Done (waiting for loop delay if enabled)
    Error           // Something went wrong
};
```

### Tick Interval

The `TickInterval` property controls how often the actor checks status:
- `0.0` = Every frame (high overhead, instant response)
- `1.0` = Once per second (balanced)
- `2.0` = Every 2 seconds (recommended, low overhead)
- `5.0` = Every 5 seconds (minimal overhead, slower response)

## Error Handling

The actor automatically handles errors:
- **Character creation fails** → Transitions to Error state
- **AutoRig fails** → Detects unrigged state, transitions to Error
- **Assembly fails** → Transitions to Error state
- **Character reference lost** → Transitions to Error state

In Error state, the actor:
- Logs detailed error message
- Stops processing
- Waits for manual intervention (Stop or Restart)

## Loop Mode

When `bLoopGeneration = true`:

1. Character completes successfully
2. Actor enters Complete state
3. Waits `LoopDelay` seconds (configurable)
4. Automatically calls `StartGeneration()` again
5. Repeats indefinitely

This enables true batch generation - just click "Start" and let it run!

## Future Enhancements (Phase 2)

See `TODO.md` in Actors folder for planned features:
- **FileCmdActor**: Read character parameters from JSON/TXT file
- **Parameter presets**: Save/load character configurations
- **Parallel generation**: Multiple actors running simultaneously
- **Progress UI widget**: Visual progress tracking
- **Export to different formats**: FBX, USD, etc.

## Technical Notes

### Why Actors in Editor?

1. **Tick() runs automatically** - No manual timer management
2. **Editor world lifecycle** - Persists across editor sessions
3. **Visual debugging** - Can see actor in World Outliner
4. **State inspection** - Properties visible in Details panel
5. **Simple cleanup** - Just destroy the actor

### Why Not Just Async Tasks?

Async tasks would require:
- Complex callback chains
- Manual state tracking
- Timer management for polling
- More error-prone code

Actors provide:
- Built-in state machine via Tick()
- Automatic lifecycle management
- Easy debugging and inspection
- Natural fit for the two-step workflow

## Testing Recommendations

1. **Single Character Test**:
   - Start batch generation
   - Wait for first character to complete
   - Stop before loop starts
   - Verify character in /Game/MetaHumans

2. **Loop Test**:
   - Start batch generation
   - Let it run for 3-5 characters
   - Check each character loads correctly
   - Verify no memory leaks

3. **Error Recovery**:
   - Start generation
   - Manually cause an error (disconnect network during AutoRig)
   - Verify actor enters Error state cleanly
   - Stop and restart

4. **Stop Mid-Generation**:
   - Start generation
   - Stop during different states (Preparing, WaitingForRig, Assembling)
   - Verify clean shutdown in all cases

## Troubleshooting

### Actor Not Ticking
- Check World Outliner for "BatchGenActor"
- Verify `PrimaryActorTick.bCanEverTick = true` in constructor
- Check Output Log for tick messages

### Characters Not Generating
- Check authentication: Cloud Authentication > Check Login Status
- Verify network connection for AutoRig
- Check Output Log for detailed error messages

### Memory Issues
- Stop batch generation after extended runs
- Check for leaked character references
- Verify actors are properly destroyed

### AutoRig Stuck
- Cloud service may be slow or unavailable
- Check network connectivity
- Try manual two-step workflow to isolate issue

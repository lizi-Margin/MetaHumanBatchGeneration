# Batch Generation Actor System - TODO List

## Phase 1: RandomGenActor (Current)
**Goal**: Test automatic character generation reliability with random parameters

### Tasks:
- [x] Create Actors folder structure
- [x] Create TODO.md
- [ ] Implement RandomGenActor.h
  - State machine enum (Idle, Preparing, WaitingForRig, Assembling, Complete, Error)
  - Actor properties (CharacterReference, OutputPath, QualityLevel, etc.)
  - Tick() override for state monitoring
  - BeginPlay() to start generation
- [ ] Implement RandomGenActor.cpp
  - Random parameter generation functions
  - State machine logic in Tick()
  - Two-step workflow integration (PrepareAndRigCharacter → AssembleCharacter)
  - Error handling and logging
- [ ] Update Build.cs if needed (add GameplayTags or other dependencies)
- [ ] Test RandomGenActor in editor
- [ ] Verify character saves to /Game/MetaHumans

## Phase 2: FileCmdActor (Future)
**Goal**: Read generation commands from file and execute batch rendering

### Planned Features:
- [ ] File format definition (JSON/TXT)
  - Character parameters (with defaults/random fallbacks)
  - Spawn locations
  - Output paths
  - Quality settings
- [ ] File parser implementation
  - Read command file
  - Parse character parameters
  - Validate inputs
- [ ] Command queue system
  - Queue multiple characters
  - Process one at a time
  - Track progress
- [ ] Enhanced state machine
  - Support multiple characters in sequence
  - Resume on error
  - Save progress
- [ ] Reporting system
  - Log generation results
  - Export statistics
  - Error reports

## Phase 3: Advanced Features (Future)
- [ ] Parallel generation (multiple actors)
- [ ] Progress UI widget
- [ ] Preset parameter libraries
- [ ] Character variation system
- [ ] Export to different formats
- [ ] Integration with sequencer for rendering

## Technical Notes

### RandomGenActor Implementation Details:
- Use `FMath::FRandRange()` for random float parameters
- Use `FMath::RandRange()` for random int/enum selection
- Generate unique names with timestamp
- Store character reference as `TWeakObjectPtr<UMetaHumanCharacter>` to avoid hard references
- Use Tick() to check `GetRiggingStatusString()` every frame during WaitingForRig state
- Add configurable tick interval to reduce overhead

### State Machine Flow:
```
Idle → (BeginPlay) → Preparing → (Call PrepareAndRigCharacter) → WaitingForRig
WaitingForRig → (Check status in Tick) → (If Rigged) → Assembling
Assembling → (Call AssembleCharacter) → Complete
Any State → (On Error) → Error State
```

### Random Parameter Ranges:
- **BodyType**: Randomly select from EMetaHumanBodyType enum
- **Height**: 150cm - 195cm
- **Chest**: 75cm - 120cm
- **Waist**: 60cm - 100cm
- **Hips**: 80cm - 120cm
- **ShoulderWidth**: 35cm - 55cm
- **ArmLength**: 55cm - 75cm
- **LegLength**: 75cm - 105cm
- **SkinTone U/V**: 0.0 - 1.0
- **SkinRoughness**: 0.5 - 1.5
- **IrisPattern**: Randomly select from available patterns
- **IrisColor U/V**: 0.0 - 1.0
- **EyelashesType**: Randomly select from available types

### File Command Format (for Phase 2):
```json
{
  "characters": [
    {
      "name": "Character01",
      "body": {
        "type": "f_med_nrw",
        "height": 170.0,
        "chest": 90.0,
        "waist": "random",
        "globalScale": 1.0
      },
      "appearance": {
        "skinToneU": 0.5,
        "skinToneV": "random",
        "irisPattern": "Iris003"
      },
      "spawn": {
        "location": [0, 0, 0],
        "rotation": [0, 0, 0]
      },
      "output": {
        "path": "/Game/MetaHumans/Batch01",
        "quality": "Cinematic"
      }
    }
  ]
}
```

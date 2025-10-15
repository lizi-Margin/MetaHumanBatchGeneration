# MetaHuman Parametric Plugin - Authentication Setup Guide

## Problem
When using the MetaHumanParametricPlugin, you may encounter authentication errors:
```
LogMetaHumanCharacterEditor: Error: User not logged in, please autorig before downloading source face textures
LogMetaHumanCharacterEditor: Error: User not logged in, please autorig before downloading source body textures
```

## Solution
The plugin now handles authentication automatically but requires proper setup.

### Step 1: Login to MetaHuman Cloud Services

1. **Open Unreal Editor**
2. Navigate to: **Window > MetaHuman > Cloud Services**
3. Click **Login** and authenticate with your Epic Games account
4. Ensure you have the necessary permissions for MetaHuman services

### Step 2: What the Plugin Does Automatically

The updated `DownloadTextureSourceData()` function now:

1. **Checks rigging state** - Determines if the character needs autorigging
2. **Performs autorig** if needed (can take 2-5 minutes)
3. **Attempts texture download** with 2K resolution
4. **Handles authentication gracefully** - continues with default textures if download fails

### Step 3: Testing the Fix

Run any MetaHumanParametricPlugin example:
```cpp
Example1_CreateSlenderFemale();
```

You should see these log messages:
```
[Step 4/6] Downloading texture source data...
Checking if autorig is required before texture download...
Starting autorig... (if needed)
Autorig completed in XXX.X seconds
Requesting 2k texture download...
Note: This requires MetaHuman cloud services login. If not logged in, download will fail but generation will continue with default textures.
```

### Expected Outcomes

#### If Logged In:
- Autorig completes successfully
- Textures download with source data
- No "Texture generated for assembly without source data" error

#### If Not Logged In:
- Warning messages appear about authentication
- Download fails gracefully
- Generation continues with default textures
- Character is still created, just with lower quality textures

### Troubleshooting

#### 1. Login Issues
- Ensure you're logged into MetaHuman Cloud Services
- Check your Epic Games account has MetaHuman access
- Try restarting Unreal Editor after login

#### 2. Autorig Timeout
- Autorig can take 2-5 minutes
- If it times out, the character will still be created but may not have proper rigging
- Check your network connection

#### 3. Texture Download Fails
- Verify you're logged into MetaHuman Cloud Services
- Check network connectivity
- The plugin will continue with default textures if download fails

### Best Practices

1. **Always log in** to MetaHuman Cloud Services before running character generation
2. **Be patient** - autorig and texture download can take several minutes
3. **Monitor logs** - progress is reported every 10-15 seconds
4. **Network stability** - ensure stable internet connection for cloud services

## Summary

The plugin now handles the full workflow:
1. Creates character
2. Configures body/appearance
3. **NEW**: Performs autorig if needed
4. **NEW**: Downloads texture source data
5. Generates assets
6. Saves to disk

This ensures compatibility with MetaHuman's requirements while gracefully handling authentication issues.
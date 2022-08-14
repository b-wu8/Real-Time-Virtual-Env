
# cs717-oculus

### Unity Setup
- Install XR Interaction Toolkit
- Install XR Plugin Management 
  - Under Androi icon, click checkbox to enable for Oculus 
  - Under Computer icon, click checkbox to enable for Oculus
- TODO...

### Unity GameObjects

- Open a new Unity Project
- Add Directional Light (optional)
- Add a sphere GameObject with scale = 0
  - Need to include at least one static GameObject so Shaders is compiled and linked during build.
- Add XR Origin (XR -> Device-Based -> XR Origin)
  - Set TrackOriginMode = Device
  - Make sure XR Origin Transform is (0, 0, 0)
- Add an empty GameObject called Client
  - Add Config.cs script to Client
    - Set variables in config script
  - Add DeviceInfoWatcher.cs script to Client
  - Add PlayerView.cs script to Client
    - Link XROrigin -> CameraOffset as the xr_rig_offset object in PlayerView.
    - Link Config.cs script as the config object for PlayerView
  - Add OculusClient.cs script to Client
    - Link Config.cs script, PlayerView.cs script, and DeviceInfoWatcher.cs scripts to the objects in PlayerView
- Make sure server is running
- Starting playing :)

* Add config to needed gameobjects
* Fill in the port and ip address like this:
<img width="495" alt="image" src="https://user-images.githubusercontent.com/23161882/157157367-e996bc20-5573-4378-a8dc-1d7408b450e8.png">

### For Server
* run the server with `./oculus_server <ip> <port> <tick_delay_ms (default=50)>` example `./oculus_server 123.456.789.100 1234 30`

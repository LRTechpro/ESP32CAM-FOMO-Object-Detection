# ESP32-CAM FOMO Validation Protocol

This protocol validates the custom two-class FOMO model on the AI Thinker ESP32-CAM using both the direct Edge Impulse example and the EloquentEsp32cam inference workflow.

## Target classes

| Label | Artifact |
|---|---|
| `keyfob` | Automotive remote key fob |
| `J2534VCI` | SAE J2534 vehicle communication interface |

## Required scene matrix

Run each scene at least three times with the direct Edge Impulse sketch and at least three times with the Eloquent FOMO sketch.

| Scene | Expected behavior | Minimum trials per sketch |
|---|---|---:|
| Key fob only | Detect `keyfob` | 3 |
| J2534 VCI only | Detect `J2534VCI` | 3 |
| Both artifacts together | Detect both labels when visible | 3 |
| Blank background | Return no object detections | 3 |

## Hardware and IDE settings

- Board: **AI Thinker ESP32-CAM**
- Camera: OV2640
- Serial Monitor: **115200 baud**
- Model header: `HaroldLRWatkins-project-1_inferencing.h`
- Eloquent camera resolution preset: `camera.resolution.yolo()`
- Eloquent pixel format: `camera.pixformat.rgb565()`

## Firmware stages

1. `firmware/01_edge_impulse_example/Stage1_EdgeImpulse_ESP32CAM.ino`
   - Calls the generated Edge Impulse classifier directly.
   - Prints label, confidence, x/y location, width, and height.

2. `firmware/02_eloquent_fomo/Stage2_Eloquent_FOMO.ino`
   - Uses EloquentEsp32cam to capture the frame and run FOMO.
   - Prints the detected objects and bounding-box dimensions.

3. `firmware/03_eloquent_fomo_area/Stage3_Eloquent_FOMO_Area.ino`
   - Adds bounding-box area calculation.
   - Uses `area = width × height` and reports the result in square model-output pixels.

## Example serial output format

```text
Found 2 object(s) in 31ms
#1) keyfob | x=8 y=11 | size=13x10 | area=130 px^2 | proba=0.84
#2) J2534VCI | x=25 y=14 | size=16x12 | area=192 px^2 | proba=0.79
```

The values above illustrate the output format only. Actual coordinates, areas, confidence values, and inference times depend on camera distance, lighting, object orientation, and model behavior.

## Evidence checklist

Capture screenshots showing:

- Each of the three firmware versions in Arduino IDE
- Successful compilation and upload
- Serial Monitor output for the four scene types
- At least three trials per scene type for each required inference workflow
- Bounding-box area output from the modified sketch

## Interpretation notes

Bounding-box area is measured in the model output coordinate space, not in physical square centimeters or inches. A larger reported area usually indicates that the detected object occupies more of the camera frame, but physical distance estimation requires camera calibration and a known object dimension.

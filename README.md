# ESP32-CAM FOMO Object Detection

## Real-time key fob and J2534 VCI recognition at the edge

This embedded computer vision project deploys a custom Edge Impulse FOMO object-detection model to an AI Thinker ESP32-CAM. The model recognizes two automotive artifacts in real time:

| Class label | Target artifact |
|---|---|
| `keyfob` | Automotive remote key fob |
| `J2534VCI` | SAE J2534 vehicle communication interface |

The repository demonstrates three deployment stages: direct Edge Impulse inference, EloquentEsp32cam FOMO inference, and an extended implementation that calculates bounding-box area.

## Project goals

- Run a custom Edge Impulse object detector on an ESP32-CAM.
- Validate single-object, multi-object, and blank-scene behavior.
- Compare direct Edge Impulse inference with the EloquentEsp32cam workflow.
- Extract bounding-box dimensions from each detection.
- Calculate box area as a lightweight geometric feature.
- Keep the implementation small enough for constrained edge hardware.

## Hardware and software

| Component | Configuration |
|---|---|
| Camera board | AI Thinker ESP32-CAM with OV2640 |
| Development environment | Arduino IDE 2.3.9 |
| Serial speed | 115200 baud |
| Machine-learning platform | Edge Impulse Studio |
| Model architecture | FOMO MobileNetV2 0.35 |
| Model input | 48 × 48 RGB |
| Deployment target | Quantized int8 Arduino library |
| Camera helper library | EloquentEsp32cam |
| Generated model header | `HaroldLRWatkins-project-1_inferencing.h` |

## Repository structure

```text
├── README.md
├── LICENSE
├── firmware/
│   ├── 01_edge_impulse_example/
│   │   └── Stage1_EdgeImpulse_ESP32CAM.ino
│   ├── 02_eloquent_fomo/
│   │   └── Stage2_Eloquent_FOMO.ino
│   └── 03_eloquent_fomo_area/
│       └── Stage3_Eloquent_FOMO_Area.ino
└── docs/
    └── VALIDATION_PROTOCOL.md
```

## Deployment stages

### Stage 1: Direct Edge Impulse inference

[`Stage1_EdgeImpulse_ESP32CAM.ino`](firmware/01_edge_impulse_example/Stage1_EdgeImpulse_ESP32CAM.ino) uses the generated Edge Impulse Arduino library directly. It:

- Initializes the AI Thinker camera pinout.
- Captures a QVGA JPEG frame.
- Converts the image to RGB888.
- Resizes the frame to the model input dimensions.
- Calls `run_classifier()`.
- Prints the label, confidence, coordinates, width, and height for each detection.

### Stage 2: EloquentEsp32cam FOMO inference

[`Stage2_Eloquent_FOMO.ino`](firmware/02_eloquent_fomo/Stage2_Eloquent_FOMO.ino) uses EloquentEsp32cam to simplify camera configuration and inference:

```cpp
camera.pinout.aithinker();
camera.resolution.yolo();
camera.pixformat.rgb565();
```

The sketch prints the number of objects found, inference time, label, coordinates, dimensions, and probability.

### Stage 3: Bounding-box area

[`Stage3_Eloquent_FOMO_Area.ino`](firmware/03_eloquent_fomo_area/Stage3_Eloquent_FOMO_Area.ino) extends the Eloquent workflow with:

```cpp
const uint32_t area =
    static_cast<uint32_t>(bbox.width) *
    static_cast<uint32_t>(bbox.height);
```

Example output format:

```text
#1) keyfob | x=8 y=11 | size=13x10 | area=130 px^2 | proba=0.84
```

The reported area is measured in model-output pixels. It indicates how much of the model frame is occupied by the detected object; it is not a physical square-inch or square-centimeter measurement.

## Validation matrix

Each inference workflow is tested against four scene types:

| Scene | Minimum trials | Expected behavior |
|---|---:|---|
| Key fob only | 3 | Detect `keyfob` |
| J2534 VCI only | 3 | Detect `J2534VCI` |
| Both objects | 3 | Detect both labels when visible |
| Blank background | 3 | Return no object detections |

See [`docs/VALIDATION_PROTOCOL.md`](docs/VALIDATION_PROTOCOL.md) for the complete test procedure and evidence checklist.

## Setup

1. Export the trained Edge Impulse impulse as an Arduino library.
2. Install the generated ZIP through **Sketch → Include Library → Add .ZIP Library**.
3. Install the EloquentEsp32cam library for Stages 2 and 3.
4. Select **AI Thinker ESP32-CAM** as the Arduino board.
5. Compile and upload one firmware stage at a time.
6. Remove GPIO 0 from ground after uploading and reset the board.
7. Open Serial Monitor at **115200 baud**.
8. Present the key fob, J2534 VCI, both objects, and blank scenes to the camera.

## Design notes

### Why FOMO?

FOMO is designed for constrained edge hardware and uses a compact grid-based detection approach. It is well suited to experiments where low memory usage and near-real-time inference matter more than high-resolution bounding boxes.

### Why EloquentEsp32cam?

EloquentEsp32cam reduces camera boilerplate and provides a concise interface for capture, inference, bounding-box enumeration, and benchmark timing.

### Why box area?

Bounding-box area is a low-cost feature that can support later proximity or calibration experiments. A larger box generally means the detected object occupies more of the image, but distance estimation requires camera calibration and a known physical reference.

## Security and privacy

- No Wi-Fi credentials are stored in this repository.
- No customer data, VINs, ECU firmware, calibration files, or proprietary diagnostic information are included.
- The project performs visual detection only; it does not communicate with vehicles or program keys or modules.
- Detection results are probabilistic and should not be treated as a standalone security control.

## Author

**Harold L. R. Watkins**  
Automotive Cybersecurity | Embedded Systems | Computer Vision

## License

Released under the [MIT License](LICENSE).

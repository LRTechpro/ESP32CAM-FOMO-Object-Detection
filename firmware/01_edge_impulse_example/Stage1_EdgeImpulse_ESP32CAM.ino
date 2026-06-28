/**
 * ESP32-CAM + Edge Impulse generated-library inference example
 *
 * Target: AI Thinker ESP32-CAM
 * Model: HaroldLRWatkins-project-1
 * Labels: keyfob, J2534VCI
 * Serial Monitor: 115200 baud
 *
 * This sketch uses the Edge Impulse generated Arduino library directly.
 */

#include <HaroldLRWatkins-project-1_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include "esp_camera.h"

// AI Thinker ESP32-CAM pin map
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define EI_CAMERA_RAW_FRAME_BUFFER_COLS 320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS 240
#define EI_CAMERA_FRAME_BYTE_SIZE       3

static bool debug_nn = false;
static bool camera_initialized = false;
static uint8_t *snapshot_buf = nullptr;

static camera_config_t camera_config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,

    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,

    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_QVGA,
    .jpeg_quality = 12,
    .fb_count = 1,
    .fb_location = CAMERA_FB_IN_DRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

bool ei_camera_init();
void ei_camera_deinit();
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf);
static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }

    Serial.println();
    Serial.println("ESP32-CAM Edge Impulse FOMO inference");

    if (!ei_camera_init()) {
        Serial.println("ERROR: Camera initialization failed");
        return;
    }

    Serial.println("Camera initialized");
    Serial.println("Starting inference in 2 seconds...");
    ei_sleep(2000);
}

void loop() {
    if (!camera_initialized) {
        delay(1000);
        return;
    }

    snapshot_buf = static_cast<uint8_t *>(malloc(
        EI_CAMERA_RAW_FRAME_BUFFER_COLS *
        EI_CAMERA_RAW_FRAME_BUFFER_ROWS *
        EI_CAMERA_FRAME_BYTE_SIZE));

    if (snapshot_buf == nullptr) {
        Serial.println("ERROR: Failed to allocate snapshot buffer");
        delay(1000);
        return;
    }

    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &ei_camera_get_data;

    if (!ei_camera_capture(EI_CLASSIFIER_INPUT_WIDTH,
                           EI_CLASSIFIER_INPUT_HEIGHT,
                           snapshot_buf)) {
        Serial.println("ERROR: Image capture failed");
        free(snapshot_buf);
        snapshot_buf = nullptr;
        delay(500);
        return;
    }

    ei_impulse_result_t result = {0};
    const EI_IMPULSE_ERROR inference_status = run_classifier(&signal, &result, debug_nn);

    if (inference_status != EI_IMPULSE_OK) {
        Serial.printf("ERROR: run_classifier failed (%d)\n", inference_status);
        free(snapshot_buf);
        snapshot_buf = nullptr;
        delay(500);
        return;
    }

    Serial.printf("Timing: DSP %d ms, classification %d ms, anomaly %d ms\n",
                  result.timing.dsp,
                  result.timing.classification,
                  result.timing.anomaly);

#if EI_CLASSIFIER_OBJECT_DETECTION == 1
    bool found = false;

    for (uint32_t i = 0; i < result.bounding_boxes_count; i++) {
        const ei_impulse_result_bounding_box_t &box = result.bounding_boxes[i];

        if (box.value == 0) {
            continue;
        }

        found = true;
        Serial.printf("%s %.2f | x=%u y=%u width=%u height=%u\n",
                      box.label,
                      box.value,
                      box.x,
                      box.y,
                      box.width,
                      box.height);
    }

    if (!found) {
        Serial.println("No objects detected");
    }
#else
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        Serial.printf("%s: %.5f\n",
                      ei_classifier_inferencing_categories[i],
                      result.classification[i].value);
    }
#endif

    free(snapshot_buf);
    snapshot_buf = nullptr;
    delay(500);
}

bool ei_camera_init() {
    if (camera_initialized) {
        return true;
    }

    const esp_err_t result = esp_camera_init(&camera_config);
    if (result != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", result);
        return false;
    }

    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor != nullptr && sensor->id.PID == OV3660_PID) {
        sensor->set_vflip(sensor, 1);
        sensor->set_brightness(sensor, 1);
        sensor->set_saturation(sensor, 0);
    }

    camera_initialized = true;
    return true;
}

void ei_camera_deinit() {
    const esp_err_t result = esp_camera_deinit();
    if (result != ESP_OK) {
        Serial.println("Camera deinitialization failed");
        return;
    }

    camera_initialized = false;
}

bool ei_camera_capture(uint32_t img_width,
                       uint32_t img_height,
                       uint8_t *out_buf) {
    if (!camera_initialized) {
        Serial.println("ERROR: Camera is not initialized");
        return false;
    }

    camera_fb_t *frame = esp_camera_fb_get();
    if (frame == nullptr) {
        Serial.println("ERROR: Camera frame capture failed");
        return false;
    }

    const bool converted = fmt2rgb888(frame->buf,
                                      frame->len,
                                      frame->format,
                                      out_buf);
    esp_camera_fb_return(frame);

    if (!converted) {
        Serial.println("ERROR: JPEG-to-RGB conversion failed");
        return false;
    }

    if (img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS ||
        img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS) {
        ei::image::processing::crop_and_interpolate_rgb888(
            out_buf,
            EI_CAMERA_RAW_FRAME_BUFFER_COLS,
            EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
            out_buf,
            img_width,
            img_height);
    }

    return true;
}

static int ei_camera_get_data(size_t offset,
                              size_t length,
                              float *out_ptr) {
    size_t pixel_index = offset * 3;

    for (size_t i = 0; i < length; i++) {
        out_ptr[i] = static_cast<float>(
            (snapshot_buf[pixel_index + 2] << 16) |
            (snapshot_buf[pixel_index + 1] << 8) |
             snapshot_buf[pixel_index]);
        pixel_index += 3;
    }

    return 0;
}

/**
 * ESP32-CAM FOMO inference using EloquentEsp32cam
 *
 * Target: AI Thinker ESP32-CAM
 * Model: HaroldLRWatkins-project-1
 * Labels: keyfob, J2534VCI
 * Serial Monitor: 115200 baud
 *
 * Stock EloquentArduino-style FOMO workflow used to capture a frame,
 * run the Edge Impulse model, and print bounding boxes.
 */

#include <HaroldLRWatkins-project-1_inferencing.h>
#include <eloquent_esp32cam.h>
#include <eloquent_esp32cam/edgeimpulse/fomo.h>

using eloq::camera;
using eloq::ei::fomo;

void setup() {
    delay(3000);
    Serial.begin(115200);
    Serial.println();
    Serial.println("__EDGE IMPULSE FOMO (NO-PSRAM)__");

    // AI Thinker ESP32-CAM hardware configuration
    camera.pinout.aithinker();
    camera.brownout.disable();

    // Non-PSRAM FOMO runs on 96x96 RGB565 frames.
    camera.resolution.yolo();
    camera.pixformat.rgb565();

    while (!camera.begin().isOk()) {
        Serial.println(camera.exception.toString());
        delay(500);
    }

    Serial.println("Camera OK");
    Serial.println("Put object in front of camera");
}

void loop() {
    if (!camera.capture().isOk()) {
        Serial.println(camera.exception.toString());
        delay(250);
        return;
    }

    if (!fomo.run().isOk()) {
        Serial.println(fomo.exception.toString());
        delay(250);
        return;
    }

    Serial.printf(
        "Found %d object(s) in %dms\n",
        fomo.count(),
        fomo.benchmark.millis());

    if (!fomo.foundAnyObject()) {
        Serial.println("No objects detected");
        delay(500);
        return;
    }

    // Convenient access when a single object is expected.
    Serial.printf(
        "Found %s at (x = %d, y = %d) (size %d x %d). Proba is %.2f\n",
        fomo.first.label,
        fomo.first.x,
        fomo.first.y,
        fomo.first.width,
        fomo.first.height,
        fomo.first.proba);

    // Enumerate all boxes when multiple objects are present.
    if (fomo.count() > 1) {
        fomo.forEach([](int index, bbox_t bbox) {
            Serial.printf(
                "#%d) Found %s at (x = %d, y = %d) (size %d x %d). Proba is %.2f\n",
                index + 1,
                bbox.label,
                bbox.x,
                bbox.y,
                bbox.width,
                bbox.height,
                bbox.proba);
        });
    }

    delay(500);
}

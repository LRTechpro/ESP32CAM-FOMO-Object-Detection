/**
 * ESP32-CAM FOMO inference with bounding-box area calculation
 *
 * Target: AI Thinker ESP32-CAM
 * Model: HaroldLRWatkins-project-1
 * Labels: keyfob, J2534VCI
 * Serial Monitor: 115200 baud
 *
 * Area is calculated in model-output pixels:
 *     area = bounding-box width × bounding-box height
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
    Serial.println("__EDGE IMPULSE FOMO WITH BOX AREA__");

    camera.pinout.aithinker();
    camera.brownout.disable();
    camera.resolution.yolo();
    camera.pixformat.rgb565();

    while (!camera.begin().isOk()) {
        Serial.println(camera.exception.toString());
        delay(500);
    }

    Serial.println("Camera OK");
    Serial.println("Bounding-box area output enabled");
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

    fomo.forEach([](int index, bbox_t bbox) {
        const uint32_t area =
            static_cast<uint32_t>(bbox.width) *
            static_cast<uint32_t>(bbox.height);

        Serial.printf(
            "#%d) %s | x=%d y=%d | size=%dx%d | area=%lu px^2 | proba=%.2f\n",
            index + 1,
            bbox.label,
            bbox.x,
            bbox.y,
            bbox.width,
            bbox.height,
            static_cast<unsigned long>(area),
            bbox.proba);
    });

    delay(500);
}

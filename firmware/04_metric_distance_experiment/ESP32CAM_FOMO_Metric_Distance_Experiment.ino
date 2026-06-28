/**
 * ESP32-CAM FOMO Metric Distance Experiment
 *
 * Commands:
 *   K22, K33, K44 = key fob at 22/33/44 cm
 *   V22, V33, V44 = J2534 VCI at 22/33/44 cm
 *
 * Board: AI Thinker ESP32-CAM
 * Baud: 115200
 * Serial Monitor line ending: Newline
 */

#include <HaroldLRWatkins-project-1_inferencing.h>
#include <eloquent_esp32cam.h>
#include <eloquent_esp32cam/edgeimpulse/fomo.h>
#include <cstring>

using eloq::camera;
using eloq::ei::fomo;

// These labels must match the Edge Impulse model exactly.
static const char *KEYFOB_LABEL = "keyfob";
static const char *VCI_LABEL = "J2534VCI";
static const char *activeTargetLabel = KEYFOB_LABEL;

static const uint16_t SAMPLES_PER_DISTANCE = 10;

static uint32_t frameCount = 0;
static uint16_t sampleCount = 0;
static float currentDistanceCm = 0.0f;
static bool collectionActive = false;

static bool targetFound = false;
static float bestTargetProbability = -1.0f;
static int bestTargetX = 0;
static int bestTargetY = 0;
static int bestTargetWidth = 0;
static int bestTargetHeight = 0;

void collectBestTarget(int index, bbox_t bbox) {
    (void)index;

    const char *detectedLabel = bbox.label.c_str();

    if (
        strcmp(detectedLabel, activeTargetLabel) == 0 &&
        (!targetFound || bbox.proba > bestTargetProbability)
    ) {
        targetFound = true;
        bestTargetProbability = bbox.proba;
        bestTargetX = bbox.x;
        bestTargetY = bbox.y;
        bestTargetWidth = bbox.width;
        bestTargetHeight = bbox.height;
    }
}

void printAllDetections(int index, bbox_t bbox) {
    Serial.printf(
        "OBSERVED,%d,%s,%d,%d,%d,%d,%.2f\n",
        index + 1,
        bbox.label.c_str(),
        bbox.x,
        bbox.y,
        bbox.width,
        bbox.height,
        bbox.proba
    );
}

void readDistanceCommand() {
    if (!Serial.available()) {
        return;
    }

    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.length() == 0) {
        return;
    }

    char prefix = command.charAt(0);

    if (prefix == 'K' || prefix == 'k') {
        activeTargetLabel = KEYFOB_LABEL;
        command.remove(0, 1);
    }
    else if (prefix == 'V' || prefix == 'v') {
        activeTargetLabel = VCI_LABEL;
        command.remove(0, 1);
    }
    else if (
        prefix == 'C' || prefix == 'c' ||
        prefix == 'D' || prefix == 'd'
    ) {
        command.remove(0, 1);
    }

    command.trim();
    float requestedDistance = command.toFloat();

    if (requestedDistance <= 0.0f) {
        Serial.println("INVALID_COMMAND");
        Serial.println("Use K22, K33, K44, V22, V33, or V44.");
        return;
    }

    currentDistanceCm = requestedDistance;
    sampleCount = 0;
    collectionActive = true;

    Serial.printf(
        "COLLECTION_STARTED,distance_cm=%.1f,samples=%u,target=%s\n",
        currentDistanceCm,
        SAMPLES_PER_DISTANCE,
        activeTargetLabel
    );
}

void printCsvRow(
    const char *status,
    const char *label,
    int objectCount,
    int x,
    int y,
    int width,
    int height,
    int area,
    float confidence,
    int inferenceMs,
    uint32_t checksum
) {
    Serial.printf(
        "CSV,%.1f,%u,%lu,%s,%d,%s,%d,%d,%d,%d,%d,%.2f,%d,0x%08lX\n",
        currentDistanceCm,
        sampleCount,
        (unsigned long)frameCount,
        status,
        objectCount,
        label,
        x,
        y,
        width,
        height,
        area,
        confidence,
        inferenceMs,
        (unsigned long)checksum
    );
}

void setup() {
    delay(3000);
    Serial.begin(115200);
    Serial.setTimeout(100);

    Serial.println();
    Serial.println("ESP32-CAM FOMO METRIC DISTANCE EXPERIMENT");
    Serial.printf("Project: %s\n", EI_CLASSIFIER_PROJECT_NAME);
    Serial.printf("Model input: %d x %d\n", EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT);
    Serial.printf("Key fob label: %s\n", KEYFOB_LABEL);
    Serial.printf("J2534 VCI label: %s\n", VCI_LABEL);
    Serial.printf("Default target: %s\n", activeTargetLabel);
    Serial.printf("Samples per distance: %u\n", SAMPLES_PER_DISTANCE);

    camera.pinout.aithinker();
    camera.brownout.disable();
    camera.resolution.yolo();
    camera.pixformat.rgb565();

    while (!camera.begin().isOk()) {
        Serial.println(camera.exception.toString());
        delay(1000);
    }

    Serial.println("Camera OK");
    Serial.println("Serial Monitor line ending: Newline");
    Serial.println("Key fob commands: K22, K33, K44");
    Serial.println("J2534 VCI commands: V22, V33, V44");
    Serial.println(
        "CSV_HEADER,distance_cm,sample,frame,status,object_count,label,"
        "x,y,width,height,area,confidence,inference_ms,checksum"
    );
}

void loop() {
    readDistanceCommand();

    if (!collectionActive) {
        delay(50);
        return;
    }

    if (!camera.capture().isOk()) {
        Serial.println(camera.exception.toString());
        delay(250);
        return;
    }

    frameCount++;

    uint8_t *buf = camera.frame->buf;
    uint32_t len = (uint32_t)camera.frame->len;
    uint32_t checksum = 0;

    for (uint32_t i = 0; i < len; i += 8) {
        checksum += buf[i];
    }

    if (!fomo.run().isOk()) {
        Serial.println(fomo.exception.toString());
        delay(250);
        return;
    }

    sampleCount++;

    int inferenceMs = fomo.benchmark.millis();
    int objectCount = fomo.count();

    targetFound = false;
    bestTargetProbability = -1.0f;
    bestTargetX = 0;
    bestTargetY = 0;
    bestTargetWidth = 0;
    bestTargetHeight = 0;

    if (fomo.foundAnyObject()) {
        fomo.forEach(collectBestTarget);
    }

    if (targetFound) {
        int area = bestTargetWidth * bestTargetHeight;

        printCsvRow(
            "DETECTED",
            activeTargetLabel,
            objectCount,
            bestTargetX,
            bestTargetY,
            bestTargetWidth,
            bestTargetHeight,
            area,
            bestTargetProbability,
            inferenceMs,
            checksum
        );
    }
    else if (fomo.foundAnyObject()) {
        int area = fomo.first.width * fomo.first.height;

        printCsvRow(
            "WRONG_LABEL",
            fomo.first.label.c_str(),
            objectCount,
            fomo.first.x,
            fomo.first.y,
            fomo.first.width,
            fomo.first.height,
            area,
            fomo.first.proba,
            inferenceMs,
            checksum
        );

        // Print every returned label so case and model-output mismatches are visible.
        fomo.forEach(printAllDetections);
    }
    else {
        printCsvRow(
            "MISSED",
            "NONE",
            0,
            0,
            0,
            0,
            0,
            0,
            0.0f,
            inferenceMs,
            checksum
        );
    }

    if (sampleCount >= SAMPLES_PER_DISTANCE) {
        collectionActive = false;

        Serial.printf(
            "COLLECTION_COMPLETE,distance_cm=%.1f,samples=%u,target=%s\n",
            currentDistanceCm,
            sampleCount,
            activeTargetLabel
        );

        Serial.println("Move the selected object to the next distance and enter the next command.");
    }

    delay(300);
}

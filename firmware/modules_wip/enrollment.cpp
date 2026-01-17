// TouchPass Enrollment Implementation

#include "enrollment.h"
#include "config.h"

const char* enrollMessages[] = {
    "Place finger on sensor",
    "Lift and place again",
    "Again, adjust slightly",
    "Now adjust your grip",
    "Place again",
    "One more time"
};

EnrollmentManager::EnrollmentManager(FingerprintSensor* sensor, TouchPassStorage* storage)
    : fpSensor(sensor),
      storage(storage),
      state(ENROLL_IDLE),
      pendingSlot(-1),
      pendingFingerId(-1),
      success(false),
      timeout(0) {
}

bool EnrollmentManager::startEnrollment(const String& name, const String& password,
                                        bool pressEnter, int fingerId) {
    // Check for existing slot for this finger
    int16_t existingSlot = storage->findSlotForFinger(fingerId, fpSensor->getLibrarySize());
    if (existingSlot >= 0) {
        fpSensor->deleteTemplate(existingSlot, 1);
        storage->deleteFinger(existingSlot);
    }

    // Find empty slot
    pendingSlot = fpSensor->findEmptySlot();
    if (pendingSlot < 0 || pendingSlot >= fpSensor->getLibrarySize()) {
        error = "Library full";
        return false;
    }

    // Setup enrollment
    pendingName = name;
    pendingPassword = password;
    pendingPressEnter = pressEnter;
    pendingFingerId = fingerId;
    success = false;
    error = "";
    timeout = millis() + ENROLL_TIMEOUT_MS;

    setState(ENROLL_CAPTURE_1);
    fpSensor->setLED(LED_BREATHING, 100, LED_BLUE, 0);

    return true;
}

void EnrollmentManager::process() {
    if (state == ENROLL_IDLE || state == ENROLL_DONE) {
        return;
    }

    // Check timeout
    if (millis() > timeout) {
        error = "Timeout";
        success = false;
        setState(ENROLL_DONE);
        fpSensor->setLED(LED_ON, 0, LED_RED, 0);
        return;
    }

    // State machine
    switch (state) {
        case ENROLL_CAPTURE_1:
            if (captureToBuffer(1)) {
                setState(ENROLL_LIFT_1);
                fpSensor->setLED(LED_FLASHING, 100, LED_GREEN, 2);
            }
            break;

        case ENROLL_LIFT_1:
            if (fpSensor->isFingerLifted()) {
                setState(ENROLL_CAPTURE_2);
                fpSensor->setLED(LED_BREATHING, 100, LED_BLUE, 0);
            }
            break;

        case ENROLL_CAPTURE_2:
            if (captureToBuffer(2)) {
                setState(ENROLL_LIFT_2);
                fpSensor->setLED(LED_FLASHING, 100, LED_GREEN, 2);
            }
            break;

        case ENROLL_LIFT_2:
            if (fpSensor->isFingerLifted()) {
                setState(ENROLL_CAPTURE_3);
                fpSensor->setLED(LED_BREATHING, 100, LED_BLUE, 0);
            }
            break;

        case ENROLL_CAPTURE_3:
            if (captureToBuffer(3)) {
                setState(ENROLL_LIFT_3);
                fpSensor->setLED(LED_FLASHING, 100, LED_GREEN, 2);
            }
            break;

        case ENROLL_LIFT_3:
            if (fpSensor->isFingerLifted()) {
                setState(ENROLL_CAPTURE_4);
                fpSensor->setLED(LED_FLASHING, 100, LED_BLUE, 3);
                delay(300);
                fpSensor->setLED(LED_BREATHING, 100, LED_BLUE, 0);
            }
            break;

        case ENROLL_CAPTURE_4:
            if (captureToBuffer(4)) {
                setState(ENROLL_LIFT_4);
                fpSensor->setLED(LED_FLASHING, 100, LED_GREEN, 2);
            }
            break;

        case ENROLL_LIFT_4:
            if (fpSensor->isFingerLifted()) {
                setState(ENROLL_CAPTURE_5);
                fpSensor->setLED(LED_BREATHING, 100, LED_BLUE, 0);
            }
            break;

        case ENROLL_CAPTURE_5:
            if (captureToBuffer(5)) {
                setState(ENROLL_LIFT_5);
                fpSensor->setLED(LED_FLASHING, 100, LED_GREEN, 2);
            }
            break;

        case ENROLL_LIFT_5:
            if (fpSensor->isFingerLifted()) {
                setState(ENROLL_CAPTURE_6);
                fpSensor->setLED(LED_BREATHING, 100, LED_BLUE, 0);
            }
            break;

        case ENROLL_CAPTURE_6:
            if (captureToBuffer(6)) {
                setState(ENROLL_MERGING);
                fpSensor->setLED(LED_BREATHING, 50, LED_BLUE, 0);
            }
            break;

        case ENROLL_MERGING:
            {
                uint8_t result = fpSensor->createTemplate();
                if (result != 0x00) {
                    error = "Template merge failed";
                    success = false;
                    setState(ENROLL_DONE);
                    fpSensor->setLED(LED_ON, 0, LED_RED, 0);
                    return;
                }

                result = fpSensor->storeTemplate(1, pendingSlot);
                if (result != 0x00) {
                    error = "Store failed";
                    success = false;
                    setState(ENROLL_DONE);
                    fpSensor->setLED(LED_ON, 0, LED_RED, 0);
                    return;
                }

                // Save to storage
                FingerData data;
                data.name = pendingName;
                data.password = pendingPassword;
                data.pressEnter = pendingPressEnter;
                data.fingerId = pendingFingerId;
                storage->saveFinger(pendingSlot, data);

                // Success
                success = true;
                setState(ENROLL_DONE);
                fpSensor->setLED(LED_ON, 0, LED_GREEN, 0);
                delay(500);
            }
            break;

        default:
            break;
    }
}

void EnrollmentManager::cancel() {
    setState(ENROLL_IDLE);
    pendingName = "";
    pendingPassword = "";
    pendingPressEnter = false;
    pendingSlot = -1;
}

EnrollState EnrollmentManager::getState() {
    return state;
}

int EnrollmentManager::getCurrentStep() {
    if (state >= ENROLL_CAPTURE_1 && state <= ENROLL_LIFT_1) return 1;
    if (state >= ENROLL_CAPTURE_2 && state <= ENROLL_LIFT_2) return 2;
    if (state >= ENROLL_CAPTURE_3 && state <= ENROLL_LIFT_3) return 3;
    if (state >= ENROLL_CAPTURE_4 && state <= ENROLL_LIFT_4) return 4;
    if (state >= ENROLL_CAPTURE_5 && state <= ENROLL_LIFT_5) return 5;
    if (state >= ENROLL_CAPTURE_6 || state == ENROLL_MERGING) return 6;
    return 0;
}

bool EnrollmentManager::isSuccess() {
    return success;
}

String EnrollmentManager::getError() {
    return error;
}

String EnrollmentManager::getStatusMessage() {
    int step = getCurrentStep();
    if (step >= 1 && step <= 6) {
        return String(enrollMessages[step - 1]);
    }
    if (state == ENROLL_MERGING) {
        return "Creating template...";
    }
    return "";
}

bool EnrollmentManager::isEnrolling() {
    return state != ENROLL_IDLE && state != ENROLL_DONE;
}

// ===== Private Methods =====

bool EnrollmentManager::captureToBuffer(uint8_t bufferNum) {
    uint8_t result = fpSensor->captureImage();
    if (result != 0x00) return false;

    result = fpSensor->generateCharacteristics(bufferNum);
    if (result != 0x00) {
        error = "Feature extraction failed";
        success = false;
        setState(ENROLL_DONE);
        fpSensor->setLED(LED_ON, 0, LED_RED, 0);
        return false;
    }
    return true;
}

void EnrollmentManager::setState(EnrollState newState) {
    state = newState;
}

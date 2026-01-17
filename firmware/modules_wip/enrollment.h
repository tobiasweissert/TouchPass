// TouchPass Enrollment Module
// Manages fingerprint enrollment state machine

#ifndef TOUCHPASS_ENROLLMENT_H
#define TOUCHPASS_ENROLLMENT_H

#include <Arduino.h>
#include "fingerprint.h"
#include "storage.h"

enum EnrollState {
    ENROLL_IDLE,
    ENROLL_CAPTURE_1, ENROLL_LIFT_1,
    ENROLL_CAPTURE_2, ENROLL_LIFT_2,
    ENROLL_CAPTURE_3, ENROLL_LIFT_3,
    ENROLL_CAPTURE_4, ENROLL_LIFT_4,
    ENROLL_CAPTURE_5, ENROLL_LIFT_5,
    ENROLL_CAPTURE_6,
    ENROLL_MERGING,
    ENROLL_DONE
};

class EnrollmentManager {
public:
    EnrollmentManager(FingerprintSensor* sensor, TouchPassStorage* storage);

    // Start enrollment
    bool startEnrollment(const String& name, const String& password,
                         bool pressEnter, int fingerId);

    // Process enrollment (call in loop)
    void process();

    // Cancel enrollment
    void cancel();

    // Get status
    EnrollState getState();
    int getCurrentStep();
    bool isSuccess();
    String getError();
    String getStatusMessage();

    // Check if enrolling
    bool isEnrolling();

private:
    FingerprintSensor* fpSensor;
    TouchPassStorage* storage;

    EnrollState state;
    String pendingName;
    String pendingPassword;
    bool pendingPressEnter;
    int pendingFingerId;
    int16_t pendingSlot;
    bool success;
    String error;
    unsigned long timeout;

    // Helper methods
    bool captureToBuffer(uint8_t bufferNum);
    void setState(EnrollState newState);
};

#endif // TOUCHPASS_ENROLLMENT_H

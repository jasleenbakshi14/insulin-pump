#include "controliq.h"

ControlIQ::ControlIQ(InsulinPump *pump, Profile *profile, CGM *monitor)
    : insulinPump(pump), glucoseMonitor(monitor), running(false)
{
    currentProfile.store(profile);
}

ControlIQ::~ControlIQ() {
    stop(); // Ensure the thread stops if the object is destroyed
};

void ControlIQ::start() {
    if (running.load()) return;
    running = true;

    workerThread = std::thread(&ControlIQ::run, this);
};

void ControlIQ::stop() {
    if (running.load()) {
        running = false;
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }
};

void ControlIQ::run() {
    while(running.load()) {
        // Need to get glucose reading from CGM
        double glucoseLevel = glucoseMonitor->getGlucoseLevel();
        autoAdjustInsulinDelivery(glucoseLevel);

        // Instead of sleeping for 1 second in one go, break the sleep into 100-millisecond intervals.
        // This allows the loop to check the 'running' flag more frequently, so stop() can be more responsive.
        for (int i = 0; i < 10 && running.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};

void ControlIQ::setProfile(Profile *profile) {
    currentProfile.store(profile);
};

double ControlIQ::calculateCorrectionBolus(double currentGlucoseLevel) {
    return (currentGlucoseLevel - currentProfile.load()->targetGlucoseLevel) / currentProfile.load()->correctionFactor;
};

double ControlIQ::predictGlucoseLevel(double currentGlucoseLevel) {
     // Number between -0.5 and 1
    double random_number = (2.0 * std::rand() / RAND_MAX) - 0.5;

    return currentGlucoseLevel + random_number;
};

void ControlIQ::autoAdjustInsulinDelivery(double currentGlucoseLevel) {
    double predictedGlucoseLevel = predictGlucoseLevel(currentGlucoseLevel);
    
    if (predictedGlucoseLevel > 10.0) { 
        // deliver automatic correction
        double correctionBolus = calculateCorrectionBolus(currentGlucoseLevel);
        insulinPump->controlIQDeliver(correctionBolus);
    } else if (predictedGlucoseLevel > 8.9) { 
        // increase basel insulin
        double currentBasalRate = insulinPump->getBasalRate();
        double newBaselRate = currentBasalRate + 0.25;
        insulinPump->setBasalRate(newBaselRate);
    } else if (predictedGlucoseLevel > 6.25) { 
        // maintain active person profile settings
        insulinPump->setBasalRate(currentProfile.load()->basalRate);
    } else if (predictedGlucoseLevel > 3.9) { 
        // decrease basel insulin delivery
        double currentBasalRate = insulinPump->getBasalRate();
        if (currentBasalRate > 1) {
            double newBaselRate = currentBasalRate - 1;
            insulinPump->setBasalRate(newBaselRate);
        }
    } else { 
        // stop basal insulin delivery
        insulinPump->setBasalRate(0);
    }
};

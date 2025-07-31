#ifndef CONTROLIQ_H
#define CONTROLIQ_H

#include "insulinpump.h"
#include "profile.h"
#include "cgm.h"
#include <cstdlib>
#include <thread>
#include <atomic>
#include <chrono>

class ControlIQ {
    private:
        InsulinPump *insulinPump;
        CGM *glucoseMonitor;
        std::atomic<bool> running;
        std::atomic<Profile*> currentProfile;
        std::thread workerThread;

        void run();
        void autoAdjustInsulinDelivery(double currentGlucoseLevel);
        double predictGlucoseLevel(double currentGlucoseLevel);
        double calculateCorrectionBolus(double currentGlucoseLevel);
        
    public:
        ControlIQ(InsulinPump *pump, Profile *profile, CGM *monitor);
        ~ControlIQ();

        void start();
        void stop();
        void setProfile(Profile *profile);
};

#endif // CONTROLIQ_H
#ifndef CGM_H
#define CGM_H

#include <random>

// Continuous Glucose Monitor (Simulated)
class CGM { 
    private:
        double currentGlucose;
        double insulinCorrectionFactor;

        // Random number generation for simulation of glucose fluctuations.
        std::default_random_engine generator;
        std::normal_distribution<double> fluctuationDistribution;

    public:
        CGM(double correctionFactor);

        double getGlucoseLevel();
        void setCorrectionFactor(double correctionFactor);
        void readGlucose();
        void injectInsulin(double units);
};

#endif

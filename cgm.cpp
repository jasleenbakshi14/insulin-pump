#include "cgm.h"

CGM::CGM(double correctionFactor) : insulinCorrectionFactor(correctionFactor), fluctuationDistribution(0.0, 0.50) {
    generator.seed(std::random_device{}());
    // Generate a random initial glucose level between 4.0 and 9.0 mmol/L.
    std::uniform_real_distribution<double> initialGlucoseDistribution(4.0, 9.0);
    currentGlucose = initialGlucoseDistribution(generator);
    readGlucose();
};

void CGM::readGlucose() {
    double fluctuation = fluctuationDistribution(generator);
    if (fluctuation < -0.5) {
        fluctuation = fluctuation * -1;
    }
    currentGlucose += fluctuation;
};

void CGM::injectInsulin(double units) {
    currentGlucose -= units * insulinCorrectionFactor;
};

double CGM::getGlucoseLevel() {
    return currentGlucose;
};

void CGM::setCorrectionFactor(double correctionFactor) {
    insulinCorrectionFactor = correctionFactor;
};

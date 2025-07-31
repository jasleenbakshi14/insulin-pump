#include "insulinpump.h"
#include <QTime>

InsulinPump::InsulinPump(CGM *monitor) {
    insulinRemaining = 350.0;
    basalRate = 0.5;
    glucoseMonitor = monitor;
}

double InsulinPump::calculateBolus(double glucose, double carbs, double targetGlucose, double insulinSensitivity, double carbRatio) {
    if (insulinSensitivity <= 0 || carbRatio <= 0)
        return 0;

    double correctionDose = (glucose - targetGlucose) / insulinSensitivity;
    double carbDose = carbs / carbRatio;
    double totalDose = correctionDose + carbDose;
    return (totalDose > 0) ? totalDose : 0;
}

bool InsulinPump::administerInsulin(double dose, const QString &type) {
    if (dose <= 0.000 || dose > insulinRemaining)
        return false;

    insulinRemaining -= dose;
    logDelivery(dose, type);
    glucoseMonitor->injectInsulin(dose);
    return true;
}

void InsulinPump::logDelivery(double insulinAmount, const QString &type) {
    QString entry = QTime::currentTime().toString("hh:mm:ss") + ": " + type + " Delivered: " + QString::number(insulinAmount, 'f', 2) + " units";
    history.append(entry);
}

QString InsulinPump::getHistory() const {
    return history.join("\n");
}

void InsulinPump::setBasalRate(double rate) {
    basalRate = rate;
}

double InsulinPump::getBasalRate() {
    return basalRate;
}

void InsulinPump::refillCartridge() {
    insulinRemaining = 200.0;
    logDelivery(0, "Cartridge Refilled");
}

double InsulinPump::getInsulinRemaining() const {
    return insulinRemaining;
}
bool InsulinPump::controlIQDeliver(double units) {
    return administerInsulin(units, "ControlIQ Correction Bolus");
}

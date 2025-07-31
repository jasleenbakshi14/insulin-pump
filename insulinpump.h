#ifndef INSULINPUMP_H
#define INSULINPUMP_H

#include <QString>
#include <QStringList>
#include "cgm.h"

class InsulinPump {
private:
    double insulinRemaining;
    double basalRate;
    QStringList history;
    CGM *glucoseMonitor;

public:
    InsulinPump(CGM *monitor);

    double calculateBolus(double glucose, double carbs, double targetGlucose, double insulinSensitivity, double carbRatio);
    bool administerInsulin(double dose, const QString &type);

    void setBasalRate(double rate);
    double getBasalRate();

    void refillCartridge();
    double getInsulinRemaining() const;

    void logDelivery(double insulinAmount, const QString &type);
    QString getHistory() const;
    bool controlIQDeliver(double units);

};

#endif // INSULINPUMP_H

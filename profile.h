#ifndef PROFILE_H
#define PROFILE_H

#include <QString>
#include <QVector>

class Profile {
    public:

        Profile(QString _name, double _basalRate, double _carbohydrateRate, double _correctionFactor, double _targetGlucoseLevel);
        Profile(const QString& name, const QVector<double>& values)
            : name(name), values(values){}

        QString name;
        QVector<double> values;
        double basalRate = 0;
        double carbohydrateRate = 0;
        double correctionFactor = 0;
        double targetGlucoseLevel = 0;
};

#endif

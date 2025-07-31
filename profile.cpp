#include "profile.h"

Profile::Profile(QString _name, double _basalRate, double _carbohydrateRate, double _correctionFactor, double _targetGlucoseLevel)
    : name(_name),
      basalRate(_basalRate),
      carbohydrateRate(_carbohydrateRate),
      correctionFactor(_correctionFactor),
      targetGlucoseLevel(_targetGlucoseLevel) {};
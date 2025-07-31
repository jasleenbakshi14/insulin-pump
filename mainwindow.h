#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "insulinpump.h"
#include "cgm.h"
#include "profile.h"
#include "controliq.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void deliverBolus();
    void deliverBasalInsulin();
    void on_buttonUpdateBasal_clicked();
    void on_optionsButton_clicked();
    void on_bolusButton_clicked();
    void on_buttonBackFromOptions_clicked();
    void on_buttonBackFromBolus_clicked();
    void updateBatteryLabelColour();
    void getInfoFromInsulinPump();
    void showLog();


private:
    Ui::MainWindow *ui;
    int batteryLevel = 100;
    InsulinPump* pump;
    double insulinRemaining = 235.0;
    double basalRate;
    QTimer *basalTimer;
    CGM* cgm;
    QTimer* glucoseTimer;
    std::vector<Profile> profiles;
    bool warnedAt50 = false;
    bool warnedInsulinLow = false;
    bool isBatteryDead = false;
    ControlIQ* controlIQ;



    void updateGlucose();

    void saveHistoryToFile(const QString &entry);
    void loadHistoryFromFile();
    void controlIQDeliver(double units);
    void administerInsulin(double units);
    void updateGlucoseGraph(double newValue);
    void saveProfile();
    void loadProfile(const QString &name);
    void loadProfileFromDropdown();
    void drainBattery();
    void on_controlIQToggled(bool enabled);


};

#endif // MAINWINDOW_H

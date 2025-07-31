#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qcustomplot.h"
#include "cgm.h"
#include "profile.h"
#include "clickablelabel.h"
#include "controliq.h"

#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->batteryLabel, &ClickableLabel::clicked, this, [=]() {
        if (batteryLevel <= 50) {
            batteryLevel = 100;
            ui->batteryLabel->setText(QString::number(batteryLevel) + "%");
            QMessageBox::information(this, "Battery Recharged", "Battery recharged to 100%!");
        } else {
            QMessageBox::information(this, "Battery", "Battery is above 50%, recharge not needed.");
        }
    });

    // loadProfile();

    // Create a default profile
    Profile* defaultProfile = new Profile("Default", 5.0, 1.0, 1.0, 6.0);
    defaultProfile->values = QVector<double>{7.0, 5.0, 7.0, 1.0, 1.0};

    profiles.push_back(*defaultProfile);
    ui->comboBoxProfiles->addItem(defaultProfile->name);
    ui->comboBoxProfiles->setCurrentText("Default");
    loadProfile("Default");

    cgm = new CGM(defaultProfile->correctionFactor);
    pump = new InsulinPump(cgm);

    controlIQ = new ControlIQ(pump, defaultProfile, cgm);

    // TESTING
   // insulinRemaining = 12.0;

   /* QTimer *glucoseTimer = new QTimer(this);
    connect(glucoseTimer, &QTimer::timeout, this, [=]() {
        double simulatedGlucose = 5 + (qrand() % 1000) / 100.0; // random value between 5 and 15
        updateGlucoseGraph(simulatedGlucose);
    });
    glucoseTimer->start(2000); // update every 2 seconds
*/

    ui->glucoseGraph->addGraph();
    ui->glucoseGraph->xAxis->setLabel("Time (min)");
    ui->glucoseGraph->yAxis->setLabel("Blood Glucose (mmol/L)");
    ui->glucoseGraph->xAxis->setRange(0, 6 * 60);   // display 6 hours
    ui->glucoseGraph->yAxis->setRange(0, 20);   // glucose range

    // TEST
    // updateGlucoseGraph(8.4);

    loadHistoryFromFile();
    basalRate = pump->getBasalRate();

    connect(ui->buttonDeliver, &QPushButton::clicked, this, &MainWindow::deliverBolus);
    connect(ui->buttonUpdateBasal, &QPushButton::clicked, this, &MainWindow::on_buttonUpdateBasal_clicked);
    connect(ui->optionsButton, &QPushButton::clicked, this, &MainWindow::on_optionsButton_clicked);
    connect(ui->bolusButton, &QPushButton::clicked, this, &MainWindow::on_bolusButton_clicked);
    connect(ui->buttonBackFromOptions, &QPushButton::clicked, this, &MainWindow::on_buttonBackFromOptions_clicked);
    connect(ui->buttonBackFromBolus, &QPushButton::clicked, this, &MainWindow::on_buttonBackFromBolus_clicked);
    connect(ui->buttonSaveProfile, &QPushButton::clicked, this, &MainWindow::saveProfile);
    connect(ui->comboBoxProfiles, &QComboBox::currentTextChanged, this, &MainWindow::loadProfileFromDropdown);
    connect(ui->comboBoxProfiles, &QComboBox::currentTextChanged, this, &MainWindow::loadProfile);
    connect(ui->buttonViewLog, &QPushButton::clicked, this, &MainWindow::showLog);
    connect(ui->checkBoxControlIQ, &QCheckBox::toggled, this, &MainWindow::on_controlIQToggled);

    // Timer setup
    basalTimer = new QTimer(this);
    connect(basalTimer, &QTimer::timeout, this, &MainWindow::deliverBasalInsulin);
    basalTimer->start(2000);  // Every 2 seconds (simulates 10 minutes)

    glucoseTimer = new QTimer(this);
    connect(glucoseTimer, &QTimer::timeout, this, &MainWindow::updateGlucose);
    glucoseTimer->start(1000); // Every second (simulates 5 minutes)

    QTimer *batteryTimer = new QTimer(this);
    connect(batteryTimer, &QTimer::timeout, this, &MainWindow::drainBattery);
    batteryTimer->start(500); // Every half second (simulates 2.5 minutes)

    QTimer *pullInfoFromPumpTimer = new QTimer(this);
    connect(pullInfoFromPumpTimer, &QTimer::timeout, this, &MainWindow::getInfoFromInsulinPump);
    pullInfoFromPumpTimer->start(1000); // Every second (simulates 5 minutes)
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::deliverBolus()
{
    bool ok1, ok2, ok3, ok4, ok5;

    double glucose = ui->lineEditGlucose->text().toDouble(&ok1);
    double carbs = ui->lineEditCarbs->text().toDouble(&ok2);
    double target = ui->targetGlucoseInput_2->text().toDouble(&ok3);
    double isf = ui->isfInput_2->text().toDouble(&ok4);
    double cr = ui->crInput_2->text().toDouble(&ok5);

    if (!ok1 || !ok2 || !ok3 || !ok4 || !ok5) {
        QMessageBox::warning(this, "Invalid Input", "Please enter valid numbers in all fields.");
        return;
    }

    double insulinDose = pump->calculateBolus(glucose, carbs, target, isf, cr);

    if (insulinDose <= 0) {
        QMessageBox::information(this, "No Dose", "Calculated insulin dose is 0. Please check your values.");
        return;
    }

    if (QMessageBox::question(this, "Confirm Bolus",
        "Deliver " + QString::number(insulinDose, 'f', 2) + " units?",
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

        pump->administerInsulin(insulinDose, "Bolus");
        saveHistoryToFile("Bolus: Delivered " + QString::number(insulinDose, 'f', 2) + " units");
    }
}

void MainWindow::deliverBasalInsulin()
{
    double dose = pump->getBasalRate() / 12.0;
    if (insulinRemaining < dose) return;

    pump->administerInsulin(dose, "Basal");
    saveHistoryToFile("Basal: Delivered " + QString::number(dose, 'f', 2) + " units");
}

void MainWindow::on_buttonUpdateBasal_clicked()
{
    bool ok;
    double rate = ui->lineEditBasalRate->text().toDouble(&ok);
    if (ok && rate >= 0) {
        pump->setBasalRate(rate);
        ui->labelBasalStatus->setText("Basal rate updated to: " + QString::number(rate) + " u/hr");
    } else {
        QMessageBox::warning(this, "Invalid Input", "Please enter a valid non-negative number.");
    }
}

void MainWindow::on_optionsButton_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->optionsPage);
}

void MainWindow::on_bolusButton_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->bolusPage);
}

void MainWindow::on_buttonBackFromOptions_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->homePage);
}

void MainWindow::on_buttonBackFromBolus_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->homePage);
}

void MainWindow::saveHistoryToFile(const QString &entry)
{
    QString filePath = "insulin_history.txt";
    QFile file(filePath);

    qDebug() <<"Saving to file:" << QFileInfo(file).absoluteFilePath();

    if (file.open(QIODevice::Append | QIODevice::Text)) {
        qDebug() <<"Saving to file" << entry;
        QTextStream out(&file);
        out << QTime::currentTime().toString("hh:mm:ss") << ": " << entry << "\n";
        file.close();
    } else {
        qDebug() << "could not oopen file for writing!";
    }
}

void MainWindow::loadHistoryFromFile()
{
    QFile file("insulin_history.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString history = in.readAll();
        file.close();
    }
}

void MainWindow::updateGlucoseGraph(double newValue) {
    static QVector<double> xData, yData;
    static int time = 0;

    xData.append(time);
    yData.append(newValue);
    time += 5;

    if (xData.size() > (12 * 6)) {
        xData.remove(0);
        yData.remove(0);

        ui->glucoseGraph->xAxis->setRange(3 * 60, 9 * 60);
    }

    ui->glucoseGraph->graph(0)->setData(xData, yData);
    ui->glucoseGraph->replot();
}

void MainWindow::updateGlucose() {
    cgm->readGlucose();
    double newLevel = cgm->getGlucoseLevel();
    updateGlucoseGraph(newLevel);  // This updates the QCustomPlot
    ui->glucoseLabel->setText("Glucose: " + QString::number(newLevel, 'f', 1) + " mmol/L");

    /*
    if (newLevel < 3.5) {
        ui->labelStatus->setText("BG Critically Low!");
    } else if (newLevel > 13.0) {
        ui->labelStatus->setText("BG Critically High!");
    } else {
        ui->labelStatus->setText("Glucose stable.");
    } */
}

void MainWindow::saveProfile() {
    QString name = ui->lineEditName->text();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter a profile name.");
        return;
    }

    QVector<double> values = {
        ui->lineEditGlucose->text().toDouble(),
        ui->lineEditCarbs->text().toDouble(),
        ui->targetGlucoseInput_2->text().toDouble(),
        ui->isfInput_2->text().toDouble(),
        ui->crInput_2->text().toDouble()
    };

    profiles.push_back(Profile(name, values));
    if(ui->comboBoxProfiles->findText(name)== -1){
       ui->comboBoxProfiles->addItem(name);
    }
    QMessageBox::information(this, "Profile Saved", "Your profile has been saved!");
}

void MainWindow::loadProfile(const QString &name) {
    for (const Profile &p : profiles) {
        if (p.name == name) {

            ui->lineEditGlucose->setText(QString::number(p.values[0]));
            ui->lineEditCarbs->setText(QString::number(p.values[1]));
            ui->targetGlucoseInput_2->setText(QString::number(p.values[2]));
            ui->isfInput_2->setText(QString::number(p.values[3]));
            ui->crInput_2->setText(QString::number(p.values[4]));

            // Update the ControlIQ profile
            //controlIQ->setProfile(new Profile(p));
            break;
        }
    }
}


void MainWindow::loadProfileFromDropdown() {
    QString selectedName = ui->comboBoxProfiles->currentText();
    for (const Profile& p : profiles) {
        if (p.name == selectedName) {
            ui->targetGlucoseInput_2->setText(QString::number(p.targetGlucoseLevel));
            ui->isfInput_2->setText(QString::number(p.correctionFactor));
            ui->crInput_2->setText(QString::number(p.carbohydrateRate));
            break;
        }
    }
}

void MainWindow::drainBattery()
{
    batteryLevel -= 1;
    if (batteryLevel < 0) batteryLevel = 0;

    updateBatteryLabelColour();

    // Update battery label
    ui->batteryLabel->setText(QString::number(batteryLevel) + "%");

    // Show warning at 50%
    if (batteryLevel == 50 && !warnedAt50) {
        QMessageBox::warning(this, "Low Battery", "Battery is at 50%. Click the battery to recharge.");
        warnedAt50 = true;
    }

    // Warning at 20%
    if (batteryLevel == 20) {
        QMessageBox::warning(this, "Battery Critical", "Battery is at 20%!");
    }

    // Critical shutdown at 0%
    if (batteryLevel == 0 && !isBatteryDead) {
        isBatteryDead = true;

        QMessageBox::critical(this, "Battery Dead", "Battery has died. Simulation will now stop.");

        // Stop timers
        if (glucoseTimer) glucoseTimer->stop();
        if (basalTimer) basalTimer->stop();

        if (controlIQ) controlIQ->stop();

        // Disable relevant buttons
        ui->buttonDeliver->setEnabled(false);
        ui->buttonUpdateBasal->setEnabled(false);
        ui->bolusButton->setEnabled(false);
        ui->optionsButton->setEnabled(false);
    }
}


void MainWindow::updateBatteryLabelColour()
{
    QString color;
    if (batteryLevel > 50)
        color = "#55cc55";
    else if (batteryLevel > 20)
        color = "#ffaa00";
    else
        color = "#ff4444";

    ui->batteryLabel->setText(QString::number(batteryLevel) + "%");
    ui->batteryLabel->setStyleSheet("color: " + color + "; font-weight: bold;");
}

void MainWindow::getInfoFromInsulinPump()
{
    insulinRemaining = pump->getInsulinRemaining();
    basalRate = pump->getBasalRate();

    QString color;
    if (insulinRemaining > 50)
        color = "#55cc55"; // Green
    else if (insulinRemaining > 20)
        color = "#ffaa00"; // Orange
    else
        color = "#ff4444"; // Red

    ui->reservoirLabel->setText(QString::number(insulinRemaining, 'f', 1) + " u");
    ui->reservoirLabel->setStyleSheet("color: " + color + "; font-weight: bold;");

    ui->basalRateLabel->setText("Basal: " + QString::number(basalRate, 'f', 2) + " u/hr");

    if (insulinRemaining < 10 && !warnedInsulinLow) {
        QMessageBox::warning(this, "Low Insulin", "Insulin is running low!");
        warnedInsulinLow = true;
    }
}

void MainWindow::showLog() {
    QFile file("insulin_history.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Could not open log file.");
        return;
    }

    QTextStream in(&file);
    QString logContent = in.readAll();
    file.close();

    QMessageBox::information(this, "Insulin Log", logContent);
}

void MainWindow::on_controlIQToggled(bool enabled) {
    if (!controlIQ) return;

    if (enabled) {
        controlIQ->start();
        qDebug() << "Control-IQ Enabled";
    } else {
        controlIQ->stop();
        qDebug() << "Control-IQ Disabled";
    }
}

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QWidgetAction>
#include <QTimer>
#include <QDebug>
#include <QQueue>

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
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_btnSetFrequency_clicked();

    void on_btnSetAtten_clicked();

    void pollDeviceParameters();        // Функция для опроса параметров устройства

    void readResponse();


private:
    Ui::MainWindow *ui;
    QSerialPort serialPort;
    QTimer *statusTimer; // Добавляем таймер для опроса статуса

    uint8_t lastCommand;

    bool awaitingResponse = false;

    void sendCommand(uint8_t command, const QByteArray &data = QByteArray());

    void processStatusData(const QByteArray &data); // Обработка данных

    bool checkCommandResponse(const QByteArray &packet, uint8_t command);

    void handleErrorResponse(const QByteArray &packet);



};
#endif // MAINWINDOW_H

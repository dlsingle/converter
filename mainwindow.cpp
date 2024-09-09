#include "mainwindow.h"
#include "./ui_mainwindow.h"
#define CMD_POWER_ON          0x31
#define CMD_POWER_OFF         0x30
#define CMD_CHECK_LINK        0x01
#define CMD_SET_FREQ_MHz      0x32
#define CMD_SET_ATTEN         0x33
#define CMD_SET_BW            0x36
#define CMD_READ_STATUS       0x34
#define CMD_GET_DEVICE_INFO   0x06
#define SOF 0xBA
#define SRC 0x80
#define DST 0x7F
#define CMD_READ_ext_STATUS 0x38

#pragma pack(push, 1)

            typedef struct {
                    uint8_t res : 3;
                    uint8_t errPLL1 : 1;
                    uint8_t errPLL2 : 1;
                    uint8_t errPLLrefGen : 1;
                    uint8_t errConfPLLrefGen : 1;
            } flagError_t;

            typedef struct {
                    uint8_t res : 6;
                    uint8_t stateLNA : 1;
                    uint8_t statePower : 1;
            } flagState_t;

            typedef struct {
                    uint8_t typePckg;         // 0xba
                    uint8_t id;               // 0x01
                    uint8_t unknown1;         // 0x80
                    uint8_t unknown2;         // 0x19
                    uint8_t unknown3;         // 0x8A
                    uint8_t unknown4;         // 0x1D
                    uint8_t unknown5;         // 0x00
                    flagState_t flagState;    // 0x00 и 0x80
                    flagError_t flagError;    // 0x00
                    uint16_t u_ext;           // 0x0000
                    uint8_t u5v;              // 0x00
                    uint16_t f_c;             // 0x03e8 и задается
                    uint8_t att;              // 0x00 и задается
                    int8_t temp;              // 0x1e
                    uint16_t res;
                    uint16_t u20v;
                    uint16_t i_supply;
                    uint16_t i_supplyAmp;
            } status_t;

#pragma pack(pop)

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    ,statusTimer(new QTimer(this)) // Инициализируем таймер
{
    ui->setupUi(this);

    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports)
    {
        ui->comboBox->addItem(port.portName());
    }
    // Заполнение списка доступных скоростей
    ui->comboBox_2->addItem("9600", QSerialPort::Baud9600);
    ui->comboBox_2->addItem("19200", QSerialPort::Baud19200);
    ui->comboBox_2->addItem("38400", QSerialPort::Baud38400);
    ui->comboBox_2->addItem("57600", QSerialPort::Baud57600);
    ui->comboBox_2->addItem("115200", QSerialPort::Baud115200);

    // Использование числовых значений для скоростей выше 115200
    ui->comboBox_2->addItem("230400", 230400);
    ui->comboBox_2->addItem("460800", 460800);
    ui->comboBox_2->addItem("921600", 921600);



    // Создаем первую кнопку
    QPushButton *setButton1 = new QPushButton("Set");

    // Создаем действие для первой кнопки и добавляем кнопку в это действие
    QWidgetAction *buttonAction1 = new QWidgetAction(ui->lineEdit_1);
    buttonAction1->setDefaultWidget(setButton1);

    // Добавляем действие (кнопку) внутрь первого QLineEdit справа
    ui->lineEdit_1->addAction(buttonAction1, QLineEdit::TrailingPosition);

    // Создаем вторую кнопку
    QPushButton *setButton2 = new QPushButton("Set");

    // Создаем действие для второй кнопки и добавляем кнопку в это действие
    QWidgetAction *buttonAction2 = new QWidgetAction(ui->lineEdit_2);
    buttonAction2->setDefaultWidget(setButton2);

    // Добавляем действие (кнопку) внутрь второго QLineEdit справа
    ui->lineEdit_2->addAction(buttonAction2, QLineEdit::TrailingPosition);

    // Связываем нажатие первой кнопки с on_btnSetFrequency_clicked
    connect(setButton1, &QPushButton::clicked, this, &MainWindow::on_btnSetFrequency_clicked);

    // Связываем нажатие первой кнопки с on_btnSetAtten_clicked
    connect(setButton2, &QPushButton::clicked, this, &MainWindow::on_btnSetAtten_clicked);

    ui->comboBox_5->setCurrentIndex(-1);

    ui->comboBox_5->setEditable(true);

    QLineEdit *lineEdit = ui->comboBox_5->lineEdit();

    lineEdit->setPlaceholderText("<MHz>");
    QFont font = lineEdit->font();
    font.setPointSize(11);
    lineEdit->setFont(font);
    lineEdit->setAlignment(Qt::AlignCenter);

    ui->widget_2->setEnabled(false);

    connect(statusTimer, &QTimer::timeout, this, &MainWindow::pollDeviceParameters);
    statusTimer->setInterval(1000); // Устанавливаем интервал в 1 секунду
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    sendCommand(CMD_POWER_ON);
}


void MainWindow::on_pushButton_2_clicked()
{
    sendCommand(CMD_POWER_OFF);
}

void MainWindow::on_btnSetFrequency_clicked()
{
    bool ok;
        uint16_t frequency = ui->lineEdit_1->text().toUInt(&ok);

        if (!ok || frequency == 0) // Проверяем, если преобразование не удалось или частота равна 0
        {
            // Выводим сообщение об ошибке, если частота недопустима
            ui->listWidget->addItem(">Ошибка: Неверная частота. Пожалуйста, введите допустимое значение частоты.");
            return;
        }

        // Создаем пакет данных для настройки частоты
        QByteArray data;

        // Добавляем частоту в формате little-endian (2 байта)
        data.append(static_cast<char>(frequency & 0xFF)); // Младший байт
        data.append(static_cast<char>((frequency >> 8) & 0xFF)); // Старший байт

        // Отправляем команду с собранными данными
        sendCommand(CMD_SET_FREQ_MHz, data);

}

void MainWindow::on_btnSetAtten_clicked()
{
    bool ok;
    uint8_t atten = ui->lineEdit_2->text().toUInt(&ok);
    if (!ok || atten == 0) // Проверяем, если преобразование не удалось или частота равна 0
    {
        // Выводим сообщение об ошибке, если частота недопустима
        ui->listWidget->addItem(">Ошибка: Неверный аттенюатор. Пожалуйста, введите допустимое значение аттенюатора.");
        return;
    }
    QByteArray data;
    data.append(atten);
    sendCommand(CMD_SET_ATTEN, data);
}



void MainWindow::on_pushButton_3_clicked()
{
    if (serialPort.isOpen()) {
            serialPort.close();
            ui->pushButton_3->setText("Подключиться");
            ui->widget_2->setEnabled(false);
            ui->comboBox->setEnabled(true);
            ui->comboBox_2->setEnabled(true);
            ui->listWidget->addItem(">Инфо: Порт закрыт.");
            statusTimer->stop();
            return;
        }
    serialPort.setPortName(ui->comboBox->currentText());
    serialPort.setBaudRate(ui->comboBox_2->currentData().toInt());
    serialPort.setDataBits(QSerialPort::Data8); // Устанавливает количество бит данных в каждом байте.
    serialPort.setParity(QSerialPort::NoParity); // Устанавливает метод проверки четности.
    serialPort.setStopBits(QSerialPort::OneStop); // Устанавливает количество стоп-бит.
    serialPort.setFlowControl(QSerialPort::NoFlowControl); // Устанавливает управление потоком данных.

    if (serialPort.open(QIODevice::ReadWrite)) {
            ui->pushButton_3->setText("Отключиться");
            ui->comboBox->setEnabled(false);
            ui->comboBox_2->setEnabled(false);
            ui->listWidget->addItem(">Инфо: Порт открыт успешно.");
            connect(&serialPort, &QSerialPort::readyRead, this, &MainWindow::readResponse);  // Добавлен сигнал для чтения ответа
            sendCommand(CMD_CHECK_LINK);
        } else {
            ui->listWidget->addItem(">Ошибка: Не удалось открыть порт.");
        }
}

void MainWindow::pollDeviceParameters()
{
    // Отправляем команду для запроса статуса устройства
    sendCommand(CMD_READ_STATUS );
}


void MainWindow::processStatusData(const QByteArray &data)
{
    if (data.size() >= sizeof(status_t)) {
            status_t status;
            memcpy(&status, data.constData(), sizeof(status_t));

            ui->tableWidget->setItem(0, 1, new QTableWidgetItem(status.flagState.statePower  ? "Включено" : "Выключено"));
            ui->tableWidget->setItem(1, 1, new QTableWidgetItem(QString::number(status.temp)));
            ui->tableWidget->setItem(2, 1, new QTableWidgetItem(QString::number(status.f_c)));
            ui->tableWidget->setItem(3, 1, new QTableWidgetItem(QString::number(status.att)));
            ui->tableWidget->setItem(4, 1, new QTableWidgetItem(QString::number(status.res)));
        }
        else
        {
            ui->listWidget->addItem(">Ошибка: Неверный размер пакета данных.");
        }

}

void MainWindow::sendCommand(uint8_t command, const QByteArray &data)
{
    if (awaitingResponse) {
            ui->listWidget->addItem(">Ошибка: Ожидание ответа от устройства. Невозможно отправить новую команду.");
            return;
        }

        awaitingResponse = true;  // Устанавливаем флаг состояния ожидания
        lastCommand = command;
        QByteArray packet;

        packet.append(SOF);
        packet.append(SRC);
        packet.append(DST);
        uint8_t length = static_cast<uint8_t>(1 + data.size() + 1);
        packet.append(length);
        packet.append(command);
        packet.append(data);

        // Вычисление контрольной суммы
        uint8_t checksum = 0;
        for (int i = 1; i < packet.size(); ++i) {
            checksum ^= static_cast<uint8_t>(packet[i]);
        }

        packet.append(checksum);

        serialPort.write(packet);
        qDebug() << "Отправлен пакет:" << packet.toHex();

    }


void MainWindow::readResponse()
{
    QByteArray packet = serialPort.readAll();  // Считываем все доступные данные

        if (packet.isEmpty()) {
            ui->listWidget->addItem(">Ошибка: Пустой ответ от устройства.");
            awaitingResponse = false;
            return;
        }

        qDebug() << "Получен пакет:" << packet.toHex();

        // Проверяем корректность ответа
        if (packet.size() < 6) {  // Минимальный размер для корректного ответа
            ui->listWidget->addItem(">Ошибка: Некорректный размер ответа.");
            awaitingResponse = false;
            return;
        }

        // Проверка контрольной суммы
        uint8_t checksum = 0;
        for (int i = 1; i < packet.size() - 1; ++i) {
            checksum ^= static_cast<uint8_t>(packet[i]);
        }

        if (checksum != static_cast<uint8_t>(packet[packet.size() - 1])) {
            ui->listWidget->addItem(">Ошибка: Некорректная контрольная сумма.");
            awaitingResponse = false;
            return;
        }

        if (lastCommand == CMD_READ_STATUS) {
            processStatusData(packet);
        } else if (checkCommandResponse(packet, lastCommand)) {
            ui->listWidget->addItem(">Команда успешно выполнена.");
            statusTimer->start();
            ui->widget_2->setEnabled(true);
        } else {
            ui->listWidget->addItem(">Ошибка: Некорректный ответ от устройства.");
        }

        awaitingResponse = false;

}

bool MainWindow::checkCommandResponse(const QByteArray &packet, uint8_t command)
{
    if (packet.size() < 6) return false; // Минимальный размер для корректного ответа

    uint8_t responseType = static_cast<uint8_t>(packet[4]);
    uint8_t responseCode = static_cast<uint8_t>(packet[5]);

    if (responseType == 0x80 && responseCode == command) {
        return true;
    } else if (responseType == 0xFF) {
        handleErrorResponse(packet);
        return false;
    }
    return false;
}

void MainWindow::handleErrorResponse(const QByteArray &packet)
{
    uint8_t errorCode = static_cast<uint8_t>(packet[6]);
    switch (errorCode) {
        case 0x51: ui->listWidget->addItem(">Неправильная длина команды "); break;
        case 0x52: ui->listWidget->addItem(">Ошибка DataFlash памяти "); break;
        case 0x53: ui->listWidget->addItem(">Некорректный размер файла "); break;
        case 0x54: ui->listWidget->addItem(">Некорректный номер блока файла "); break;
        case 0x55: ui->listWidget->addItem(">Некорректный параметр команды "); break;
        case 0x56: ui->listWidget->addItem(">Ошибка выполнения команды из-за аппаратной неисправности."); break;
        case 0x57: ui->listWidget->addItem(">Операция не поддерживается (неизвестная команда) "); break;
        case 0x58: ui->listWidget->addItem(">Команда недопустима для данного режима работы "); break;
        case 0x59: ui->listWidget->addItem(">Устройство занято, команда игнорирована "); break;
        case 0x5B: ui->listWidget->addItem(">Ошибка контрольной суммы пакета "); break;
        default: ui->listWidget->addItem(">Неизвестная ошибка "); break;
    }
}





#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QPainter>
#include <QTimer>
#include <QPen>
#include <string>
#include <cmath>

struct TelemetryData {
    int battery = 0;
    int altitude = 0;
    bool is_valid = false;
};

// Наш перевірений безпечний парсер
TelemetryData parseTelemetrySafe(const std::string& data) {
    TelemetryData result;
    size_t bat_pos = data.find("BAT:");
    size_t alt_pos = data.find("ALT:");

    if (bat_pos == std::string::npos || alt_pos == std::string::npos) {
        result.is_valid = false;
        return result;
    }

    size_t semi_pos1 = data.find(";", bat_pos);
    std::string bat_str = data.substr(bat_pos + 4, semi_pos1 - (bat_pos + 4));
    result.battery = std::stoi(bat_str);

    size_t semi_pos2 = data.find(";", alt_pos);
    std::string alt_str = data.substr(alt_pos + 4, semi_pos2 - (alt_pos + 4));
    result.altitude = std::stoi(alt_str);

    result.is_valid = true;
    return result;
}


// Клас, який відповідає за малювання прицілу
class HudWidget : public QWidget {
    Q_OBJECT
public:
    HudWidget(QWidget *parent = nullptr) : QWidget(parent) {
        // Встановлюємо мінімальний розмір для нашого екрану HUD
        setMinimumSize(300, 200);
        // Створюємо таймер, який буде працювати в фоні
        QTimer *timer = new QTimer(this);
        // З'єднуємо сигнал таймера timeout() з методом update() нашого віджета.
        // Метод update() каже Qt: "Перемалюй цей віджет при першій можливості" (викликає paintEvent).
        connect(timer, &QTimer::timeout, this, QOverload<>::of(&QWidget::update));
        // Запускаємо таймер з інтервалом 30 мілісекунд (приблизно 33 кадри на секунду)
        timer->start(30);        
    }

protected:
    // Цей метод викликається автоматично, коли Qt малює віджет
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event); // Кажемо компілятору, що ми не використовуємо змінну event

        QPainter painter(this);
        // Вмикаємо згладжування (Antialiasing), щоб лінії були красивими і не "зубчастими"
        painter.setRenderHint(QPainter::Antialiasing);

        // Отримуємо центр нашого віджета
        int centerX = width() / 2;
        int centerY = height() / 2;

        // Налаштовуємо "олівець" (колір Flat UI Emerald, товщина 2 пікселі)
        QPen pen(QColor(46, 204, 113), 2); 
        painter.setPen(pen);

        // 1. Малюємо центральне коло візиру (радіус 40 пікселів)
        painter.drawEllipse(QPoint(centerX, centerY), 40, 40);

        // 2. Малюємо лінії перехрестя
        // Горизонтальна лінія (від -60 до +60 пікселів від центру)
        painter.drawLine(centerX - 60, centerY, centerX + 60, centerY);
        // Вертикальна лінія
        painter.drawLine(centerX, centerY - 60, centerX, centerY + 60);

        // --- ДИНАМІЧНИЙ ТРЕКІНГ ---
        // Використовуємо час для створення плавної траєкторії руху.
        // static_cast<double> потрібен для точних математичних обчислень.
        static double time = 0.0;
        time += 0.05; // Швидкість руху

        // Вираховуємо зміщення рамки по синусоїді та косинусоїді (рух по колу)
        int offsetX = static_cast<int>(30.0 * sin(time));
        int offsetY = static_cast<int>(20.0 * cos(time));        

        // Малюємо рамку фокусування об'єкта, яка зміщена на offsetX та offsetY
        painter.drawRect(centerX - 20 + offsetX, centerY - 20 + offsetY, 40, 40);
    }
};

// Створюємо наш власний клас вікна
class MainWindow : public QWidget {
    Q_OBJECT // Обов'язковий макрос Qt для роботи сигналів/слотів

public:
    MainWindow(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowTitle("FalconEye Ground Control Station");
        resize(400, 250);

        // Створюємо елементи інтерфейсу
        titleLabel = new QLabel("=== FalconEye GCS ===", this);
        titleLabel->setStyleSheet("font-weight: bold; font-size: 16px; margin-bottom: 10px;");
        titleLabel->setAlignment(Qt::AlignCenter);

        batteryLabel = new QLabel("Battery: -- %", this);
        batteryLabel->setStyleSheet("font-size: 14px; color: #27ae60; font-weight: bold;");

        altitudeLabel = new QLabel("Altitude: -- meters", this);
        altitudeLabel->setStyleSheet("font-size: 14px; color: #2980b9; font-weight: bold;");

        statusLabel = new QLabel("Status: Waiting for drone...", this);
        statusLabel->setStyleSheet("font-size: 12px; color: #7f8c8d;");

        // Створюємо наш приціл
        hudWidget = new HudWidget(this);

        // Розміщуємо їх у вікні вертикально
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(titleLabel);
        layout->addWidget(hudWidget);
        layout->addWidget(batteryLabel);
        layout->addWidget(altitudeLabel);
        layout->addWidget(statusLabel);

        // Налаштовуємо UDP сокет через Qt
        udpSocket = new QUdpSocket(this);
        // bind на порт 5005 (слухаємо локальний хост)
        udpSocket->bind(QHostAddress::LocalHost, 5005);

        // З'ЄДНУЄМО СИГНАЛ ТА СЛОТ:
        // Коли сокет отримує пакет, він випускає сигнал readyRead.
        // Ми кажемо Qt викликати наш метод (слот) readPendingDatagrams.
        connect(udpSocket, &QUdpSocket::readyRead, this, &MainWindow::readPendingDatagrams);
    }

private slots:
    // Наш слот для обробки вхідних пакетів
    void readPendingDatagrams() {
        // Поки в сокеті є непрочитані пакети
        while (udpSocket->hasPendingDatagrams()) {
            // Отримуємо пакет через Qt
            QNetworkDatagram datagram = udpSocket->receiveDatagram();
            // Перетворюємо байти в std::string
            std::string raw_data = datagram.data().toStdString();

            TelemetryData t = parseTelemetrySafe(raw_data);
            if (t.is_valid == true) {
                batteryLabel->setText("Battery: " + QString::number(t.battery) + " %");
                altitudeLabel->setText("Altitude: " + QString::number(t.altitude) + " meters");
                statusLabel->setText("Status: Connected");
            } else {
                statusLabel->setText("Status: [WARNING] Signal interference!");
            }
        }
    }

private:
    QLabel *titleLabel;
    QLabel *batteryLabel;
    QLabel *altitudeLabel;
    QLabel *statusLabel;
    QUdpSocket *udpSocket;
    HudWidget *hudWidget;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}

// Цей рядок обов'язковий, якщо ми пишемо клас з Q_OBJECT прямо в main.cpp.
// Він каже компілятору підключити згенерований Qt код сигналів/слотів.
#include "main.moc"
// main.cpp
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
    int roll = 0;
    int pitch = 0;    
    bool is_valid = false;
};

// Допоміжна функція для безпечного витягування цілого числа за ключем.
// Параметр 'ok' передається по посиланню (bool&), щоб ми могли повернути статус успіху.
int extractValue(const std::string& data, const std::string& key, bool& ok) {
    size_t key_pos = data.find(key);
    if (key_pos == std::string::npos) {
        ok = false;
        return 0;
    }

    size_t semi_pos = data.find(";", key_pos);
    // Якщо крапку з комою не знайдено, ми беремо кінець рядка як роздільник!
    if (semi_pos == std::string::npos) {
        semi_pos = data.length(); 
    }

    size_t key_len = key.length();
    // Вирізаємо підрядок між ключем і крапкою з комою
    std::string val_str = data.substr(key_pos + key_len, semi_pos - (key_pos + key_len));

    // Безпечно конвертуємо в число. 
    // try-catch захистить нас, якщо замість числа прилетить сміття (наприклад, "BAT:abc;")
    try {
        ok = true;
        return std::stoi(val_str);
    } catch (...) {
        ok = false;
        return 0;
    }
}

// Наш перевірений безпечний парсер
TelemetryData parseTelemetrySafe(const std::string& data) {
    TelemetryData result;
    bool ok = true;

    // Витягуємо батарею
    result.battery = extractValue(data, "BAT:", ok);
    if (!ok) { result.is_valid = false; return result; }

    // Витягуємо висоту
    result.altitude = extractValue(data, "ALT:", ok);
    if (!ok) { result.is_valid = false; return result; }

    // Витягуємо крен (Roll)
    result.roll = extractValue(data, "ROLL:", ok);
    if (!ok) { result.is_valid = false; return result; }

    // Витягуємо тангаж (Pitch)
    result.pitch = extractValue(data, "PITCH:", ok);
    if (!ok) { result.is_valid = false; return result; }

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

    // Метод для оновлення висоти з головного вікна
    void setAltitude(int alt) {
        m_altitude = alt;
    }

    void setAttitude(int roll, int pitch) {
        m_roll = roll;
        m_pitch = pitch;
    }    

protected:
    // Цей метод викликається автоматично, коли Qt малює віджет
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // --- 1. МАЛЮЄМО СІТКУ ЗЕМЛІ (СИМУЛЯЦІЯ КАМЕРИ) ---
        QPen gridPen(QColor(127, 140, 141, 80), 1); 
        painter.setPen(gridPen);

        static double gridOffset = 0.0;
        double speed = 0.0;
        if (m_altitude > 0) {
            speed = 150.0 / m_altitude; 
        }
        
        gridOffset += speed;
        if (gridOffset >= 40.0) {
            gridOffset = 0.0;
        }

        for (int x = 0; x < width(); x += 40) {
            painter.drawLine(x, 0, x, height());
        }
        for (int y = static_cast<int>(gridOffset); y < height(); y += 40) {
            painter.drawLine(0, y, width(), y);
        }

        int centerX = width() / 2;
        int centerY = height() / 2;


        // --- 2. МАЛЮЄМО ШТУЧНИЙ ГОРИЗОНТ (Attitude Indicator) ---
        // Зберігаємо поточний стан художника (координати за замовчуванням)
        painter.save();

        // Налаштовуємо синій олівець для лінії горизонту
        QPen horizonPen(QColor(41, 128, 185), 2); 
        painter.setPen(horizonPen);

        // Переносимо центр координат у центр нашого віджета
        painter.translate(centerX, centerY);
        
        // Зміщуємо по вертикалі залежно від тангажу (Pitch).
        // Множимо на 3, щоб рух був більш помітним на екрані.
        painter.translate(0, m_pitch * 3);

        // Повертаємо систему координат на кут крен (Roll)
        painter.rotate(m_roll);

        // Малюємо лінію горизонту (від -100 до +100 пікселів відносно нового центру)
        painter.drawLine(-100, 0, 100, 0);
        
        // Малюємо маленькі засічки на лінії горизонту (шкала нахилу)
        painter.drawLine(-50, -10, -50, 10);
        painter.drawLine(50, -10, 50, 10);

        // Відновлюємо стан художника (повертаємо координати на місце)
        painter.restore();


        // --- 3. МАЛЮЄМО СТАТИЧНИЙ ВІЗИР (ПРИЦІЛ) ---
        // Він залишається по центру і НЕ крутиться, бо ми зробили painter.restore()!
        QPen pen(QColor(46, 204, 113), 2); 
        painter.setPen(pen);

        // Малюємо центральне коло візира
        painter.drawEllipse(QPoint(centerX, centerY), 40, 40);

        // Малюємо лінії перехрестя
        painter.drawLine(centerX - 60, centerY, centerX + 60, centerY);
        painter.drawLine(centerX, centerY - 60, centerX, centerY + 60);


        // --- 4. ДИНАМІЧНИЙ ТРЕКІНГ (РАМКА ФОКУСУВАННЯ) ---
        // Замість синуса, давай прив'яжемо рух рамки до реальних нахилів дрона!
        // Це покаже, як рамка реально зміщується при маневрах.
        int offsetX = m_roll * 2;
        int offsetY = m_pitch * 2;

        painter.drawRect(centerX - 20 + offsetX, centerY - 20 + offsetY, 40, 40);
    }

private:
    int m_altitude = 100; // Висота за замовчуванням
    int m_roll = 0;
    int m_pitch = 0;    
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

                // Передаємо нову висоту в наш HUD-віджет
                hudWidget->setAltitude(t.altitude);
                hudWidget->setAttitude(t.roll, t.pitch); 
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
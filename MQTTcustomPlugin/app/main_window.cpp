#include "main_window.hpp"

#include "connection_panel.hpp"
#include "device_dashboard.hpp"
#include "message_log.hpp"
#include "publish_panel.hpp"
#include "subscription_panel.hpp"

#include <QCoreApplication>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("RamMQTT — Smart Bed Lab");
    resize(1280, 800);
    setMinimumSize(960, 640);

    mqtt_client_ = std::make_unique<ramai::MqttClient>();
    mqtt_client_->set_message_handler([this](const ramai::MqttMessage& message) {
        handleIncomingMessage(message);
    });
    loadConfiguration();

    auto* central = new QWidget(this);
    auto* root_layout = new QVBoxLayout(central);

    connection_panel_ = new ConnectionPanel(central);
    device_dashboard_ = new DeviceDashboard(central);

    auto* middle_splitter = new QSplitter(Qt::Horizontal, central);
    subscription_panel_ = new SubscriptionPanel(middle_splitter);
    message_log_ = new MessageLog(middle_splitter);
    middle_splitter->addWidget(subscription_panel_);
    middle_splitter->addWidget(message_log_);
    middle_splitter->setStretchFactor(0, 1);
    middle_splitter->setStretchFactor(1, 3);

    publish_panel_ = new PublishPanel(central);

    root_layout->addWidget(connection_panel_);
    root_layout->addWidget(device_dashboard_);
    root_layout->addWidget(middle_splitter, 1);
    root_layout->addWidget(publish_panel_);
    setCentralWidget(central);

    poll_timer_ = new QTimer(this);
    connect(poll_timer_, &QTimer::timeout, this, &MainWindow::onPollMessages);

    connect(connection_panel_, &ConnectionPanel::connectRequested, this, &MainWindow::onConnectRequested);
    connect(connection_panel_, &ConnectionPanel::disconnectRequested, this, &MainWindow::onDisconnectRequested);
    connect(subscription_panel_, &SubscriptionPanel::subscribeRequested, this, &MainWindow::onSubscribeRequested);
    connect(subscription_panel_, &SubscriptionPanel::unsubscribeRequested, this, &MainWindow::onUnsubscribeRequested);
    connect(subscription_panel_, &SubscriptionPanel::smartBedPresetsRequested, this, &MainWindow::onSmartBedPresetsRequested);
    connect(publish_panel_, &PublishPanel::publishRequested, this, &MainWindow::onPublishRequested);
    connect(message_log_, &MessageLog::exportRequested, this, &MainWindow::onExportLogRequested);

    if (!app_config_.profiles.empty()) {
        connection_panel_->setProfile(app_config_.profiles.front());
        active_profile_ = app_config_.profiles.front();
    }
}

MainWindow::~MainWindow() {
    if (mqtt_client_) {
        mqtt_client_->disconnect();
    }
}

void MainWindow::loadConfiguration() {
    if (!load_app_config(configPath().toStdString(), app_config_)) {
        QMessageBox::warning(this, "Configuration",
                             "Could not load config/default_profile.json. Using built-in defaults.");
        active_profile_.name = "Lab HA Broker";
        active_profile_.host = "10.20.25.40";
        active_profile_.port = 1883;
        active_profile_.client_id = "RamMQTT";
        active_profile_.preset_subscriptions = {
            "smartbed/sensor/#",
            "funhouse/#",
            "degenstation/#",
        };
        app_config_.profiles.push_back(active_profile_);
    }

    device_registry_.load_mappings(app_config_.device_mappings);
}

void MainWindow::onConnectRequested(const ramai::BrokerProfile& profile) {
    active_profile_ = profile;
    device_dashboard_->clearReadings();

    mqtt_client_->connect(profile, [this](bool connected, const std::string& detail) {
        setConnectionState(connected, QString::fromStdString(detail));
    });

    if (mqtt_client_->is_connected()) {
        poll_timer_->start(100);
        for (const auto& topic : profile.preset_subscriptions) {
            mqtt_client_->subscribe(topic, 0);
            subscription_panel_->addSubscription(QString::fromStdString(topic));
        }
    }
}

void MainWindow::onDisconnectRequested() {
    poll_timer_->stop();
    mqtt_client_->disconnect();
    setConnectionState(false, "Disconnected");
}

void MainWindow::onSubscribeRequested(const QString& topic) {
    mqtt_client_->subscribe(topic.toStdString(), 0);
    subscription_panel_->addSubscription(topic);
}

void MainWindow::onUnsubscribeRequested(const QString& topic) {
    mqtt_client_->unsubscribe(topic.toStdString());
}

void MainWindow::onPublishRequested(const ramai::MqttMessage& message) {
    mqtt_client_->publish(message);
    publish_panel_->rememberTopic(QString::fromStdString(message.topic));
    handleIncomingMessage(message);
}

void MainWindow::onSmartBedPresetsRequested() {
    for (const auto& topic : active_profile_.preset_subscriptions) {
        onSubscribeRequested(QString::fromStdString(topic));
    }
}

void MainWindow::onPollMessages() {
    mqtt_client_->drain_incoming_messages();
}

void MainWindow::onExportLogRequested() {
    const QString path = QFileDialog::getSaveFileName(this, "Export Message Log", "rammqtt_log.csv", "CSV (*.csv)");
    if (path.isEmpty()) {
        return;
    }

    if (!message_log_->exportToCsv(path)) {
        QMessageBox::warning(this, "Export Failed", "Could not write CSV file.");
    }
}

void MainWindow::handleIncomingMessage(const ramai::MqttMessage& message) {
    message_log_->appendMessage(message);
    publish_panel_->rememberTopic(QString::fromStdString(message.topic));

    if (const auto reading = device_registry_.parse_message(message)) {
        device_dashboard_->updateReading(*reading);
    }
}

void MainWindow::setConnectionState(bool connected, const QString& detail) {
    connection_panel_->setConnected(connected, detail);
    if (!connected) {
        poll_timer_->stop();
    }
}

QString MainWindow::configPath() const {
    const QString config_dir = QCoreApplication::applicationDirPath() + "/config";
    const QString local_profile = config_dir + "/local_profile.json";
    if (QFile::exists(local_profile)) {
        return local_profile;
    }
    return config_dir + "/default_profile.json";
}
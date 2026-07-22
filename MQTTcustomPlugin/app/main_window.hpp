#pragma once

#include "ramai/config.hpp"
#include "ramai/device_registry.hpp"
#include "ramai/mqtt_client.hpp"

#include <QMainWindow>
#include <memory>

class ConnectionPanel;
class SubscriptionPanel;
class MessageLog;
class PublishPanel;
class DeviceDashboard;
class QTimer;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onConnectRequested(const ramai::BrokerProfile& profile);
    void onDisconnectRequested();
    void onSubscribeRequested(const QString& topic);
    void onUnsubscribeRequested(const QString& topic);
    void onPublishRequested(const ramai::MqttMessage& message);
    void onSmartBedPresetsRequested();
    void onPollMessages();
    void onExportLogRequested();

private:
    void loadConfiguration();
    void handleIncomingMessage(const ramai::MqttMessage& message);
    void setConnectionState(bool connected, const QString& detail);
    QString configPath() const;

    std::unique_ptr<ramai::MqttClient> mqtt_client_;
    ramai::AppConfig app_config_;
    ramai::DeviceRegistry device_registry_;
    ramai::BrokerProfile active_profile_;

    ConnectionPanel* connection_panel_ = nullptr;
    SubscriptionPanel* subscription_panel_ = nullptr;
    MessageLog* message_log_ = nullptr;
    PublishPanel* publish_panel_ = nullptr;
    DeviceDashboard* device_dashboard_ = nullptr;
    QTimer* poll_timer_ = nullptr;
};
#pragma once

#include "ramai/message.hpp"

#include <QWidget>

#include <map>
#include <string>

class QTreeWidget;

class DeviceDashboard : public QWidget {
    Q_OBJECT

public:
    explicit DeviceDashboard(QWidget* parent = nullptr);

    void updateReading(const ramai::SensorReading& reading);
    void clearReadings();

private:
    struct SensorState {
        QString label;
        QString value;
        QString unit;
        QString updated_at;
    };

    void ensureDeviceNode(const QString& device_id, const QString& device_label);
    void refreshDeviceNode(const QString& device_id);

    QTreeWidget* tree_ = nullptr;
    std::map<std::string, std::map<std::string, SensorState>> device_readings_;
    std::map<std::string, QString> device_labels_;
};
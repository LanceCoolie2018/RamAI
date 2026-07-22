#include "device_dashboard.hpp"

#include <QLabel>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <chrono>

namespace {

QString formatTimestamp(const std::chrono::system_clock::time_point& time_point) {
    const auto time_t_value = std::chrono::system_clock::to_time_t(time_point);
    std::tm local_tm{};
#if defined(_WIN32)
    localtime_s(&local_tm, &time_t_value);
#else
    localtime_r(&time_t_value, &local_tm);
#endif
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &local_tm);
    return QString::fromUtf8(buffer);
}

QString formatValue(const QString& value, const QString& unit) {
    if (unit.isEmpty()) {
        return value;
    }
    return value + " " + unit;
}

}  // namespace

DeviceDashboard::DeviceDashboard(QWidget* parent) : QWidget(parent) {
    tree_ = new QTreeWidget(this);
    tree_->setHeaderLabels({"Device / Sensor", "Value", "Updated"});
    tree_->setColumnWidth(0, 280);
    tree_->setAlternatingRowColors(true);
    tree_->setRootIsDecorated(true);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Sensors by Device", this));
    layout->addWidget(tree_);
}

void DeviceDashboard::updateReading(const ramai::SensorReading& reading) {
    const QString device_id = QString::fromStdString(reading.device_id);
    const QString device_label = QString::fromStdString(reading.device_label);
    const QString sensor_key = QString::fromStdString(reading.sensor_key);

    device_labels_[reading.device_id] = device_label;
    device_readings_[reading.device_id][reading.sensor_key] = {
        QString::fromStdString(reading.sensor_label),
        QString::fromStdString(reading.value),
        QString::fromStdString(reading.unit),
        formatTimestamp(reading.received_at),
    };

    ensureDeviceNode(device_id, device_label);
    refreshDeviceNode(device_id);
}

void DeviceDashboard::clearReadings() {
    device_readings_.clear();
    device_labels_.clear();
    tree_->clear();
}

void DeviceDashboard::ensureDeviceNode(const QString& device_id, const QString& device_label) {
    for (int index = 0; index < tree_->topLevelItemCount(); ++index) {
        if (tree_->topLevelItem(index)->data(0, Qt::UserRole).toString() == device_id) {
            tree_->topLevelItem(index)->setText(0, device_label);
            return;
        }
    }

    auto* device_item = new QTreeWidgetItem(tree_);
    device_item->setText(0, device_label);
    device_item->setData(0, Qt::UserRole, device_id);
    device_item->setExpanded(true);
    tree_->addTopLevelItem(device_item);
}

void DeviceDashboard::refreshDeviceNode(const QString& device_id) {
    QTreeWidgetItem* device_item = nullptr;
    for (int index = 0; index < tree_->topLevelItemCount(); ++index) {
        if (tree_->topLevelItem(index)->data(0, Qt::UserRole).toString() == device_id) {
            device_item = tree_->topLevelItem(index);
            break;
        }
    }

    if (!device_item) {
        return;
    }

    const auto device_key = device_id.toStdString();
    const auto& sensors = device_readings_[device_key];

    while (device_item->childCount() > 0) {
        delete device_item->takeChild(0);
    }

    for (const auto& [sensor_key, state] : sensors) {
        auto* sensor_item = new QTreeWidgetItem(device_item);
        sensor_item->setText(0, state.label);
        sensor_item->setText(1, formatValue(state.value, state.unit));
        sensor_item->setText(2, state.updated_at);
    }
}
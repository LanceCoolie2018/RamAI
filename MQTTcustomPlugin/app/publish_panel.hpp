#pragma once

#include "ramai/message.hpp"

#include <QWidget>

class QComboBox;
class QLineEdit;
class QCheckBox;
class QSpinBox;
class QPushButton;

class PublishPanel : public QWidget {
    Q_OBJECT

public:
    explicit PublishPanel(QWidget* parent = nullptr);

    void rememberTopic(const QString& topic);

signals:
    void publishRequested(const ramai::MqttMessage& message);

private:
    QComboBox* topic_combo_ = nullptr;
    QLineEdit* payload_edit_ = nullptr;
    QSpinBox* qos_spin_ = nullptr;
    QCheckBox* retain_check_ = nullptr;
};
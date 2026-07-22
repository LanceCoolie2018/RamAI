#include "publish_panel.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

PublishPanel::PublishPanel(QWidget* parent) : QWidget(parent) {
    topic_combo_ = new QComboBox(this);
    topic_combo_->setEditable(true);
    payload_edit_ = new QLineEdit(this);
    qos_spin_ = new QSpinBox(this);
    retain_check_ = new QCheckBox("Retain", this);
    auto* send_button = new QPushButton("Send", this);

    qos_spin_->setRange(0, 2);

    auto* layout = new QHBoxLayout(this);
    layout->addWidget(new QLabel("Publish", this));
    layout->addWidget(topic_combo_, 2);
    layout->addWidget(payload_edit_, 3);
    layout->addWidget(new QLabel("QoS", this));
    layout->addWidget(qos_spin_);
    layout->addWidget(retain_check_);
    layout->addWidget(send_button);

    connect(send_button, &QPushButton::clicked, this, [this]() {
        const QString topic = topic_combo_->currentText().trimmed();
        if (topic.isEmpty()) {
            return;
        }

        ramai::MqttMessage message;
        message.topic = topic.toStdString();
        message.payload = payload_edit_->text().toStdString();
        message.qos = qos_spin_->value();
        message.retained = retain_check_->isChecked();
        emit publishRequested(message);
    });
}

void PublishPanel::rememberTopic(const QString& topic) {
    if (topic.isEmpty()) {
        return;
    }

    const int index = topic_combo_->findText(topic);
    if (index < 0) {
        topic_combo_->addItem(topic);
    }
    topic_combo_->setCurrentText(topic);
}
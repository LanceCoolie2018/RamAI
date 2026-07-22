#include "subscription_panel.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

SubscriptionPanel::SubscriptionPanel(QWidget* parent) : QWidget(parent) {
    subscription_list_ = new QListWidget(this);
    topic_edit_ = new QLineEdit(this);
    topic_edit_->setPlaceholderText("smartbed/sensor/#");

    auto* subscribe_button = new QPushButton("Subscribe", this);
    auto* unsubscribe_button = new QPushButton("Unsubscribe", this);
    auto* presets_button = new QPushButton("Smart Bed Presets", this);

    auto* input_row = new QHBoxLayout();
    input_row->addWidget(topic_edit_, 1);
    input_row->addWidget(subscribe_button);
    input_row->addWidget(unsubscribe_button);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Subscriptions", this));
    layout->addWidget(subscription_list_, 1);
    layout->addLayout(input_row);
    layout->addWidget(presets_button);

    connect(subscribe_button, &QPushButton::clicked, this, [this]() {
        const QString topic = topic_edit_->text().trimmed();
        if (!topic.isEmpty()) {
            emit subscribeRequested(topic);
        }
    });

    connect(unsubscribe_button, &QPushButton::clicked, this, [this]() {
        const auto* item = subscription_list_->currentItem();
        if (!item) {
            return;
        }
        emit unsubscribeRequested(item->text());
        delete subscription_list_->takeItem(subscription_list_->currentRow());
    });

    connect(presets_button, &QPushButton::clicked, this, &SubscriptionPanel::smartBedPresetsRequested);
}

void SubscriptionPanel::addSubscription(const QString& topic) {
    for (int row = 0; row < subscription_list_->count(); ++row) {
        if (subscription_list_->item(row)->text() == topic) {
            return;
        }
    }
    subscription_list_->addItem(topic);
}

QStringList SubscriptionPanel::subscriptions() const {
    QStringList topics;
    for (int row = 0; row < subscription_list_->count(); ++row) {
        topics << subscription_list_->item(row)->text();
    }
    return topics;
}
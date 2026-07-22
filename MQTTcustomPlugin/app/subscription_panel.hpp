#pragma once

#include <QWidget>

class QListWidget;
class QLineEdit;
class QPushButton;

class SubscriptionPanel : public QWidget {
    Q_OBJECT

public:
    explicit SubscriptionPanel(QWidget* parent = nullptr);

    void addSubscription(const QString& topic);
    QStringList subscriptions() const;

signals:
    void subscribeRequested(const QString& topic);
    void unsubscribeRequested(const QString& topic);
    void smartBedPresetsRequested();

private:
    QListWidget* subscription_list_ = nullptr;
    QLineEdit* topic_edit_ = nullptr;
};
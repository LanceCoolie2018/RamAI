#pragma once

#include "ramai/config.hpp"

#include <QWidget>

class QLineEdit;
class QPushButton;
class QLabel;

class ConnectionPanel : public QWidget {
    Q_OBJECT

public:
    explicit ConnectionPanel(QWidget* parent = nullptr);

    void setProfile(const ramai::BrokerProfile& profile);
    ramai::BrokerProfile currentProfile() const;
    void setConnected(bool connected, const QString& detail);

signals:
    void connectRequested(const ramai::BrokerProfile& profile);
    void disconnectRequested();

private:
    QLineEdit* host_edit_ = nullptr;
    QLineEdit* port_edit_ = nullptr;
    QLineEdit* client_id_edit_ = nullptr;
    QLineEdit* username_edit_ = nullptr;
    QLineEdit* password_edit_ = nullptr;
    QPushButton* connect_button_ = nullptr;
    QLabel* status_label_ = nullptr;
};
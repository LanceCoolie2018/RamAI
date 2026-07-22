#include "connection_panel.hpp"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

ConnectionPanel::ConnectionPanel(QWidget* parent) : QWidget(parent) {
    host_edit_ = new QLineEdit(this);
    port_edit_ = new QLineEdit(this);
    client_id_edit_ = new QLineEdit(this);
    username_edit_ = new QLineEdit(this);
    password_edit_ = new QLineEdit(this);
    connect_button_ = new QPushButton("Connect", this);
    status_label_ = new QLabel("Disconnected", this);

    port_edit_->setMaximumWidth(80);
    password_edit_->setEchoMode(QLineEdit::Password);

    auto* form = new QFormLayout();
    form->addRow("Host", host_edit_);
    form->addRow("Port", port_edit_);
    form->addRow("Client ID", client_id_edit_);
    form->addRow("Username", username_edit_);
    form->addRow("Password", password_edit_);

    auto* actions = new QHBoxLayout();
    actions->addWidget(connect_button_);
    actions->addWidget(status_label_);
    actions->addStretch();

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addLayout(actions);

    connect(connect_button_, &QPushButton::clicked, this, [this]() {
        if (connect_button_->text() == "Connect") {
            emit connectRequested(currentProfile());
        } else {
            emit disconnectRequested();
        }
    });
}

void ConnectionPanel::setProfile(const ramai::BrokerProfile& profile) {
    host_edit_->setText(QString::fromStdString(profile.host));
    port_edit_->setText(QString::number(profile.port));
    client_id_edit_->setText(QString::fromStdString(profile.client_id));
    username_edit_->setText(QString::fromStdString(profile.username));
    password_edit_->setText(QString::fromStdString(profile.password));
}

ramai::BrokerProfile ConnectionPanel::currentProfile() const {
    ramai::BrokerProfile profile;
    profile.name = "Current";
    profile.host = host_edit_->text().toStdString();
    profile.port = port_edit_->text().toInt();
    profile.client_id = client_id_edit_->text().toStdString();
    profile.username = username_edit_->text().toStdString();
    profile.password = password_edit_->text().toStdString();
    return profile;
}

void ConnectionPanel::setConnected(bool connected, const QString& detail) {
    connect_button_->setText(connected ? "Disconnect" : "Connect");
    status_label_->setText(connected ? QString("● %1").arg(detail) : detail);
    status_label_->setStyleSheet(connected ? "color: #2e7d32;" : "color: #c62828;");
}
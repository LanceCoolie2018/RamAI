#pragma once

#include "ramai/message.hpp"

#include <QWidget>

class QTableWidget;
class QLineEdit;
class QPushButton;

class MessageLog : public QWidget {
    Q_OBJECT

public:
    explicit MessageLog(QWidget* parent = nullptr);

    void appendMessage(const ramai::MqttMessage& message);
    void clearLog();
    bool exportToCsv(const QString& path) const;

signals:
    void exportRequested();

private:
    void applyFilter();

    QTableWidget* table_ = nullptr;
    QLineEdit* filter_edit_ = nullptr;
};
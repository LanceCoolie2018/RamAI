#include "message_log.hpp"

#include <QFile>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
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

}  // namespace

MessageLog::MessageLog(QWidget* parent) : QWidget(parent) {
    filter_edit_ = new QLineEdit(this);
    filter_edit_->setPlaceholderText("Filter by topic...");

    auto* export_button = new QPushButton("Export CSV", this);
    auto* clear_button = new QPushButton("Clear", this);

    table_ = new QTableWidget(0, 5, this);
    table_->setHorizontalHeaderLabels({"Time", "Topic", "Payload", "QoS", "Retained"});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto* top_row = new QHBoxLayout();
    top_row->addWidget(new QLabel("Message Log", this));
    top_row->addStretch();
    top_row->addWidget(filter_edit_, 1);
    top_row->addWidget(export_button);
    top_row->addWidget(clear_button);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(top_row);
    layout->addWidget(table_, 1);

    connect(filter_edit_, &QLineEdit::textChanged, this, &MessageLog::applyFilter);
    connect(export_button, &QPushButton::clicked, this, &MessageLog::exportRequested);
    connect(clear_button, &QPushButton::clicked, this, &MessageLog::clearLog);
}

void MessageLog::appendMessage(const ramai::MqttMessage& message) {
    const int row = table_->rowCount();
    table_->insertRow(row);

    const QString topic = QString::fromStdString(message.topic);
    const QString payload = QString::fromStdString(message.payload);

    table_->setItem(row, 0, new QTableWidgetItem(formatTimestamp(message.received_at)));
    table_->setItem(row, 1, new QTableWidgetItem(topic));
    table_->setItem(row, 2, new QTableWidgetItem(payload));
    table_->setItem(row, 3, new QTableWidgetItem(QString::number(message.qos)));
    table_->setItem(row, 4, new QTableWidgetItem(message.retained ? "yes" : "no"));

    applyFilter();
    table_->scrollToBottom();
}

void MessageLog::clearLog() {
    table_->setRowCount(0);
}

bool MessageLog::exportToCsv(const QString& path) const {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream << "time,topic,payload,qos,retained\n";

    for (int row = 0; row < table_->rowCount(); ++row) {
        if (table_->isRowHidden(row)) {
            continue;
        }

        stream << table_->item(row, 0)->text() << ','
               << '"' << table_->item(row, 1)->text().replace("\"", "\"\"") << '"' << ','
               << '"' << table_->item(row, 2)->text().replace("\"", "\"\"") << '"' << ','
               << table_->item(row, 3)->text() << ','
               << table_->item(row, 4)->text() << '\n';
    }

    return true;
}

void MessageLog::applyFilter() {
    const QString filter = filter_edit_->text().trimmed();
    for (int row = 0; row < table_->rowCount(); ++row) {
        const bool visible = filter.isEmpty() || table_->item(row, 1)->text().contains(filter, Qt::CaseInsensitive);
        table_->setRowHidden(row, !visible);
    }
}
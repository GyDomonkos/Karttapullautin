#pragma once

#include <QObject>
#include <QProcess>

class KarttaRunner : public QObject {
    Q_OBJECT

public:
    explicit KarttaRunner(QObject* parent = nullptr);
    void run(const QString& program, const QStringList& args);

signals:
    void log(QString);
    void finished(int code);

private:
    QProcess process;
};

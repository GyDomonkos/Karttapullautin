#pragma once

#include <QObject>
#include <QProcess>

class KarttaRunner : public QObject
{
    Q_OBJECT

public:
    explicit KarttaRunner(QObject* parent = nullptr);

    void run(const QString& executablePath,
             const QStringList& arguments);

    // Forcefully terminates the running process, if any.
    void cancel();

signals:
    void outputReceived(const QString&);
    void finished(int exitCode);

private:
    QProcess* process;
};
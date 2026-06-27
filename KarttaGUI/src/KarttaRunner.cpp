#include "KarttaRunner.h"
#include <QFileInfo>

KarttaRunner::KarttaRunner(QObject* parent)
    : QObject(parent)
{
    process = new QProcess(this);

    connect(process, &QProcess::readyReadStandardOutput, [this]() {
        emit outputReceived(process->readAllStandardOutput());
    });

    connect(process, &QProcess::readyReadStandardError, [this]() {
        emit outputReceived(process->readAllStandardError());
    });

    connect(process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this](int exitCode, QProcess::ExitStatus) {
        emit finished(exitCode);
    });
}

void KarttaRunner::run(const QString& executablePath,
                       const QStringList& arguments)
{
    QFileInfo info(executablePath);
    process->setWorkingDirectory(info.absolutePath());
    process->start(executablePath, arguments);
}

void KarttaRunner::cancel()
{
    // kill() sends SIGKILL / TerminateProcess — immediate, no clean-up.
    // Appropriate here since pullauta has no graceful shutdown protocol.
    if (process->state() != QProcess::NotRunning)
        process->kill();
}
#include "KarttaRunner.h"
#include <qfileinfo.h>

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


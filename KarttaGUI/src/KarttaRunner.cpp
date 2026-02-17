#include "KarttaRunner.h"

KarttaRunner::KarttaRunner(QObject* parent)
    : QObject(parent) {

    connect(&process, &QProcess::readyReadStandardOutput, [&]() {
        emit log(process.readAllStandardOutput());
    });

    connect(&process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [&](int code, QProcess::ExitStatus) {
                emit finished(code);
            });
}

void KarttaRunner::run(const QString& program,
                       const QStringList& args) {
    process.start(program, args);
}

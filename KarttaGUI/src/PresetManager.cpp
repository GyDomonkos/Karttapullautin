#include "PresetManager.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>


QString PresetManager::presetsDir()
{
    const QString path =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + "/presets";
    QDir().mkpath(path);   // create directory tree if it doesn't exist yet
    return path;
}

QStringList PresetManager::list()
{
    const QDir dir(presetsDir());
    const QStringList files = dir.entryList({ "*.json" }, QDir::Files, QDir::Name);

    QStringList names;
    names.reserve(files.size());
    for (const QString& file : files)
        names << QFileInfo(file).completeBaseName();   // strip .json

    return names;
}

bool PresetManager::save(const QString& name, const QJsonObject& data)
{
    QFile file(filePathFor(name));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return false;

    file.write(QJsonDocument(data).toJson(QJsonDocument::Indented));
    return true;
}

QJsonObject PresetManager::load(const QString& name)
{
    QFile file(filePathFor(name));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return {};

    return doc.object();
}

bool PresetManager::remove(const QString& name)
{
    return QFile::remove(filePathFor(name));
}

// --------------------------------------------------------------------------
// Private helpers
// --------------------------------------------------------------------------
QString PresetManager::sanitizeName(const QString& name)
{
    // Replace characters that are invalid in filenames on Windows or Linux
    // with a hyphen. Also collapse multiple hyphens.
    QString safe = name.trimmed();
    static const QRegularExpression invalid(R"([\\/:*?"<>|]+)");
    safe.replace(invalid, "-");
    return safe.isEmpty() ? "preset" : safe;
}

QString PresetManager::filePathFor(const QString& name)
{
    return presetsDir() + "/" + sanitizeName(name) + ".json";
}
#pragma once

#include <QString>
#include <QStringList>
#include <QJsonObject>

// --------------------------------------------------------------------------
// PresetManager — static utility for saving and loading named settings
// presets as JSON files.
//
// Storage location: QStandardPaths::AppDataLocation + "/presets/"
//   Windows: C:\Users\<user>\AppData\Roaming\KarttaGUI\presets\
//   Linux:   ~/.local/share/KarttaGUI/presets/
//
// File naming: each preset is stored as "<name>.json". Characters that are
// invalid in filenames on any supported platform are replaced with "-".
// --------------------------------------------------------------------------
class PresetManager
{
public:
    // Returns the presets directory path, creating it if needed.
    static QString     presetsDir();

    // Lists all saved preset names (without extension), sorted alphabetically.
    static QStringList list();

    // Saves a preset. Returns false if the file could not be written.
    static bool        save(const QString& name, const QJsonObject& data);

    // Loads a preset. Returns an empty object if the file does not exist or
    // cannot be parsed.
    static QJsonObject load(const QString& name);

    // Deletes a preset. Returns false if the file could not be removed.
    static bool        remove(const QString& name);

private:
    static QString sanitizeName(const QString& name);
    static QString filePathFor(const QString& name);
};
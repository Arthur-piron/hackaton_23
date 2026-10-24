#pragma once

#include <map>
#include <string>
#include <utility>

// Fichier de sauvegarde texte « clé=valeur », une entrée par ligne, portable Linux/Windows.
// Les lignes vides ou commençant par # sont ignorées.
class SaveFile {
public:
    // lastlight_save.txt à côté de l'exécutable si on peut y écrire,
    // sinon dans le dossier utilisateur (%APPDATA% ou $HOME).
    static std::string defaultPath();

    explicit SaveFile(std::string path) : path_(std::move(path)) {}

    // Faux si le fichier n'existe pas encore (première partie) : les valeurs restent vides.
    bool load();
    bool save() const;

    int getInt(const std::string& key, int fallback) const;
    void setInt(const std::string& key, int value);
    const std::string& path() const { return path_; }

private:
    std::string path_;
    std::map<std::string, std::string> values_;
};

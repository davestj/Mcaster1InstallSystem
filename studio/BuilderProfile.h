#pragma once
/*
 * BuilderProfile.h — Developer / company identity profile
 *
 * Profiles store identity info (company, email, certs, etc.) that can be
 * applied to a project to auto-populate AppInfoEditor fields.  They are
 * persisted as a JSON array in the user's app-config directory:
 *   macOS: ~/Library/Preferences/com.mcaster1.installstudio/profiles.json
 *   Win:   %APPDATA%\mcaster1\installstudio\profiles.json
 *   Linux: ~/.config/mcaster1/installstudio/profiles.json
 *
 * Variables exposed as {company}, {email}, {url}, {publisher}, {supportUrl}
 * are substituted into the manifest when applyToManifest() is called.
 */

#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include "Manifest.h"

// ─────────────────────────────────────────────────────────────────────────────
struct BuilderProfile
{
    QString id;            // UUID or unique slug
    QString displayName;   // Shown in the combo box (e.g. "ACME Corp (personal)")

    // Identity fields
    QString company;       // Company / organization name
    QString publisherName; // Publisher name used in installer paths
    QString email;         // Contact e-mail
    QString website;       // Product / company homepage URL
    QString supportUrl;    // Support page URL

    // Code signing
    QString macosSigningId;     // e.g. "Developer ID Application: ACME Corp (XXXXXXX)"
    QString windowsSigningCert; // Path to .pfx or SHA-1 thumbprint
    QString linuxGpgKeyId;      // GPG key fingerprint for package signing

    // Code signing mode
    bool        skipSigning  = false;  // skip ALL signing on every platform
    bool        devSignMode  = false;  // ad-hoc: macOS codesign --force --sign -; Win/Lin: skip

    // Projects associated with this profile (absolute paths to .mis files)
    QStringList projectPaths;

    // Optional custom tokens (key → value)
    QMap<QString, QString> customTokens;

    // ── Serialisation ────────────────────────────────────────────────────────
    QJsonObject toJson() const
    {
        QJsonObject o;
        o["id"]             = id;
        o["displayName"]    = displayName;
        o["company"]        = company;
        o["publisherName"]  = publisherName;
        o["email"]          = email;
        o["website"]        = website;
        o["supportUrl"]     = supportUrl;
        o["macosSigningId"] = macosSigningId;
        o["windowsSigningCert"] = windowsSigningCert;
        o["linuxGpgKeyId"]  = linuxGpgKeyId;

        o["skipSigning"]  = skipSigning;
        o["devSignMode"]  = devSignMode;
        QJsonArray pp;
        for (const QString &p : projectPaths) pp.append(p);
        o["projectPaths"] = pp;

        QJsonObject tok;
        for (auto it = customTokens.constBegin(); it != customTokens.constEnd(); ++it)
            tok[it.key()] = it.value();
        o["customTokens"] = tok;
        return o;
    }

    static BuilderProfile fromJson(const QJsonObject &o)
    {
        BuilderProfile p;
        p.id             = o["id"].toString();
        p.displayName    = o["displayName"].toString();
        p.company        = o["company"].toString();
        p.publisherName  = o["publisherName"].toString();
        p.email          = o["email"].toString();
        p.website        = o["website"].toString();
        p.supportUrl     = o["supportUrl"].toString();
        p.macosSigningId = o["macosSigningId"].toString();
        p.windowsSigningCert = o["windowsSigningCert"].toString();
        p.linuxGpgKeyId  = o["linuxGpgKeyId"].toString();
        p.skipSigning = o["skipSigning"].toBool();
        p.devSignMode = o["devSignMode"].toBool();
        for (const QJsonValue &v : o["projectPaths"].toArray())
            p.projectPaths << v.toString();
        const QJsonObject tok = o["customTokens"].toObject();
        for (auto it = tok.constBegin(); it != tok.constEnd(); ++it)
            p.customTokens[it.key()] = it.value().toString();
        return p;
    }

    // Apply this profile's values to manifest fields (only non-empty fields overwrite)
    void applyToManifest(Manifest &m) const
    {
        if (!publisherName.isEmpty()) m.app.publisher = publisherName;
        if (!website.isEmpty())       m.app.url        = website;
        if (!supportUrl.isEmpty())    m.app.supportUrl = supportUrl;
        if (!macosSigningId.isEmpty())
            m.signing.macosSigner = macosSigningId;
        if (!windowsSigningCert.isEmpty())
            m.signing.winPfxPath  = windowsSigningCert;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
class BuilderProfileManager
{
public:
    // Load profiles from disk; returns true if file was found and parsed.
    bool load()
    {
        m_profiles.clear();
        const QString path = profilesFilePath();
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) return false;
        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
        if (err.error != QJsonParseError::NoError) return false;
        for (const QJsonValue &v : doc.array())
            m_profiles.append(BuilderProfile::fromJson(v.toObject()));
        return true;
    }

    // Save profiles to disk; returns true on success.
    bool save() const
    {
        const QString path = profilesFilePath();
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
        QJsonArray arr;
        for (const BuilderProfile &p : m_profiles) arr.append(p.toJson());
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
        return true;
    }

    QList<BuilderProfile> &profiles()             { return m_profiles; }
    const QList<BuilderProfile> &profiles() const { return m_profiles; }

    // Find profile by id; returns nullptr if not found.
    const BuilderProfile *findById(const QString &id) const
    {
        for (const BuilderProfile &p : m_profiles)
            if (p.id == id) return &p;
        return nullptr;
    }

    void add(const BuilderProfile &p)    { m_profiles.append(p); }
    void remove(int index)               { if (index >= 0 && index < m_profiles.size()) m_profiles.removeAt(index); }
    void update(int index, const BuilderProfile &p)
    {
        if (index >= 0 && index < m_profiles.size())
            m_profiles[index] = p;
    }

    static QString profilesFilePath()
    {
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        return dir + "/profiles.json";
    }

private:
    QList<BuilderProfile> m_profiles;
};

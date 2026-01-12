#pragma once
/*
 * BuildBackend.h — Abstract base class for platform build backends.
 *
 * Each platform (macOS, Windows, Linux) derives from this and implements:
 *   - build()    : produce the installer package from a Manifest
 *   - validate() : pre-flight checks before building
 *
 * All methods are called from a background thread (QThread / QtConcurrent)
 * via BuildPanel.  Use the emit helpers to report progress to the Studio log.
 */

#include "../manifest/Manifest.h"
#include <QString>
#include <QStringList>
#include <functional>

// Progress callback: (percent 0-100, message)
using ProgressFn = std::function<void(int, const QString &)>;

class BuildBackend
{
public:
    virtual ~BuildBackend() = default;

    // Platform identifier string returned by platform()
    virtual QString platform() const = 0;

    // Human-readable platform name (e.g. "macOS (.dmg)")
    virtual QString displayName() const = 0;

    // Pre-flight validation — returns list of blocking issues
    virtual QStringList validate(const Manifest &m, const QString &projectDir) const = 0;

    // Perform the build.  Reports progress via progress callback.
    // Returns true on success; sets errOut on failure.
    virtual bool build(const Manifest   &m,
                       const QString    &projectDir,
                       const QString    &outputDir,
                       const ProgressFn &progress,
                       QString          *errOut = nullptr) = 0;

    // Output filename for the package this backend produces
    virtual QString outputFilename(const Manifest &m) const = 0;

protected:
    // Convenience helper used by backends to report progress
    void report(const ProgressFn &fn, int pct, const QString &msg) const
    {
        if (fn) fn(pct, msg);
    }
};

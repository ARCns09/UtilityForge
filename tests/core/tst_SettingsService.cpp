#include "core/settings/SettingsService.h"

#include <QTemporaryDir>
#include <QTest>

using utilityforge::core::AppPaths;
using utilityforge::core::SettingsService;

class SettingsServiceTest final : public QObject
{
    Q_OBJECT

private slots:
    void settingsRoundTripInIsolatedPath();
    void concurrencyIsBounded();
};

void SettingsServiceTest::settingsRoundTripInIsolatedPath()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const AppPaths paths =
        AppPaths::forTestRoot(temporaryDirectory.path(), QStringLiteral("utilityforge"));
    QVERIFY(!paths.ensureDirectories().has_value());

    {
        SettingsService settings(paths);
        QVERIFY(!settings.initialize().has_value());
        settings.setWindowGeometry(QByteArray("geometry"));
        settings.setWindowState(QByteArray("state"));
        settings.setLastOutputDirectory(QStringLiteral("/tmp/output"));
        QVERIFY(!settings.sync().has_value());
    }

    SettingsService reloadedSettings(paths);
    QVERIFY(!reloadedSettings.initialize().has_value());
    QCOMPARE(reloadedSettings.windowGeometry(), QByteArray("geometry"));
    QCOMPARE(reloadedSettings.windowState(), QByteArray("state"));
    QCOMPARE(reloadedSettings.lastOutputDirectory(), QStringLiteral("/tmp/output"));
}

void SettingsServiceTest::concurrencyIsBounded()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const AppPaths paths =
        AppPaths::forTestRoot(temporaryDirectory.path(), QStringLiteral("utilityforge"));
    QVERIFY(!paths.ensureDirectories().has_value());
    SettingsService settings(paths);
    QVERIFY(!settings.initialize().has_value());

    settings.setMaxConcurrentTasks(99);
    QCOMPARE(settings.maxConcurrentTasks(), 4);
    settings.setMaxConcurrentTasks(-5);
    QCOMPARE(settings.maxConcurrentTasks(), 1);
}

QTEST_GUILESS_MAIN(SettingsServiceTest)

#include "tst_SettingsService.moc"

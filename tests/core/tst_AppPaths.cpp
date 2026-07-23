#include "core/settings/AppPaths.h"

#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

using utilityforge::core::AppPaths;

class AppPathsTest final : public QObject
{
    Q_OBJECT

private slots:
    void testRootUsesIsolatedXdgLayout();
    void ensureDirectoriesCreatesWritableLocations();
};

void AppPathsTest::testRootUsesIsolatedXdgLayout()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const AppPaths paths =
        AppPaths::forTestRoot(temporaryDirectory.path(), QStringLiteral("utilityforge"));

    QVERIFY(paths.configDirectory().startsWith(temporaryDirectory.path()));
    QVERIFY(paths.dataDirectory().startsWith(temporaryDirectory.path()));
    QCOMPARE(QFileInfo(paths.settingsFilePath()).fileName(), QStringLiteral("settings.ini"));
}

void AppPathsTest::ensureDirectoriesCreatesWritableLocations()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const AppPaths paths =
        AppPaths::forTestRoot(temporaryDirectory.path(), QStringLiteral("utilityforge"));
    const auto error = paths.ensureDirectories();

    QVERIFY(!error.has_value());
    QVERIFY(QDir(paths.configDirectory()).exists());
    QVERIFY(QDir(paths.dataDirectory()).exists());
    QVERIFY(QDir(paths.cacheDirectory()).exists());
    QVERIFY(QDir(paths.stateDirectory()).exists());
    QVERIFY(QDir(paths.runtimeDirectory()).exists());
}

QTEST_GUILESS_MAIN(AppPathsTest)

#include "tst_AppPaths.moc"

#include "core/process/ExternalToolRegistry.h"

#include <QSignalSpy>
#include <QTest>

using utilityforge::core::ExternalTool;
using utilityforge::core::ExternalToolRegistry;
using utilityforge::core::ProcessRunner;
using utilityforge::core::ToolAvailability;
using utilityforge::core::ToolStatus;

class ExternalToolRegistryTest final : public QObject
{
    Q_OBJECT

private slots:
    void constructionDoesNotProbe();
    void explicitCheckUsesValidatedOverride();
    void missingOverrideFailsOnlyTheTool();
};

void ExternalToolRegistryTest::constructionDoesNotProbe()
{
    ProcessRunner runner;
    ExternalToolRegistry registry(&runner);
    QSignalSpy processSpy(&runner, &ProcessRunner::processStarted);

    QCOMPARE(registry.status(ExternalTool::Ffmpeg).availability, ToolAvailability::Unknown);
    QCOMPARE(registry.status(ExternalTool::Ffprobe).availability, ToolAvailability::Unknown);
    QCOMPARE(registry.status(ExternalTool::Bsdtar).availability, ToolAvailability::Unknown);
    QCOMPARE(processSpy.count(), 0);
}

void ExternalToolRegistryTest::explicitCheckUsesValidatedOverride()
{
    ProcessRunner runner;
    ExternalToolRegistry registry(&runner);
    QSignalSpy statusSpy(&registry, &ExternalToolRegistry::toolStatusChanged);
    QVERIFY(registry.setExecutableOverride(
        ExternalTool::Ffmpeg, QStringLiteral(UTILITYFORGE_FAKE_PROCESS_PATH)));

    registry.checkTool(ExternalTool::Ffmpeg);
    QTRY_VERIFY_WITH_TIMEOUT(
        registry.status(ExternalTool::Ffmpeg).availability == ToolAvailability::Available,
        3'000);

    const ToolStatus status = registry.status(ExternalTool::Ffmpeg);
    QCOMPARE(status.executablePath, QStringLiteral(UTILITYFORGE_FAKE_PROCESS_PATH));
    QVERIFY(status.versionSummary.contains(QStringLiteral("ffmpeg version")));
    QVERIFY(statusSpy.count() >= 2);
}

void ExternalToolRegistryTest::missingOverrideFailsOnlyTheTool()
{
    ProcessRunner runner;
    ExternalToolRegistry registry(&runner);
    QVERIFY(registry.setExecutableOverride(
        ExternalTool::Bsdtar, QStringLiteral("/not/a/real/utilityforge-tool")));

    registry.checkTool(ExternalTool::Bsdtar);

    QCOMPARE(registry.status(ExternalTool::Bsdtar).availability, ToolAvailability::Missing);
    QCOMPARE(registry.status(ExternalTool::Ffmpeg).availability, ToolAvailability::Unknown);
    QCOMPARE(runner.activeProcessCount(), 0);
}

QTEST_GUILESS_MAIN(ExternalToolRegistryTest)

#include "tst_ExternalToolRegistry.moc"

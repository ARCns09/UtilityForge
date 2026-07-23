#include "core/errors/ErrorFormatter.h"

#include <QTest>

using utilityforge::core::AppError;
using utilityforge::core::ErrorCategory;
using utilityforge::core::ErrorFormatter;
using utilityforge::core::ErrorSeverity;

class AppErrorTest final : public QObject
{
    Q_OBJECT

private slots:
    void invalidErrorUsesSafeFallback();
    void technicalSummaryIncludesStableCode();
    void processFailureIsStructured();
};

void AppErrorTest::invalidErrorUsesSafeFallback()
{
    const AppError error;

    QVERIFY(!error.isValid());
    QCOMPARE(ErrorFormatter::userSummary(error), QStringLiteral("An unknown error occurred."));
}

void AppErrorTest::technicalSummaryIncludesStableCode()
{
    const AppError error{
        .category = ErrorCategory::Validation,
        .severity = ErrorSeverity::Warning,
        .code = QStringLiteral("input.invalid"),
        .userMessage = QStringLiteral("The input is invalid."),
        .technicalDetails = QStringLiteral("fixture detail"),
        .taskId = {},
        .recoveryActions = {},
    };

    QVERIFY(error.isValid());
    QCOMPARE(ErrorFormatter::technicalSummary(error),
             QStringLiteral("input.invalid: fixture detail"));
}

void AppErrorTest::processFailureIsStructured()
{
    const AppError error =
        ErrorFormatter::processFailure(7, QStringLiteral("bounded stderr"));

    QCOMPARE(error.category, ErrorCategory::ProcessFailure);
    QCOMPARE(error.code, QStringLiteral("process.nonzero_exit"));
    QVERIFY(error.technicalDetails.contains(QStringLiteral("7")));
}

QTEST_GUILESS_MAIN(AppErrorTest)

#include "tst_AppError.moc"

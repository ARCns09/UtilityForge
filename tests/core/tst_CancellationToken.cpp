#include "core/tasks/CancellationToken.h"

#include <QTest>

#include <thread>

using utilityforge::core::CancellationSource;

class CancellationTokenTest final : public QObject
{
    Q_OBJECT

private slots:
    void requestIsIdempotent();
    void tokenObservesCrossThreadRequest();
};

void CancellationTokenTest::requestIsIdempotent()
{
    CancellationSource source;
    const auto token = source.token();

    QVERIFY(!token.isCancellationRequested());
    QVERIFY(source.requestCancellation());
    QVERIFY(!source.requestCancellation());
    QVERIFY(token.isCancellationRequested());
}

void CancellationTokenTest::tokenObservesCrossThreadRequest()
{
    CancellationSource source;
    const auto token = source.token();

    std::thread requester([source]() mutable {
        static_cast<void>(source.requestCancellation());
    });
    requester.join();

    QVERIFY(token.isCancellationRequested());
}

QTEST_GUILESS_MAIN(CancellationTokenTest)

#include "tst_CancellationToken.moc"

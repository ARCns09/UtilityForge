#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QTimer>

namespace {

void writeBytes(QFile &stream, const QByteArray &bytes)
{
    static_cast<void>(stream.write(bytes));
    static_cast<void>(stream.flush());
}

} // namespace

int main(int argumentCount, char *argumentValues[])
{
    QCoreApplication application(argumentCount, argumentValues);
    const QStringList arguments = application.arguments().sliced(1);
    QFile standardOutput;
    static_cast<void>(standardOutput.open(stdout, QIODevice::WriteOnly));
    QFile standardError;
    static_cast<void>(standardError.open(stderr, QIODevice::WriteOnly));

    if (arguments.contains(QStringLiteral("-version"))
        || arguments.contains(QStringLiteral("--version"))) {
        writeBytes(standardOutput,
                   QByteArrayLiteral("ffmpeg version fake\n"
                                     "ffprobe version fake\n"
                                     "bsdtar 3.0 libarchive fake\n"));
        return 0;
    }

    const qsizetype exitCodeIndex = arguments.indexOf(QStringLiteral("--exit-code"));
    if (exitCodeIndex >= 0 && exitCodeIndex + 1 < arguments.size()) {
        writeBytes(standardError, QByteArrayLiteral("requested failure"));
        return arguments.at(exitCodeIndex + 1).toInt();
    }

    const qsizetype delayIndex = arguments.indexOf(QStringLiteral("--delay-ms"));
    if (delayIndex >= 0 && delayIndex + 1 < arguments.size()) {
        const int delayMs = arguments.at(delayIndex + 1).toInt();
        QTimer::singleShot(delayMs, &application, [&application, &standardOutput]() {
            writeBytes(standardOutput, QByteArrayLiteral("completed"));
            application.quit();
        });
        return application.exec();
    }

    const qsizetype createOutputIndex =
        arguments.indexOf(QStringLiteral("--create-output"));
    if (createOutputIndex >= 0 && createOutputIndex + 1 < arguments.size()) {
        QFile outputFile(arguments.at(createOutputIndex + 1));
        if (!outputFile.open(QIODevice::WriteOnly)) {
            return 9;
        }
        writeBytes(outputFile, QByteArrayLiteral("fixture output"));
        return 0;
    }

    const qsizetype stdoutBytesIndex =
        arguments.indexOf(QStringLiteral("--stdout-bytes"));
    if (stdoutBytesIndex >= 0 && stdoutBytesIndex + 1 < arguments.size()) {
        const int byteCount = arguments.at(stdoutBytesIndex + 1).toInt();
        writeBytes(standardOutput, QByteArray(byteCount, 'x'));
        return 0;
    }

    for (const QString &argument : arguments) {
        writeBytes(standardOutput, argument.toUtf8());
        writeBytes(standardOutput, QByteArray(1, '\0'));
    }
    return 0;
}

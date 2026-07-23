#include "core/notifications/NotificationCenter.h"

#include "core/errors/ErrorFormatter.h"

#include <QTimer>

namespace utilityforge::core {
namespace {

constexpr qsizetype kMaximumRetainedNotifications = 20;

NotificationKind notificationKindForError(ErrorSeverity severity)
{
    if (severity == ErrorSeverity::Information) {
        return NotificationKind::Information;
    }
    if (severity == ErrorSeverity::Warning) {
        return NotificationKind::Warning;
    }
    return NotificationKind::Error;
}

} // namespace

NotificationCenter::NotificationCenter(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<Notification>();
    qRegisterMetaType<NotificationId>();
}

NotificationId NotificationCenter::post(NotificationKind kind,
                                        const QString &title,
                                        const QString &message,
                                        bool isPersistent,
                                        int displayDurationMs)
{
    if (title.trimmed().isEmpty() || message.trimmed().isEmpty()) {
        return {};
    }

    const Notification notification{
        .id = QUuid::createUuid(),
        .kind = kind,
        .title = title.trimmed(),
        .message = message.trimmed(),
        .isPersistent = isPersistent,
    };
    notifications_.insert(notification.id, notification);
    notificationOrder_.append(notification.id);
    trimToBound();
    emit notificationPosted(notification);

    if (!isPersistent && displayDurationMs > 0) {
        QTimer::singleShot(displayDurationMs, this, [this, id = notification.id]() {
            static_cast<void>(dismiss(id));
        });
    }
    return notification.id;
}

NotificationId NotificationCenter::postError(const AppError &error)
{
    if (!error.isValid()) {
        return {};
    }
    return post(notificationKindForError(error.severity),
                QStringLiteral("UtilityForge"),
                ErrorFormatter::userSummary(error),
                true);
}

bool NotificationCenter::dismiss(const NotificationId &notificationId)
{
    if (!notifications_.remove(notificationId)) {
        return false;
    }

    notificationOrder_.removeAll(notificationId);
    emit notificationDismissed(notificationId);
    return true;
}

QList<Notification> NotificationCenter::notifications() const
{
    QList<Notification> orderedNotifications;
    orderedNotifications.reserve(notificationOrder_.size());
    for (const NotificationId &notificationId : notificationOrder_) {
        const auto iterator = notifications_.constFind(notificationId);
        if (iterator != notifications_.cend()) {
            orderedNotifications.append(iterator.value());
        }
    }
    return orderedNotifications;
}

void NotificationCenter::trimToBound()
{
    while (notificationOrder_.size() > kMaximumRetainedNotifications) {
        const NotificationId oldestId = notificationOrder_.takeFirst();
        if (notifications_.remove(oldestId)) {
            emit notificationDismissed(oldestId);
        }
    }
}

} // namespace utilityforge::core

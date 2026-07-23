#pragma once

#include "core/errors/AppError.h"

#include <QHash>
#include <QList>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QUuid>

namespace utilityforge::core {

using NotificationId = QUuid;

enum class NotificationKind {
    Information,
    Success,
    Warning,
    Error
};

struct Notification {
    NotificationId id;
    NotificationKind kind{NotificationKind::Information};
    QString title;
    QString message;
    bool isPersistent{false};
};

class NotificationCenter final : public QObject
{
    Q_OBJECT

public:
    explicit NotificationCenter(QObject *parent = nullptr);

    [[nodiscard]] NotificationId post(NotificationKind kind,
                                      const QString &title,
                                      const QString &message,
                                      bool isPersistent = false,
                                      int displayDurationMs = 5'000);
    [[nodiscard]] NotificationId postError(const AppError &error);
    [[nodiscard]] bool dismiss(const NotificationId &notificationId);
    [[nodiscard]] QList<Notification> notifications() const;

signals:
    void notificationPosted(const utilityforge::core::Notification &notification);
    void notificationDismissed(const utilityforge::core::NotificationId &notificationId);

private:
    void trimToBound();

    QHash<NotificationId, Notification> notifications_;
    QList<NotificationId> notificationOrder_;
};

} // namespace utilityforge::core

Q_DECLARE_METATYPE(utilityforge::core::Notification)
Q_DECLARE_METATYPE(utilityforge::core::NotificationKind)

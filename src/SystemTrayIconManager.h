/*
 * Copyright 2017 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef QUENTIER_SYSTEM_TRAY_ICON_MANAGER_H
#define QUENTIER_SYSTEM_TRAY_ICON_MANAGER_H

#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>
#include <quentier/utility/Macros.h>
#include <QObject>
#include <QPointer>
#include <QSystemTrayIcon>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(AccountManager)

class SystemTrayIconManager : public QObject
{
    Q_OBJECT
public:
    explicit SystemTrayIconManager(AccountManager & accountManager,
                                   QObject * parent = Q_NULLPTR);

    /**
     * @brief isSystemTrayAvailable
     * @return either the output of QSystemTrayIcon::isSystemTrayAvailable
     * or the override of this check from the application settings
     */
    bool isSystemTrayAvailable() const;

    bool isShown() const;

    void show();
    void hide();

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

    // private signals
    void switchAccount(Account account);

private Q_SLOTS:
    void onAccountSwitched(Account account);

private:
    void createConnections();

    void persistTrayIconState();
    void restoreTrayIconState();

private:
    QPointer<AccountManager>    m_pAccountManager;
    QSystemTrayIcon *           m_pSystemTrayIcon;
};

} // namespace quentier

#endif // QUENTIER_SYSTEM_TRAY_ICON_MANAGER_H

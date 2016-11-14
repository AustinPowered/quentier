/*
 * Copyright 2016 Dmitry Ivanov
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

#include "ManageAccountsDialog.h"
#include "ui_ManageAccountsDialog.h"
#include "AddAccountDialog.h"
#include <quentier/logging/QuentierLogger.h>

using namespace quentier;

ManageAccountsDialog::ManageAccountsDialog(const QVector<Account> & availableAccounts,
                                           QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::ManageAccountsDialog),
    m_availableAccounts(availableAccounts),
    m_accountListModel()
{
    m_pUi->setupUi(this);

    m_pUi->listView->setModel(&m_accountListModel);
    updateAvailableAccountsInView();

    QObject::connect(m_pUi->addNewAccountButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(ManageAccountsDialog,onAddAccountButtonPressed));
    QObject::connect(m_pUi->revokeAuthenticationButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(ManageAccountsDialog,onRevokeAuthenticationButtonPressed));
}

ManageAccountsDialog::~ManageAccountsDialog()
{
    delete m_pUi;
}

void ManageAccountsDialog::onAvailableAccountsChanged(const QVector<Account> & availableAccounts)
{
    QNDEBUG(QStringLiteral("ManageAccountsDialog::onAvailableAccountsChanged"));

    m_availableAccounts = availableAccounts;
    updateAvailableAccountsInView();
}

void ManageAccountsDialog::onAddAccountButtonPressed()
{
    QNDEBUG(QStringLiteral("ManageAccountsDialog::onAddAccountButtonPressed"));

    QScopedPointer<AddAccountDialog> addAccountDialog(new AddAccountDialog(m_availableAccounts, this));
    addAccountDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(addAccountDialog.data(), QNSIGNAL(AddAccountDialog,evernoteAccountAdditionRequested,QString),
                     this, QNSIGNAL(ManageAccountsDialog,evernoteAccountAdditionRequested,QString));
    QObject::connect(addAccountDialog.data(), QNSIGNAL(AddAccountDialog,localAccountAdditionRequested,QString),
                     this, QNSIGNAL(ManageAccountsDialog,localAccountAdditionRequested,QString));
    Q_UNUSED(addAccountDialog->exec())
}

void ManageAccountsDialog::onRevokeAuthenticationButtonPressed()
{
    QNDEBUG(QStringLiteral("ManageAccountsDialog::onRevokeAuthenticationButtonPressed"));

    QModelIndex currentIndex = m_pUi->listView->currentIndex();
    if (Q_UNLIKELY(!currentIndex.isValid())) {
        QNTRACE(QStringLiteral("Current index is invalid"));
        return;
    }

    int currentRow = currentIndex.row();
    if (Q_UNLIKELY(currentRow < 0)) {
        QNTRACE(QStringLiteral("Current row is negative"));
        return;
    }

    if (Q_UNLIKELY(currentRow >= m_availableAccounts.size())) {
        QNTRACE(QStringLiteral("Current row is larger than the number of available accounts"));
        return;
    }

    const Account & availableAccount = m_availableAccounts.at(currentRow);
    if (availableAccount.type() == Account::Type::Local) {
        QNTRACE(QStringLiteral("The current account is local, nothing to do"));
        return;
    }

    emit revokeAuthentication(availableAccount.id());
}

void ManageAccountsDialog::updateAvailableAccountsInView()
{
    QNDEBUG(QStringLiteral("ManageAccountsDialog::updateAvailableAccountsInView"));

    int numAvailableAccounts = m_availableAccounts.size();
    QStringList accountsList;
    accountsList.reserve(numAvailableAccounts);

    for(int i = 0; i < numAvailableAccounts; ++i)
    {
        const Account & availableAccount = m_availableAccounts.at(i);
        QString displayedAccountName = availableAccount.name();
        if (availableAccount.type() == Account::Type::Local) {
            displayedAccountName += QStringLiteral(" (");
            // TRANSLATOR: the "local" word here means the local account i.e. the one not synchronized with Evernote
            displayedAccountName += tr("local");
            displayedAccountName += QStringLiteral(")");
        }

        accountsList << displayedAccountName;
    }

    m_accountListModel.setStringList(accountsList);
}
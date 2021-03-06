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

#ifndef QUENTIER_ACTIONS_INFO_H
#define QUENTIER_ACTIONS_INFO_H

#include <quentier/utility/Printable.h>
#include <QList>
#include <QHash>
#include <QKeySequence>

QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QAction)

namespace quentier {

struct ActionKeyWithContext
{
    ActionKeyWithContext() : m_key(-1), m_context() {}

    int     m_key;
    QString m_context;
};

struct ActionNonStandardKeyWithContext
{
    QString     m_nonStandardActionKey;
    QString     m_context;
};

class ActionsInfo
{
public:
    class ActionInfo: public Printable
    {
    public:
        ActionInfo();

        virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

        bool isEmpty() const;

        QString         m_name;
        QString         m_localizedName;
        QString         m_context;
        QString         m_category;
        int             m_shortcutKey;
        QString         m_nonStandardShortcutKey;
        QKeySequence    m_shortcut;
    };

public:
    ActionsInfo(const QList<QMenu*> & menus);

    const ActionInfo findActionInfo(const QString & actionName, const QString & context) const;

    class Iterator
    {
    public:
        Iterator(const int menuIndex, const int actionIndex, const ActionsInfo & actionsInfo);

        const ActionInfo actionInfo() const;

        bool operator==(const Iterator & other) const;
        bool operator!=(const Iterator & other) const;

        Iterator operator++();
        Iterator & operator++(int);

    private:
        void increment();

    private:
        const ActionsInfo &     m_actionsInfo;
        int                     m_menuIndex;
        int                     m_actionIndex;
    };

    friend class Iterator;

    Iterator begin() const;
    Iterator end() const;

private:
    ActionInfo fromAction(const QAction * pAction, const QString & category) const;

private:
    Q_DISABLE_COPY(ActionsInfo)

private:
    QList<QMenu*>   m_menus;
};

} // namespace quentier

Q_DECLARE_METATYPE(quentier::ActionKeyWithContext)
Q_DECLARE_METATYPE(quentier::ActionNonStandardKeyWithContext)

#endif // QUENTIER_ACTIONS_INFO_H

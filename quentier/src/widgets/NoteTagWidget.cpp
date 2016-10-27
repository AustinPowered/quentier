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

#include "NoteTagWidget.h"
#include "ui_NoteTagWidget.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

NoteTagWidget::NoteTagWidget(const QString & tagName, QWidget *parent) :
    QWidget(parent),
    m_pUi(new Ui::NoteTagWidget)
{
    m_pUi->setupUi(this);

    if (!QIcon::hasThemeIcon(QStringLiteral("edit-delete"))) {
        QIcon editDeleteIcon(QStringLiteral(":/fallback_icons/png/edit-delete-6.png"));
        m_pUi->deleteTagButton->setIcon(editDeleteIcon);
        QNTRACE(QStringLiteral("set fallback edit-delete icon"));
    }

    setTagName(tagName);
    QObject::connect(m_pUi->deleteTagButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(NoteTagWidget,onRemoveTagButtonPressed));
}

NoteTagWidget::~NoteTagWidget()
{
    delete m_pUi;
}

QString NoteTagWidget::tagName() const
{
    return m_pUi->tagNameLabel->text();
}

void NoteTagWidget::setTagName(const QString & name)
{
    m_pUi->tagNameLabel->setText(name);
}

void NoteTagWidget::onCanCreateTagRestrictionChanged(bool canCreateTag)
{
    m_pUi->deleteTagButton->setHidden(!canCreateTag);
}

void NoteTagWidget::onRemoveTagButtonPressed()
{
    emit removeTagFromNote(m_pUi->tagNameLabel->text());
}

} // namespace quentier

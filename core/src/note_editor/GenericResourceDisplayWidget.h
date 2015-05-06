#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__GENERIC_RESOURCE_DISPLAY_WIDGET_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__GENERIC_RESOURCE_DISPLAY_WIDGET_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QMimeDatabase)

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(IResource)

class GenericResourceDisplayWidget: public QWidget
{
    Q_OBJECT
public:
    GenericResourceDisplayWidget(const QIcon & icon, const QString & name,
                                 const QString & size, const IResource & resource,
                                 QWidget * parent = nullptr);

private:
    Q_DISABLE_COPY(GenericResourceDisplayWidget)

private:
    const IResource *     m_resource;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__GENERIC_RESOURCE_DISPLAY_WIDGET_H

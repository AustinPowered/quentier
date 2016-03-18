#ifndef __QUTE_NOTE__MODELS__TAG_MODEL_ITEM_H
#define __QUTE_NOTE__MODELS__TAG_MODEL_ITEM_H

#include <qute_note/utility/Printable.h>

namespace qute_note {

class TagModelItem: public Printable
{
public:
    explicit TagModelItem(const QString & localUid = QString(),
                          const QString & name = QString(),
                          const QString & parentLocalUid = QString(),
                          const bool isSynchronizable = false,
                          const bool isDirty = false,
                          TagModelItem * parent = Q_NULLPTR);
    ~TagModelItem();

    const QString & localUid() const { return m_localUid; }
    void setLocalUid(const QString & localUid) { m_localUid = localUid; }

    const QString & name() const { return m_name; }
    void setName(const QString & name) { m_name = name; }

    const QString & parentLocalUid() const { return m_parentLocalUid; }
    void setParentLocalUid(const QString & parentLocalUid) { m_parentLocalUid = parentLocalUid; }

    bool isSynchronizable() const { return m_isSynchronizable; }
    void setSynchronizable(const bool synchronizable) { m_isSynchronizable = synchronizable; }

    bool isDirty() const { return m_isDirty; }
    void setDirty(const bool dirty) { m_isDirty = dirty; }

    TagModelItem * parent() const { return m_parent; }
    void setParent(TagModelItem * parent) { m_parent = parent; }

    TagModelItem * childAtRow(const int row) const;
    int rowForChild(TagModelItem * child) const;

    bool hasChildren() const { return !m_children.isEmpty(); }
    QList<TagModelItem*> children() const { return m_children; }
    int numChildren() const { return m_children.size(); }

    void insertChild(const int row, TagModelItem * item);
    void addChild(TagModelItem * item);
    bool swapChildren(const int sourceRow, const int destRow);
    TagModelItem * takeChild(const int row);

    virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QString     m_localUid;
    QString     m_name;
    QString     m_parentLocalUid;
    bool        m_isSynchronizable;
    bool        m_isDirty;

    TagModelItem *          m_parent;
    QList<TagModelItem*>    m_children;
};

} // namespace qute_note

#endif // __QUTE_NOTE__MODELS__TAG_MODEL_ITEM_H
#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__TYPES__SYNCHRONIZED_DATA_ELEMENT_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__TYPES__SYNCHRONIZED_DATA_ELEMENT_H

#include "Guid.h"

namespace qute_note {

class SynchronizedDataElement: public Printable
{
public:
    SynchronizedDataElement();
    SynchronizedDataElement(const SynchronizedDataElement & other);
    SynchronizedDataElement & operator=(const SynchronizedDataElement & other);
    virtual ~SynchronizedDataElement();

    unsigned int getUpdateSequenceNumber() const;
    void setUpdateSequenceNumber(const unsigned int usn);

    /**
     * @brief isDirty - tells the caller whether this synchronized data element
     * in its latest revision has been synchronized with Evernote's online database
     * @return true is object was not synchronized or has local modifications,
     * false otherwise
     */
    bool isDirty() const;

    /**
     * @brief setDirty - mark the object as locally modified and needing synchronization
     * with Evernote online database
     */
    void setDirty();

    /**
     * @brief setSynchronized - mark the object as synchronized with Evernote online database
     */
    void setSynchronized();

    /**
     * @return true if the synchronizable object has never actually been synchronized
     * with remote Evernote service.
     */
    bool isLocal() const;

    const Guid & guid() const;
    void assignGuid(const std::string & guid);

    virtual bool isEmpty() const;

    /**
     * @brief operator < - compares two SynchronizedDataElement objects via their guid,
     * in order to make it possible to have sets of synchronized data elements
     * @param other - the other object for comparison
     * @return true if current object's guid is less than the other one's, false otherwise
     */
    bool operator<(const SynchronizedDataElement & other);

    virtual QTextStream & Print(QTextStream & strm) const;

private:
    unsigned int  m_updateSequenceNumber;
    bool          m_isDirty;
    bool          m_isLocal;
    Guid          m_guid;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__TYPES__SYNCHRONIZED_DATA_ELEMENT_H

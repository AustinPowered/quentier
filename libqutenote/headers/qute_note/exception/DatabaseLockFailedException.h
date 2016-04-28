#ifndef __LIB_QUTE_NOTE__EXCEPTION__DATABASE_LOCK_FAILED_EXCEPTION_H
#define __LIB_QUTE_NOTE__EXCEPTION__DATABASE_LOCK_FAILED_EXCEPTION_H

#include <qute_note/exception/IQuteNoteException.h>

namespace qute_note {

class QUTE_NOTE_EXPORT DatabaseLockFailedException: public IQuteNoteException
{
public:
    explicit DatabaseLockFailedException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__EXCEPTION__DATABASE_LOCK_FAILED_EXCEPTION_H
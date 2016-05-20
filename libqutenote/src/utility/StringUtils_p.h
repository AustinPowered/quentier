#ifndef LIB_QUTE_NOTE_UTILITY_STRING_UTILS_PRIVATE_H
#define LIB_QUTE_NOTE_UTILITY_STRING_UTILS_PRIVATE_H

#include <qute_note/utility/StringUtils.h>
#include <QHash>
#include <QStringList>

namespace qute_note {

class StringUtilsPrivate
{
public:
    StringUtilsPrivate();

    void removePunctuation(QString & str, const QVector<QChar> & charactersToPreserve) const;
    void removeDiacritics(QString & str) const;

private:
    void initialize();

private:
    QString     m_diacriticLetters;
    QStringList m_noDiacriticLetters;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_UTILITY_STRING_UTILS_PRIVATE_H

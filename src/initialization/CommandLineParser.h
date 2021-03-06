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

#ifndef QUENTIER_COMMAND_LINE_PARSER_H
#define QUENTIER_COMMAND_LINE_PARSER_H

#include <quentier/utility/Macros.h>
#include <quentier/types/ErrorString.h>
#include <QString>
#include <QHash>

namespace quentier {

class CommandLineParser
{
public:
    explicit CommandLineParser(int argc, char * argv[]);

    QString responseMessage() const;
    bool shouldQuit() const;

    bool hasError() const;
    ErrorString errorDescription() const;

    typedef QHash<QString, QVariant> CommandLineOptions;
    CommandLineOptions options() const;

private:
    Q_DISABLE_COPY(CommandLineParser)

private:
    QString             m_responseMessage;
    bool                m_shouldQuit;
    ErrorString         m_errorDescription;
    CommandLineOptions  m_parsedArgs;
};

} // namespace quentier

#endif // QUENTIER_COMMAND_LINE_PARSER_H

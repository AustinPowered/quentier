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

#include "MainWindow.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QStringList>
#include <QDebug>

int main(int argc, char * argv[])
{
    QApplication app(argc, argv);

    QStringList args = app.arguments();

    // NOTE: on Windows args.at(0) might contain the application name or it might not. Need to figure this out.
    if (args.at(0).contains(QString::fromUtf8("quentier_crash_handler"))) {
        args.pop_front();
    }

    if (args.size() < 4)  {
        qWarning() << QString::fromUtf8("Usage: quentier_crash_handler <compressed quentier symbols file location> "
                                        "<compressed libquentier symbols file location> <stackwalker tool location> "
                                        "<minidump file location>");
        return 1;
    }

    MainWindow window(args.at(0), args.at(1), args.at(2), args.at(3));
    QDesktopWidget * pDesktopWidget = QApplication::desktop();
    if (pDesktopWidget)
    {
        int screenWidth = pDesktopWidget->width();
        int screenHeight = pDesktopWidget->height();

        int width = window.frameGeometry().width();
        int height = window.frameGeometry().height();

        int x = (screenWidth - width) / 2;
        int y = (screenHeight - height) / 2;

        window.move(x, y);
    }

    window.show();

    return app.exec();
}
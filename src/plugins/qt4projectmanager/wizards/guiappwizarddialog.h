/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef GUIAPPWIZARDDIALOG_H
#define GUIAPPWIZARDDIALOG_H

#include "qtwizard.h"

namespace Qt4ProjectManager {
namespace Internal {

struct QtProjectParameters;
class FilesPage;

// Additional parameters required besides QtProjectParameters
struct GuiAppParameters
{
    GuiAppParameters();
    QString className;
    QString baseClassName;
    QString sourceFileName;
    QString headerFileName;
    QString formFileName;
    int widgetWidth;
    int widgetHeight;
    bool designerForm;
};

class GuiAppWizardDialog : public BaseQt4ProjectWizardDialog
{
    Q_OBJECT

public:
    explicit GuiAppWizardDialog(const QString &templateName,
                                const QIcon &icon,
                                const QList<QWizardPage*> &extensionPages,
                                bool showModulesPage = false,
                                QWidget *parent = 0);

    void setBaseClasses(const QStringList &baseClasses);
    void setSuffixes(const QString &header, const QString &source,  const QString &form);
    void setLowerCaseFiles(bool l);

    QtProjectParameters projectParameters() const;
    GuiAppParameters parameters() const;

private:
    FilesPage *m_filesPage;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // GUIAPPWIZARDDIALOG_H

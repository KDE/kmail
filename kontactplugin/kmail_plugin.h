/*
  This file is part of Kontact.

  Copyright (c) 2003-2013 Kontact Developer <kde-pim@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#ifndef KMAIL_PLUGIN_H
#define KMAIL_PLUGIN_H

#include <KontactInterface/Plugin>
#include <KontactInterface/UniqueAppHandler>

#include <QUrl>
class OrgKdeKmailKmailInterface;

namespace KontactInterface
{
class UniqueAppWatcher;
}

class KMailUniqueAppHandler : public KontactInterface::UniqueAppHandler
{
public:
    explicit KMailUniqueAppHandler(KontactInterface::Plugin *plugin)
        : KontactInterface::UniqueAppHandler(plugin) {}
    void loadCommandLineOptions(QCommandLineParser *parser) Q_DECL_OVERRIDE;
    int activate(const QStringList &args) Q_DECL_OVERRIDE;
};

class KMailPlugin : public KontactInterface::Plugin
{
    Q_OBJECT

public:
    KMailPlugin(KontactInterface::Core *core, const QVariantList &);
    ~KMailPlugin();

    bool isRunningStandalone() const Q_DECL_OVERRIDE;
    bool createDBUSInterface(const QString &serviceType) Q_DECL_OVERRIDE;
    KontactInterface::Summary *createSummaryWidget(QWidget *parent) Q_DECL_OVERRIDE;
    int weight() const Q_DECL_OVERRIDE
    {
        return 200;
    }

    QStringList invisibleToolbarActions() const Q_DECL_OVERRIDE;
    bool queryClose() const Q_DECL_OVERRIDE;

    void shortcutChanged() Q_DECL_OVERRIDE;

protected:
    KParts::ReadOnlyPart *createPart() Q_DECL_OVERRIDE;
    void openComposer(const QUrl &attach = QUrl());
    void openComposer(const QString &to);
    bool canDecodeMimeData(const QMimeData *) const Q_DECL_OVERRIDE;
    void processDropEvent(QDropEvent *) Q_DECL_OVERRIDE;

protected Q_SLOTS:
    void slotNewMail();
    void slotSyncFolders();

private:
    OrgKdeKmailKmailInterface *m_instance;
    KontactInterface::UniqueAppWatcher *mUniqueAppWatcher;
};

#endif

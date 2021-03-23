/*
  This file is part of Kontact.

  SPDX-FileCopyrightText: 2003-2013 Kontact Developer <kde-pim@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

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
    Q_OBJECT
public:
    explicit KMailUniqueAppHandler(KontactInterface::Plugin *plugin)
        : KontactInterface::UniqueAppHandler(plugin)
    {
    }

    void loadCommandLineOptions(QCommandLineParser *parser) override;
    Q_REQUIRED_RESULT int activate(const QStringList &args, const QString &workingDir) override;
};

class KMailPlugin : public KontactInterface::Plugin
{
    Q_OBJECT

public:
    KMailPlugin(KontactInterface::Core *core, const QVariantList &);
    ~KMailPlugin() override;

    Q_REQUIRED_RESULT bool isRunningStandalone() const override;
    KontactInterface::Summary *createSummaryWidget(QWidget *parent) override;
    Q_REQUIRED_RESULT int weight() const override;

    Q_REQUIRED_RESULT QStringList invisibleToolbarActions() const override;
    Q_REQUIRED_RESULT bool queryClose() const override;

    void shortcutChanged() override;

protected:
    KParts::Part *createPart() override;
    void openComposer(const QUrl &attach = QUrl());
    void openComposer(const QString &to);
    bool canDecodeMimeData(const QMimeData *) const override;
    void processDropEvent(QDropEvent *) override;

protected Q_SLOTS:
    void slotNewMail();
    void slotSyncFolders();

private:
    OrgKdeKmailKmailInterface *m_instance = nullptr;
    KontactInterface::UniqueAppWatcher *mUniqueAppWatcher = nullptr;
};


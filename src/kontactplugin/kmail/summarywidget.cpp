/*

  This file is part of Kontact.

  SPDX-FileCopyrightText: 2003 Tobias Koenig <tokoe@kde.org>
  SPDX-FileCopyrightText: 2013-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "summarywidget.h"
#include "kmailinterface.h"

#include <KontactInterface/Core>
#include <KontactInterface/Plugin>

#include <Akonadi/ChangeRecorder>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/CollectionStatistics>
#include <Akonadi/ETMViewStateSaver>
#include <Akonadi/EntityTreeModel>

#include <KMime/Message>

#include "kmailplugin_debug.h"
#include <KCheckableProxyModel>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KUrlLabel>

#include <KMessageBox>
#include <QEvent>
#include <QGridLayout>
#include <QIcon>
#include <QItemSelectionModel>
#include <QVBoxLayout>

#include <ctime>

SummaryWidget::SummaryWidget(KontactInterface::Plugin *plugin, QWidget *parent)
    : KontactInterface::Summary(parent)
    , mPlugin(plugin)
    , mChangeRecorder(new Akonadi::ChangeRecorder(this))
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(3);
    mainLayout->setContentsMargins(3, 3, 3, 3);

    QWidget *header = createHeader(this, QStringLiteral("view-pim-mail"), i18n("New Messages"));
    mainLayout->addWidget(header);

    mLayout = new QGridLayout();
    mainLayout->addItem(mLayout);
    mLayout->setSpacing(3);
    mLayout->setRowStretch(6, 1);

    // Create a new change recorder.
    mChangeRecorder->setMimeTypeMonitored(KMime::Message::mimeType());
    mChangeRecorder->fetchCollectionStatistics(true);
    mChangeRecorder->setAllMonitored(true);
    mChangeRecorder->collectionFetchScope().setIncludeStatistics(true);

    mModel = new Akonadi::EntityTreeModel(mChangeRecorder, this);
    mModel->setItemPopulationStrategy(Akonadi::EntityTreeModel::NoItemPopulation);
    connect(mModel, &Akonadi::EntityTreeModel::errorOccurred, this, [this](const QString &message) {
        KMessageBox::error(this, message);
    });

    mSelectionModel = new QItemSelectionModel(mModel);
    mModelProxy = new KCheckableProxyModel(this);
    mModelProxy->setSelectionModel(mSelectionModel);
    mModelProxy->setSourceModel(mModel);

    KSharedConfigPtr _config = KSharedConfig::openConfig(QStringLiteral("kcmkmailsummaryrc"));

    mModelState = new KViewStateMaintainer<Akonadi::ETMViewStateSaver>(_config->group(QStringLiteral("CheckState")), this);
    mModelState->setSelectionModel(mSelectionModel);

    connect(mChangeRecorder, qOverload<const Akonadi::Collection &>(&Akonadi::ChangeRecorder::collectionChanged), this, &SummaryWidget::slotCollectionChanged);
    connect(mChangeRecorder, &Akonadi::ChangeRecorder::collectionRemoved, this, &SummaryWidget::slotCollectionChanged);
    connect(mChangeRecorder, &Akonadi::ChangeRecorder::collectionStatisticsChanged, this, &SummaryWidget::slotCollectionChanged);
    QTimer::singleShot(0, this, &SummaryWidget::slotUpdateFolderList);
}

int SummaryWidget::summaryHeight() const
{
    return 1;
}

void SummaryWidget::slotCollectionChanged()
{
    QTimer::singleShot(0, this, &SummaryWidget::slotUpdateFolderList);
}

void SummaryWidget::updateSummary(bool force)
{
    Q_UNUSED(force)
    QTimer::singleShot(0, this, &SummaryWidget::slotUpdateFolderList);
}

void SummaryWidget::selectFolder(const QString &folder)
{
    if (mPlugin->isRunningStandalone()) {
        mPlugin->bringToForeground();
    } else {
        mPlugin->core()->selectPlugin(mPlugin);
    }

    org::kde::kmail::kmail kmail(QStringLiteral("org.kde.kmail"), QStringLiteral("/KMail"), QDBusConnection::sessionBus());
    kmail.selectFolder(folder);
}

void SummaryWidget::displayModel(const QModelIndex &parent, int &counter, const bool showFolderPaths, QStringList parentTreeNames)
{
    const int nbCol = mModelProxy->rowCount(parent);
    for (int i = 0; i < nbCol; ++i) {
        const QModelIndex child = mModelProxy->index(i, 0, parent);
        const auto col = mModelProxy->data(child, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
        const int showCollection = mModelProxy->data(child, Qt::CheckStateRole).toInt();

        if (col.isValid()) {
            const Akonadi::CollectionStatistics stats = col.statistics();
            if (((stats.unreadCount()) != Q_INT64_C(0)) && showCollection) {
                // Collection Name.
                KUrlLabel *urlLabel = nullptr;

                if (showFolderPaths) {
                    // Construct the full path string.
                    parentTreeNames.insert(parentTreeNames.size(), col.name());
                    urlLabel = new KUrlLabel(QString::number(col.id()), parentTreeNames.join(QLatin1Char('/')), this);
                    parentTreeNames.removeLast();
                } else {
                    urlLabel = new KUrlLabel(QString::number(col.id()), col.name(), this);
                }

                urlLabel->installEventFilter(this);
                urlLabel->setAlignment(Qt::AlignLeft);
                urlLabel->setWordWrap(true);
                mLayout->addWidget(urlLabel, counter, 1);
                mLabels.append(urlLabel);

                // tooltip
                urlLabel->setToolTip(
                    i18n("<qt><b>%1</b>"
                         "<br/>Total: %2<br/>"
                         "Unread: %3</qt>",
                         col.name(),
                         stats.count(),
                         stats.unreadCount()));

                connect(urlLabel, &KUrlLabel::leftClickedUrl, this, [this, urlLabel]() {
                    selectFolder(urlLabel->url());
                });

                // Read and unread count.
                auto label = new QLabel(i18nc("%1: number of unread messages "
                                              "%2: total number of messages",
                                              "%1 / %2",
                                              stats.unreadCount(),
                                              stats.count()),
                                        this);

                label->setAlignment(Qt::AlignLeft);
                mLayout->addWidget(label, counter, 2);
                mLabels.append(label);

                // Folder icon.
                auto icon = mModelProxy->data(child, Qt::DecorationRole).value<QIcon>();
                label = new QLabel(this);
                label->setPixmap(icon.pixmap(label->height() / 1.5));
                label->setMaximumWidth(label->minimumSizeHint().width());
                label->setAlignment(Qt::AlignVCenter);
                mLayout->addWidget(label, counter, 0);
                mLabels.append(label);

                ++counter;
            }
            parentTreeNames.insert(parentTreeNames.size(), col.name());
            displayModel(child, counter, showFolderPaths, parentTreeNames);
            // Remove the last parent collection name for the next iteration.
            parentTreeNames.removeLast();
        }
    }
}

void SummaryWidget::slotUpdateFolderList()
{
    qDeleteAll(mLabels);
    mLabels.clear();
    mModelState->restoreState();
    int counter = 0;
    qCDebug(KMAILPLUGIN_LOG) << QStringLiteral("Iterating over") << mModel->rowCount() << QStringLiteral("collections.");
    KConfig _config(QStringLiteral("kcmkmailsummaryrc"));
    KConfigGroup config(&_config, QStringLiteral("General"));
    const bool showFolderPaths = config.readEntry("showFolderPaths", false);
    displayModel(QModelIndex(), counter, showFolderPaths, QStringList());

    if (counter == 0) {
        auto label = new QLabel(i18nc("@label:textbox", "No unread messages in your monitored folders"), this);
        label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        mLayout->addWidget(label, 0, 0);
        mLabels.append(label);
    }

    QList<QLabel *>::const_iterator lit;
    QList<QLabel *>::const_iterator lend(mLabels.constEnd());
    for (lit = mLabels.constBegin(); lit != lend; ++lit) {
        (*lit)->show();
    }
}

bool SummaryWidget::eventFilter(QObject *obj, QEvent *e)
{
    if (obj->inherits("KUrlLabel")) {
        auto label = static_cast<KUrlLabel *>(obj);
        if (e->type() == QEvent::Enter) {
            Q_EMIT message(i18n("Open Folder: \"%1\"", label->text()));
        } else if (e->type() == QEvent::Leave) {
            Q_EMIT message(QString());
        }
    }

    return KontactInterface::Summary::eventFilter(obj, e);
}

#include "moc_summarywidget.cpp"

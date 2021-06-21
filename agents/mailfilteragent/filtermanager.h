/*
 * kmail: KDE mail client
 * SPDX-FileCopyrightText: 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#pragma once

#include <AkonadiCore/collection.h>
#include <AkonadiCore/item.h>

#include <MailCommon/SearchPattern>
#include <memory>
namespace MailCommon
{
class MailFilter;
class ItemContext;
}

class FilterManager : public QObject
{
    Q_OBJECT

public:
    /**
     * Describes the list of filters.
     */
    enum FilterSet {
        NoSet = 0x0,
        Inbound = 0x1,
        Outbound = 0x2,
        Explicit = 0x4,
        BeforeOutbound = 0x8,
        AllFolders = 0x16,
        All = Inbound | BeforeOutbound | Outbound | Explicit | AllFolders,
    };

    enum FilterRequires {
        Unknown = 0,
        HeaderMessage = 1,
        FullMessage = 2,
    };

    /**
     * Creates a new filter manager.
     *
     * @param parent The parent object.
     */
    explicit FilterManager(QObject *parent = nullptr);

    /**
     * Destroys the filter manager.
     */
    ~FilterManager() override;

    /**
     * Clears the list of filters and deletes them.
     */
    void clear();

    /**
     * Reloads the filter rules from config file.
     */
    void readConfig();

    /**
     * Checks for existing filters with the @p name and extend the
     * "name" to "name (i)" until no match is found for i=1..n
     */
    Q_REQUIRED_RESULT QString createUniqueName(const QString &name) const;

    /**
     * Process given message item by applying the filter rules one by
     * one. You can select which set of filters (incoming or outgoing)
     * should be used.
     *
     *  @param item The message item to process.
     *  @param set Select the filter set to use.
     *  @param account @c true if an account id is specified else @c false
     *  @param accountId The id of the KMAccount that the message was retrieved from
     *
     *  @return true if the filtering was successful, false in case of any error
     */
    Q_REQUIRED_RESULT bool
    process(const Akonadi::Item &item, bool needsFullPayload, FilterSet set = Inbound, bool account = false, const QString &accountId = QString());

    Q_REQUIRED_RESULT bool process(const QVector<MailCommon::MailFilter *> &mailFilters,
                                   const Akonadi::Item &item,
                                   bool needsFullPayload,
                                   FilterSet set = Inbound,
                                   bool account = false,
                                   const QString &accountId = QString());

    /**
     * For ad-hoc filters.
     *
     * Applies @p filter to message @p item.
     * Return codes are as with the above method.
     */
    Q_REQUIRED_RESULT bool process(const Akonadi::Item &item, bool needsFullPayload, const MailCommon::MailFilter *filter);

    void filter(const Akonadi::Item &item, FilterManager::FilterSet set, const QString &resourceId);
    void filter(const Akonadi::Item &item, const QString &filterId, const QString &resourceId);

    void applySpecificFilters(const Akonadi::Item::List &selectedMessages,
                              MailCommon::SearchRule::RequiredPart requiredPart,
                              const QStringList &listFilters,
                              FilterSet set = Explicit);

    /**
     * Applies the filters on the given @p messages.
     */
    void applyFilters(const Akonadi::Item::List &messages, FilterSet set = Explicit);

    /**
     * Returns whether the configured filters need the full mail content.
     */
    Q_REQUIRED_RESULT MailCommon::SearchRule::RequiredPart requiredPart(const QString &id) const;

    void mailCollectionRemoved(const Akonadi::Collection &collection);
    void agentRemoved(const QString &identifier);

    Q_REQUIRED_RESULT bool hasAllFoldersFilter() const;

    /**
     * Outputs all filter rules to console. Used for debugging.
     */
    void dump() const;

protected:
    Q_REQUIRED_RESULT bool processContextItem(MailCommon::ItemContext context);

Q_SIGNALS:
    /**
     * This signal is emitted whenever the filter list has been updated.
     */
    void filterListUpdated();

    /**
     * This signal is emitted to notify that @p item has not been moved.
     */
    void filteringFailed(const Akonadi::Item &item);

    void percent(int progress);
    void progressMessage(const QString &message);

private:
    //@cond PRIVATE
    class Private;
    std::unique_ptr<Private> const d;
    //@endcond
};


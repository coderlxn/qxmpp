/*
 * Copyright (C) 2008-2021 The QXmpp developers
 *
 * Author:
 *  Jeremy Lainé
 *
 * Source:
 *  https://github.com/qxmpp-project/qxmpp
 *
 * This file is a part of QXmpp library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 */

#ifndef QXMPPPUBSUBIQ_H
#define QXMPPPUBSUBIQ_H

#include "QXmppIq.h"

#include <QSharedDataPointer>

#if QXMPP_DEPRECATED_SINCE(1, 2)
#include "QXmppPubSubItem.h"
#endif

class QXmppPubSubIqPrivate;

/// \brief The QXmppPubSubIq class represents an IQ used for the
/// publish-subscribe mechanisms defined by \xep{0060}: Publish-Subscribe.
///
/// \ingroup Stanzas

class QXMPP_EXPORT QXmppPubSubIq : public QXmppIq
{
public:
    /// This enum is used to describe a publish-subscribe query type.
    enum QueryType {
        AffiliationsQuery,
        DefaultQuery,
        ItemsQuery,
        PublishQuery,
        RetractQuery,
        SubscribeQuery,
        SubscriptionQuery,
        SubscriptionsQuery,
        UnsubscribeQuery
    };

    QXmppPubSubIq();
    QXmppPubSubIq(const QXmppPubSubIq &iq);
    ~QXmppPubSubIq();

    QXmppPubSubIq &operator=(const QXmppPubSubIq &iq);

    QXmppPubSubIq::QueryType queryType() const;
    void setQueryType(QXmppPubSubIq::QueryType queryType);

    QString queryJid() const;
    void setQueryJid(const QString &jid);

    QString queryNode() const;
    void setQueryNode(const QString &node);

    QList<QXmppPubSubItem> items() const;
    void setItems(const QList<QXmppPubSubItem> &items);

    QString subscriptionId() const;
    void setSubscriptionId(const QString &id);

    /// \cond
    static bool isPubSubIq(const QDomElement &element);
    /// \endcond

protected:
    /// \cond
    void parseElementFromChild(const QDomElement &) override;
    void toXmlElementFromChild(QXmlStreamWriter *writer) const override;
    /// \endcond

private:
    QSharedDataPointer<QXmppPubSubIqPrivate> d;
};

#endif  // QXMPPPUBSUBIQ_H

/*
 * Copyright (C) 2008-2021 The QXmpp developers
 *
 * Authors:
 *  Manjeet Dahiya
 *  Jeremy Lainé
 *  Linus Jahn
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

#include "QXmppMessage.h"

#include "QXmppBitsOfBinaryDataList.h"
#include "QXmppConstants_p.h"
#include "QXmppMixInvitation.h"
#include "QXmppUtils.h"

#include <optional>

#include <QDateTime>
#include <QDomElement>
#include <QTextStream>
#include <QXmlStreamWriter>

static const QStringList CHAT_STATES = {
    QString(),
    QStringLiteral("active"),
    QStringLiteral("inactive"),
    QStringLiteral("gone"),
    QStringLiteral("composing"),
    QStringLiteral("paused")
};

static const QStringList MESSAGE_TYPES = {
    QStringLiteral("error"),
    QStringLiteral("normal"),
    QStringLiteral("chat"),
    QStringLiteral("groupchat"),
    QStringLiteral("headline")
};

static const QStringList MARKER_TYPES = {
    QString(),
    QStringLiteral("received"),
    QStringLiteral("displayed"),
    QStringLiteral("acknowledged")
};

static const QStringList ENCRYPTION_NAMESPACES = {
    QString(),
    QString(),
    ns_otr,
    ns_legacy_openpgp,
    ns_ox,
    ns_omemo
};

static const QStringList HINT_TYPES = {
    QStringLiteral("no-permanent-store"),
    QStringLiteral("no-store"),
    QStringLiteral("no-copy"),
    QStringLiteral("store")
};

static const QStringList ENCRYPTION_NAMES = {
    QString(),
    QString(),
    QStringLiteral("OTR"),
    QStringLiteral("Legacy OpenPGP"),
    QStringLiteral("OpenPGP for XMPP (OX)"),
    QStringLiteral("OMEMO")
};

static bool checkElement(const QDomElement &element, const QString &tagName, const QString &xmlns)
{
    return element.tagName() == tagName && element.namespaceURI() == xmlns;
}

enum StampType {
    LegacyDelayedDelivery,  // XEP-0091: Legacy Delayed Delivery
    DelayedDelivery         // XEP-0203: Delayed Delivery
};

class QXmppMessagePrivate : public QSharedData
{
public:
    QXmppMessagePrivate();

    QString body;
    QString subject;
    QString thread;
    QString parentThread;
    QXmppMessage::Type type;

    // XEP-0066: Out of Band Data
    QString outOfBandUrl;

    // XEP-0071: XHTML-IM
    QString xhtml;

    // XEP-0085: Chat State Notifications
    QXmppMessage::State state;

    // XEP-0091: Legacy Delayed Delivery | XEP-0203: Delayed Delivery
    QDateTime stamp;
    StampType stampType;

    // XEP-0184: Message Delivery Receipts
    QString receiptId;
    bool receiptRequested;

    // XEP-0224: Attention
    bool attentionRequested;

    // XEP-0231: Bits of Binary
    QXmppBitsOfBinaryDataList bitsOfBinaryData;

    // XEP-0249: Direct MUC Invitations
    QString mucInvitationJid;
    QString mucInvitationPassword;
    QString mucInvitationReason;

    // XEP-0280: Message Carbons
    bool privatemsg;

    // XEP-0308: Last Message Correction
    QString replaceId;

    // XEP-0333: Chat Markers
    bool markable;
    QXmppMessage::Marker marker;
    QString markedId;
    QString markedThread;

    // XEP-0334: Message Processing Hints
    quint8 hints;

    // XEP-0359: Unique and Stable Stanza IDs
    QString stanzaId;
    QString stanzaIdBy;
    QString originId;

    // XEP-0367: Message Attaching
    QString attachId;

    // XEP-0369: Mediated Information eXchange (MIX)
    QString mixUserJid;
    QString mixUserNick;

    // XEP-0380: Explicit Message Encryption
    QString encryptionMethod;
    QString encryptionName;

    // XEP-0382: Spoiler messages
    bool isSpoiler;
    QString spoilerHint;

    // XEP-0407: Mediated Information eXchange (MIX): Miscellaneous Capabilities
    std::optional<QXmppMixInvitation> mixInvitation;

    // XEP-0428: Fallback Indication
    bool isFallback;
};

QXmppMessagePrivate::QXmppMessagePrivate()
    : type(QXmppMessage::Normal),
      state(QXmppMessage::None),
      stampType(DelayedDelivery),
      receiptRequested(false),
      attentionRequested(false),
      privatemsg(false),
      markable(false),
      marker(QXmppMessage::NoMarker),
      hints(0),
      isSpoiler(false),
      isFallback(false)
{
}

/// Constructs a QXmppMessage.
///
/// \param from
/// \param to
/// \param body
/// \param thread

QXmppMessage::QXmppMessage(const QString &from, const QString &to, const QString &body, const QString &thread)
    : QXmppStanza(from, to), d(new QXmppMessagePrivate)
{
    d->type = Chat;
    d->body = body;
    d->thread = thread;
}

/// Constructs a copy of \a other.

QXmppMessage::QXmppMessage(const QXmppMessage &other) = default;

QXmppMessage::~QXmppMessage() = default;

/// Assigns \a other to this message.

QXmppMessage &QXmppMessage::operator=(const QXmppMessage &other) = default;

///
/// Indicates if the QXmppStanza is a stanza in the XMPP sense (i. e. a message,
/// iq or presence)
///
/// \since QXmpp 1.0
///
bool QXmppMessage::isXmppStanza() const
{
    return true;
}

/// Returns the message's body.

QString QXmppMessage::body() const
{
    return d->body;
}

/// Sets the message's body.
///
/// \param body

void QXmppMessage::setBody(const QString &body)
{
    d->body = body;
}

/// Returns the message's type.

QXmppMessage::Type QXmppMessage::type() const
{
    return d->type;
}

/// Sets the message's type.
///
/// \param type

void QXmppMessage::setType(QXmppMessage::Type type)
{
    d->type = type;
}

/// Returns the message's subject.

QString QXmppMessage::subject() const
{
    return d->subject;
}

/// Sets the message's subject.
///
/// \param subject

void QXmppMessage::setSubject(const QString &subject)
{
    d->subject = subject;
}

/// Returns the message's thread.

QString QXmppMessage::thread() const
{
    return d->thread;
}

/// Sets the message's thread.
///
/// \param thread

void QXmppMessage::setThread(const QString &thread)
{
    d->thread = thread;
}

///
/// Returns the optional parent thread of this message.
///
/// The possibility to create child threads was added in RFC6121.
///
/// \since QXmpp 1.3
///
QString QXmppMessage::parentThread() const
{
    return d->parentThread;
}

///
/// Sets the optional parent thread of this message.
///
/// The possibility to create child threads was added in RFC6121.
///
/// \since QXmpp 1.3
///
void QXmppMessage::setParentThread(const QString &parent)
{
    d->parentThread = parent;
}

///
/// Returns a possibly attached URL from \xep{0066}: Out of Band Data
///
/// \since QXmpp 1.0
///
QString QXmppMessage::outOfBandUrl() const
{
    return d->outOfBandUrl;
}

///
/// Sets the attached URL for \xep{0066}: Out of Band Data
///
/// \since QXmpp 1.0
///
void QXmppMessage::setOutOfBandUrl(const QString &url)
{
    d->outOfBandUrl = url;
}

///
/// Returns the message's XHTML body as defined by \xep{0071}: XHTML-IM.
///
/// \since QXmpp 0.6.2
///
QString QXmppMessage::xhtml() const
{
    return d->xhtml;
}

///
/// Sets the message's XHTML body as defined by \xep{0071}: XHTML-IM.
///
/// \since QXmpp 0.6.2
///
void QXmppMessage::setXhtml(const QString &xhtml)
{
    d->xhtml = xhtml;
}

///
/// Returns the the chat state notification according to \xep{0085}: Chat State
/// Notifications.
///
/// \since QXmpp 0.2
///
QXmppMessage::State QXmppMessage::state() const
{
    return d->state;
}

///
/// Sets the the chat state notification according to \xep{0085}: Chat State
/// Notifications.
///
/// \since QXmpp 0.2
///
void QXmppMessage::setState(QXmppMessage::State state)
{
    d->state = state;
}

///
/// Returns the optional timestamp of the message specified using \xep{0093}:
/// Legacy Delayed Delivery or using \xep{0203}: Delayed Delivery (preferred).
///
/// \since QXmpp 0.2
///
QDateTime QXmppMessage::stamp() const
{
    return d->stamp;
}

///
/// Sets the message's timestamp without modifying the type of the stamp
/// (\xep{0093}: Legacy Delayed Delivery or \xep{0203}: Delayed Delivery).
///
/// By default messages are constructed with the new delayed delivery XEP, but
/// parsed messages keep their type.
///
/// \since QXmpp 0.2
///
void QXmppMessage::setStamp(const QDateTime &stamp)
{
    d->stamp = stamp;
}

///
/// Returns true if a delivery receipt is requested, as defined by \xep{0184}:
/// Message Delivery Receipts.
///
/// \since QXmpp 0.4
///
bool QXmppMessage::isReceiptRequested() const
{
    return d->receiptRequested;
}

///
/// Sets whether a delivery receipt is requested, as defined by \xep{0184}:
/// Message Delivery Receipts.
///
/// \since QXmpp 0.4
///
void QXmppMessage::setReceiptRequested(bool requested)
{
    d->receiptRequested = requested;
    if (requested && id().isEmpty())
        generateAndSetNextId();
}

///
/// If this message is a delivery receipt, returns the ID of the original
/// message.
///
/// \since QXmpp 0.4
///
QString QXmppMessage::receiptId() const
{
    return d->receiptId;
}

///
/// Make this message a delivery receipt for the message with the given \a id.
///
/// \since QXmpp 0.4
///
void QXmppMessage::setReceiptId(const QString &id)
{
    d->receiptId = id;
}

///
/// Returns true if the user's attention is requested, as defined by \xep{0224}:
/// Attention.
///
/// \since QXmpp 0.4
///
bool QXmppMessage::isAttentionRequested() const
{
    return d->attentionRequested;
}

///
/// Sets whether the user's attention is requested, as defined by \xep{0224}:
/// Attention.
///
/// \param requested Whether to request attention (true) or not (false)
///
/// \since QXmpp 0.4
///
void QXmppMessage::setAttentionRequested(bool requested)
{
    d->attentionRequested = requested;
}

///
/// Returns a list of data packages attached using \xep{0231}: Bits of Binary.
///
/// This could be used to resolve \c cid: URIs found in the X-HTML body.
///
/// \since QXmpp 1.2
///
QXmppBitsOfBinaryDataList QXmppMessage::bitsOfBinaryData() const
{
    return d->bitsOfBinaryData;
}

///
/// Returns a list of data attached using \xep{0231}: Bits of Binary.
///
/// This could be used to resolve \c cid: URIs found in the X-HTML body.
///
/// \since QXmpp 1.2
///
QXmppBitsOfBinaryDataList &QXmppMessage::bitsOfBinaryData()
{
    return d->bitsOfBinaryData;
}

///
/// Sets a list of \xep{0231}: Bits of Binary attachments to be included.
///
/// \since QXmpp 1.2
///
void QXmppMessage::setBitsOfBinaryData(const QXmppBitsOfBinaryDataList &bitsOfBinaryData)
{
    d->bitsOfBinaryData = bitsOfBinaryData;
}

///
/// Returns whether the given text is a '/me command' as defined in \xep{0245}:
/// The /me Command.
///
/// \since QXmpp 1.3
///
bool QXmppMessage::isSlashMeCommand(const QString &body)
{
    return body.startsWith(QStringLiteral("/me "));
}

///
/// Returns whether the body of the message is a '/me command' as defined in
/// \xep{0245}: The /me Command.
///
/// \note If you want to check a custom string for the /me command, you can use
/// the static version of this method. This can be helpful when checking user
/// input before a message was sent.
///
/// \since QXmpp 1.3
///
bool QXmppMessage::isSlashMeCommand() const
{
    return isSlashMeCommand(d->body);
}

///
/// Returns the part of the body after the /me command.
///
/// This cuts off '/me ' (with the space) from the body, in case the body
/// starts with that. In case the body does not contain a /me command as
/// defined in \xep{0245}: The /me Command, a null string is returned.
///
/// This is useful when displaying the /me command correctly to the user.
///
/// \since QXmpp 1.3
///
QString QXmppMessage::slashMeCommandText(const QString &body)
{
    if (isSlashMeCommand(body))
        return body.mid(4);
    return {};
}

///
/// Returns the part of the body after the /me command.
///
/// This cuts off '/me ' (with the space) from the body, in case the body
/// starts with that. In case the body does not contain a /me command as
/// defined in \xep{0245}: The /me Command, a null string is returned.
///
/// This is useful when displaying the /me command correctly to the user.
///
/// \since QXmpp 1.3
///
QString QXmppMessage::slashMeCommandText() const
{
    return slashMeCommandText(d->body);
}

///
/// Returns the JID for a multi-user chat direct invitation as defined by
/// \xep{0249}: Direct MUC Invitations.
///
/// \since QXmpp 0.7.4
///
QString QXmppMessage::mucInvitationJid() const
{
    return d->mucInvitationJid;
}

///
/// Sets the JID for a multi-user chat direct invitation as defined by
/// \xep{0249}: Direct MUC Invitations.
///
/// \since QXmpp 0.7.4
///
void QXmppMessage::setMucInvitationJid(const QString &jid)
{
    d->mucInvitationJid = jid;
}

///
/// Returns the password for a multi-user chat direct invitation as defined by
/// \xep{0249}: Direct MUC Invitations.
///
/// \since QXmpp 0.7.4
///
QString QXmppMessage::mucInvitationPassword() const
{
    return d->mucInvitationPassword;
}

///
/// Sets the \a password for a multi-user chat direct invitation as defined by
/// \xep{0249}: Direct MUC Invitations.
///
/// \since QXmpp 0.7.4
///
void QXmppMessage::setMucInvitationPassword(const QString &password)
{
    d->mucInvitationPassword = password;
}

///
/// Returns the reason for a multi-user chat direct invitation as defined by
/// \xep{0249}: Direct MUC Invitations.
///
/// \since QXmpp 0.7.4
///
QString QXmppMessage::mucInvitationReason() const
{
    return d->mucInvitationReason;
}

///
/// Sets the \a reason for a multi-user chat direct invitation as defined by
/// \xep{0249}: Direct MUC Invitations.
///
/// \since QXmpp 0.7.4
///
void QXmppMessage::setMucInvitationReason(const QString &reason)
{
    d->mucInvitationReason = reason;
}

///
/// Returns if the message is marked with a &lt;private/&gt; tag, in which case
/// it will not be forwarded to other resources according to \xep{0280}: Message
/// Carbons.
///
/// \since QXmpp 1.0
///
bool QXmppMessage::isPrivate() const
{
    return d->privatemsg;
}

///
/// If true is passed, the message is marked with a &lt;private/&gt; tag, in
/// which case it will not be forwarded to other resources according to
/// \xep{0280}: Message Carbons.
///
/// \since QXmpp 1.0
///
void QXmppMessage::setPrivate(const bool priv)
{
    d->privatemsg = priv;
}

///
/// Returns the message id to replace with this message as used in \xep{0308}:
/// Last Message Correction. If the returned string is empty, this message is
/// not replacing another.
///
/// \since QXmpp 1.0
///
QString QXmppMessage::replaceId() const
{
    return d->replaceId;
}

///
/// Sets the message id to replace with this message as in \xep{0308}: Last
/// Message Correction.
///
/// \since QXmpp 1.0
///
void QXmppMessage::setReplaceId(const QString &replaceId)
{
    d->replaceId = replaceId;
}

///
/// Returns true if a message is markable, as defined by \xep{0333}: Chat
/// Markers.
///
/// \since QXmpp 0.8.1
///
bool QXmppMessage::isMarkable() const
{
    return d->markable;
}

///
/// Sets if the message is markable, as defined by \xep{0333}: Chat Markers.
///
/// \since QXmpp 0.8.1
///
void QXmppMessage::setMarkable(const bool markable)
{
    d->markable = markable;
}

///
/// Returns the message's marker id, as defined by \xep{0333}: Chat Markers.
///
/// \since QXmpp 0.8.1
///
QString QXmppMessage::markedId() const
{
    return d->markedId;
}

///
/// Sets the message's marker id, as defined by \xep{0333}: Chat Markers.
///
/// \since QXmpp 0.8.1
///
void QXmppMessage::setMarkerId(const QString &markerId)
{
    d->markedId = markerId;
}

///
/// Returns the message's marker thread, as defined by \xep{0333}: Chat Markers.
///
/// \since QXmpp 0.8.1
///
QString QXmppMessage::markedThread() const
{
    return d->markedThread;
}

///
/// Sets the message's marked thread, as defined by \xep{0333}: Chat Markers.
///
/// \since QXmpp 0.8.1
///
void QXmppMessage::setMarkedThread(const QString &markedThread)
{
    d->markedThread = markedThread;
}

///
/// Returns the message's marker, as defined by \xep{0333}: Chat Markers.
///
/// \since QXmpp 0.8.1
///
QXmppMessage::Marker QXmppMessage::marker() const
{
    return d->marker;
}

///
/// Sets the message's marker, as defined by \xep{0333}: Chat Markers
///
/// \since QXmpp 0.8.1
///
void QXmppMessage::setMarker(const Marker marker)
{
    d->marker = marker;
}

///
/// Returns true if the message contains the hint passed, as defined in
/// \xep{0334}: Message Processing Hints
///
/// \since QXmpp 1.1
///
bool QXmppMessage::hasHint(const Hint hint) const
{
    return d->hints & hint;
}

///
/// Adds a hint to the message, as defined in \xep{0334}: Message Processing
/// Hints
///
/// \since QXmpp 1.1
///
void QXmppMessage::addHint(const Hint hint)
{
    d->hints |= hint;
}

///
/// Removes a hint from the message, as defined in \xep{0334}: Message
/// Processing Hints
///
/// \since QXmpp 1.1
///
void QXmppMessage::removeHint(const Hint hint)
{
    d->hints &= ~hint;
}

///
/// Removes all hints from the message, as defined in \xep{0334}: Message
/// Processing Hints
///
/// \since QXmpp 1.1
///
void QXmppMessage::removeAllHints()
{
    d->hints = 0;
}

///
/// Returns the stanza ID of the message according to \xep{0359}: Unique and
/// Stable Stanza IDs.
///
/// \since QXmpp 1.3
///
QString QXmppMessage::stanzaId() const
{
    return d->stanzaId;
}

///
/// Sets the stanza ID of the message according to \xep{0359}: Unique and
/// Stable Stanza IDs.
///
/// \since QXmpp 1.3
///
void QXmppMessage::setStanzaId(const QString &id)
{
    d->stanzaId = id;
}

///
/// Returns the creator of the stanza ID according to \xep{0359}: Unique and
/// Stable Stanza IDs.
///
/// \since QXmpp 1.3
///
QString QXmppMessage::stanzaIdBy() const
{
    return d->stanzaIdBy;
}

///
/// Sets the creator of the stanza ID according to \xep{0359}: Unique and
/// Stable Stanza IDs.
///
/// \since QXmpp 1.3
///
void QXmppMessage::setStanzaIdBy(const QString &by)
{
    d->stanzaIdBy = by;
}

///
/// Returns the origin ID of the message according to \xep{0359}: Unique and
/// Stable Stanza IDs.
///
/// \since QXmpp 1.3
///
QString QXmppMessage::originId() const
{
    return d->originId;
}

///
/// Sets the origin ID of the message according to \xep{0359}: Unique and
/// Stable Stanza IDs.
///
/// \since QXmpp 1.3
///
void QXmppMessage::setOriginId(const QString &id)
{
    d->originId = id;
}

///
/// Returns the message id this message is linked/attached to. See \xep{0367}:
/// Message Attaching for details.
///
/// \since QXmpp 1.1
///
QString QXmppMessage::attachId() const
{
    return d->attachId;
}

///
/// Sets the id of the attached message as in \xep{0367}: Message Attaching.
/// This can be used for a "reply to" or "reaction" function.
///
/// The used message id depends on the message context, see the Business rules
/// section of the XEP for details about when to use which id.
///
/// \since QXmpp 1.1
///
void QXmppMessage::setAttachId(const QString &attachId)
{
    d->attachId = attachId;
}

///
/// Returns the actual JID of a MIX channel participant.
///
/// \since QXmpp 1.1
///
QString QXmppMessage::mixUserJid() const
{
    return d->mixUserJid;
}

///
/// Sets the actual JID of a MIX channel participant.
///
/// \since QXmpp 1.1
///
void QXmppMessage::setMixUserJid(const QString &mixUserJid)
{
    d->mixUserJid = mixUserJid;
}

///
/// Returns the MIX participant's nickname.
///
/// \since QXmpp 1.1
///
QString QXmppMessage::mixUserNick() const
{
    return d->mixUserNick;
}

///
/// Sets the MIX participant's nickname.
///
/// \since QXmpp 1.1
///
void QXmppMessage::setMixUserNick(const QString &mixUserNick)
{
    d->mixUserNick = mixUserNick;
}

///
/// Returns the encryption method this message is advertised to be encrypted
/// with.
///
/// \note QXmppMessage::NoEncryption does not necesserily mean that the message
/// is not encrypted; it may also be that the author of the message does not
/// support \xep{0380}: Explicit Message Encryption.
///
/// \note If this returns QXmppMessage::UnknownEncryption, you can still get
/// the namespace of the encryption with \c encryptionMethodNs() and possibly
/// also a name with \c encryptionName().
///
/// \since QXmpp 1.1
///
QXmppMessage::EncryptionMethod QXmppMessage::encryptionMethod() const
{
    if (d->encryptionMethod.isEmpty())
        return QXmppMessage::NoEncryption;

    int index = ENCRYPTION_NAMESPACES.indexOf(d->encryptionMethod);
    if (index < 0)
        return QXmppMessage::UnknownEncryption;
    return static_cast<QXmppMessage::EncryptionMethod>(index);
}

///
/// Advertises that this message is encrypted with the given encryption method.
/// See \xep{0380}: Explicit Message Encryption for details.
///
/// \since QXmpp 1.1
///
void QXmppMessage::setEncryptionMethod(QXmppMessage::EncryptionMethod method)
{
    d->encryptionMethod = ENCRYPTION_NAMESPACES.at(int(method));
}

///
/// Returns the namespace of the advertised encryption method via. \xep{0380}:
/// Explicit Message Encryption.
///
/// \since QXmpp 1.1
///
QString QXmppMessage::encryptionMethodNs() const
{
    return d->encryptionMethod;
}

///
/// Sets the namespace of the encryption method this message advertises to be
/// encrypted with. See \xep{0380}: Explicit Message Encryption for details.
///
/// \since QXmpp 1.1
///
void QXmppMessage::setEncryptionMethodNs(const QString &encryptionMethod)
{
    d->encryptionMethod = encryptionMethod;
}

///
/// Returns the associated name of the encryption method this message
/// advertises to be encrypted with. See \xep{0380}: Explicit Message Encryption
/// for details.
///
/// \since QXmpp 1.1
///
QString QXmppMessage::encryptionName() const
{
    if (!d->encryptionName.isEmpty())
        return d->encryptionName;
    return ENCRYPTION_NAMES.at(int(encryptionMethod()));
}

///
/// Sets the name of the encryption method for \xep{0380}: Explicit Message
/// Encryption.
///
/// \note This should only be used, if the encryption method is custom and is
/// not one of the methods listed in the XEP.
///
/// \since QXmpp 1.1
///
void QXmppMessage::setEncryptionName(const QString &encryptionName)
{
    d->encryptionName = encryptionName;
}

///
/// Returns true, if this is a spoiler message according to \xep{0382}: Spoiler
/// messages. The spoiler hint however can still be empty.
///
/// A spoiler message's content should not be visible to the user by default.
///
/// \since QXmpp 1.1
///
bool QXmppMessage::isSpoiler() const
{
    return d->isSpoiler;
}

///
/// Sets whether this is a spoiler message as specified in \xep{0382}: Spoiler
/// messages.
///
/// The content of spoiler messages will not be displayed by default to the
/// user. However, clients not supporting spoiler messages will still display
/// the content as usual.
///
/// \since QXmpp 1.1
///
void QXmppMessage::setIsSpoiler(bool isSpoiler)
{
    d->isSpoiler = isSpoiler;
}

///
/// Returns the spoiler hint as specified in \xep{0382}: Spoiler messages.
///
/// The hint may be empty, even if isSpoiler is true.
///
/// \since QXmpp 1.1
///
QString QXmppMessage::spoilerHint() const
{
    return d->spoilerHint;
}

///
/// Sets a spoiler hint for \xep{0382}: Spoiler messages. If the spoiler hint
/// is not empty, isSpoiler will be set to true.
///
/// A spoiler hint is optional for spoiler messages.
///
/// Keep in mind that the spoiler hint is not displayed at all by clients not
/// supporting spoiler messages.
///
/// \since QXmpp 1.1
///
void QXmppMessage::setSpoilerHint(const QString &spoilerHint)
{
    d->spoilerHint = spoilerHint;
    if (!spoilerHint.isEmpty())
        d->isSpoiler = true;
}

///
/// Returns an included \xep{0369}: Mediated Information eXchange (MIX)
/// invitation as defined by \xep{0407}: Mediated Information eXchange (MIX):
/// Miscellaneous Capabilities.
///
/// \since QXmpp 1.4
///
std::optional<QXmppMixInvitation> QXmppMessage::mixInvitation() const
{
    return d->mixInvitation;
}

///
/// Sets a \xep{0369}: Mediated Information eXchange (MIX) invitation as defined
/// by \xep{0407}: Mediated Information eXchange (MIX): Miscellaneous
/// Capabilities.
///
/// \since QXmpp 1.4
///
void QXmppMessage::setMixInvitation(const std::optional<QXmppMixInvitation> &mixInvitation)
{
    d->mixInvitation = mixInvitation;
}

///
/// Sets whether this message is only a fallback according to \xep{0428}:
/// Fallback Indication.
///
/// This is useful for clients not supporting end-to-end encryption to indicate
/// that the message body does not contain the intended text of the author.
///
/// \since QXmpp 1.3
///
bool QXmppMessage::isFallback() const
{
    return d->isFallback;
}

///
/// Sets whether this message is only a fallback according to \xep{0428}:
/// Fallback Indication.
///
/// This is useful for clients not supporting end-to-end encryption to indicate
/// that the message body does not contain the intended text of the author.
///
/// \since QXmpp 1.3
///
void QXmppMessage::setIsFallback(bool isFallback)
{
    d->isFallback = isFallback;
}

/// \cond
void QXmppMessage::parse(const QDomElement &element)
{
    QXmppStanza::parse(element);

    // message type
    int messageType = MESSAGE_TYPES.indexOf(element.attribute(QStringLiteral("type")));
    if (messageType != -1)
        d->type = static_cast<Type>(messageType);
    else
        d->type = QXmppMessage::Normal;

    QXmppElementList extensions;
    QDomElement childElement = element.firstChildElement();
    while (!childElement.isNull()) {
        if (childElement.tagName() == QStringLiteral("body")) {
            d->body = childElement.text();
        } else if (childElement.tagName() == QStringLiteral("subject")) {
            d->subject = childElement.text();
        } else if (childElement.tagName() == QStringLiteral("thread")) {
            d->thread = childElement.text();
            d->parentThread = childElement.attribute(QStringLiteral("parent"));

            // parse message extensions
            // XEP-0033: Extended Stanza Addressing and errors are parsed by QXmppStanza
        } else if (!checkElement(childElement, QStringLiteral("addresses"), ns_extended_addressing) &&
                   childElement.tagName() != QStringLiteral("error")) {
            parseExtension(childElement, extensions);
        }
        childElement = childElement.nextSiblingElement();
    }
    setExtensions(extensions);
}

void QXmppMessage::toXml(QXmlStreamWriter *xmlWriter) const
{
    xmlWriter->writeStartElement(QStringLiteral("message"));
    helperToXmlAddAttribute(xmlWriter, QStringLiteral("xml:lang"), lang());
    helperToXmlAddAttribute(xmlWriter, QStringLiteral("id"), id());
    helperToXmlAddAttribute(xmlWriter, QStringLiteral("to"), to());
    helperToXmlAddAttribute(xmlWriter, QStringLiteral("from"), from());
    helperToXmlAddAttribute(xmlWriter, QStringLiteral("type"), MESSAGE_TYPES.at(d->type));
    if (!d->subject.isEmpty())
        helperToXmlAddTextElement(xmlWriter, QStringLiteral("subject"), d->subject);
    if (!d->body.isEmpty())
        helperToXmlAddTextElement(xmlWriter, QStringLiteral("body"), d->body);
    if (!d->thread.isEmpty()) {
        xmlWriter->writeStartElement(QStringLiteral("thread"));
        helperToXmlAddAttribute(xmlWriter, QStringLiteral("parent"), d->parentThread);
        xmlWriter->writeCharacters(d->thread);
        xmlWriter->writeEndElement();
    }
    error().toXml(xmlWriter);

    // XEP-0066: Out of Band Data
    if (!d->outOfBandUrl.isEmpty()) {
        xmlWriter->writeStartElement(QStringLiteral("x"));
        xmlWriter->writeDefaultNamespace(ns_oob);
        xmlWriter->writeTextElement(QStringLiteral("url"), d->outOfBandUrl);
        xmlWriter->writeEndElement();
    }

    // XEP-0071: XHTML-IM
    if (!d->xhtml.isEmpty()) {
        xmlWriter->writeStartElement(QStringLiteral("html"));
        xmlWriter->writeDefaultNamespace(ns_xhtml_im);
        xmlWriter->writeStartElement(QStringLiteral("body"));
        xmlWriter->writeDefaultNamespace(ns_xhtml);
        xmlWriter->writeCharacters(QStringLiteral(""));
        xmlWriter->device()->write(d->xhtml.toUtf8());
        xmlWriter->writeEndElement();
        xmlWriter->writeEndElement();
    }

    // XEP-0085: Chat State Notifications
    if (d->state > None && d->state <= Paused) {
        xmlWriter->writeStartElement(CHAT_STATES.at(d->state));
        xmlWriter->writeDefaultNamespace(ns_chat_states);
        xmlWriter->writeEndElement();
    }

    // XEP-0091: Legacy Delayed Delivery | XEP-0203: Delayed Delivery
    if (d->stamp.isValid()) {
        QDateTime utcStamp = d->stamp.toUTC();
        if (d->stampType == DelayedDelivery) {
            // XEP-0203: Delayed Delivery
            xmlWriter->writeStartElement(QStringLiteral("delay"));
            xmlWriter->writeDefaultNamespace(ns_delayed_delivery);
            helperToXmlAddAttribute(xmlWriter, QStringLiteral("stamp"), QXmppUtils::datetimeToString(utcStamp));
            xmlWriter->writeEndElement();
        } else {
            // XEP-0091: Legacy Delayed Delivery
            xmlWriter->writeStartElement(QStringLiteral("x"));
            xmlWriter->writeDefaultNamespace(ns_legacy_delayed_delivery);
            helperToXmlAddAttribute(xmlWriter, QStringLiteral("stamp"), utcStamp.toString(QStringLiteral("yyyyMMddThh:mm:ss")));
            xmlWriter->writeEndElement();
        }
    }

    // XEP-0184: Message Delivery Receipts
    if (!d->receiptId.isEmpty()) {
        xmlWriter->writeStartElement(QStringLiteral("received"));
        xmlWriter->writeDefaultNamespace(ns_message_receipts);
        xmlWriter->writeAttribute(QStringLiteral("id"), d->receiptId);
        xmlWriter->writeEndElement();
    }
    if (d->receiptRequested) {
        xmlWriter->writeStartElement(QStringLiteral("request"));
        xmlWriter->writeDefaultNamespace(ns_message_receipts);
        xmlWriter->writeEndElement();
    }

    // XEP-0224: Attention
    if (d->attentionRequested) {
        xmlWriter->writeStartElement(QStringLiteral("attention"));
        xmlWriter->writeDefaultNamespace(ns_attention);
        xmlWriter->writeEndElement();
    }

    // XEP-0249: Direct MUC Invitations
    if (!d->mucInvitationJid.isEmpty()) {
        xmlWriter->writeStartElement(QStringLiteral("x"));
        xmlWriter->writeDefaultNamespace(ns_conference);
        xmlWriter->writeAttribute(QStringLiteral("jid"), d->mucInvitationJid);
        if (!d->mucInvitationPassword.isEmpty())
            xmlWriter->writeAttribute(QStringLiteral("password"), d->mucInvitationPassword);
        if (!d->mucInvitationReason.isEmpty())
            xmlWriter->writeAttribute(QStringLiteral("reason"), d->mucInvitationReason);
        xmlWriter->writeEndElement();
    }

    // XEP-0231: Bits of Binary
    for (const auto &data : std::as_const(d->bitsOfBinaryData))
        data.toXmlElementFromChild(xmlWriter);

    // XEP-0280: Message Carbons
    if (d->privatemsg) {
        xmlWriter->writeStartElement(QStringLiteral("private"));
        xmlWriter->writeDefaultNamespace(ns_carbons);
        xmlWriter->writeEndElement();
    }

    // XEP-0308: Last Message Correction
    if (!d->replaceId.isEmpty()) {
        xmlWriter->writeStartElement(QStringLiteral("replace"));
        xmlWriter->writeDefaultNamespace(ns_message_correct);
        xmlWriter->writeAttribute(QStringLiteral("id"), d->replaceId);
        xmlWriter->writeEndElement();
    }

    // XEP-0333: Chat Markers
    if (d->markable) {
        xmlWriter->writeStartElement(QStringLiteral("markable"));
        xmlWriter->writeDefaultNamespace(ns_chat_markers);
        xmlWriter->writeEndElement();
    }
    if (d->marker != NoMarker) {
        xmlWriter->writeStartElement(MARKER_TYPES.at(d->marker));
        xmlWriter->writeDefaultNamespace(ns_chat_markers);
        xmlWriter->writeAttribute(QStringLiteral("id"), d->markedId);
        if (!d->markedThread.isNull() && !d->markedThread.isEmpty()) {
            xmlWriter->writeAttribute(QStringLiteral("thread"), d->markedThread);
        }
        xmlWriter->writeEndElement();
    }

    // XEP-0334: Message Processing Hints
    for (quint8 i = 0; i < HINT_TYPES.size(); i++) {
        if (hasHint(Hint(1 << i))) {
            xmlWriter->writeStartElement(HINT_TYPES.at(i));
            xmlWriter->writeDefaultNamespace(ns_message_processing_hints);
            xmlWriter->writeEndElement();
        }
    }

    // XEP-0359: Unique and Stable Stanza IDs
    if (!d->stanzaId.isNull()) {
        xmlWriter->writeStartElement(QStringLiteral("stanza-id"));
        xmlWriter->writeDefaultNamespace(ns_sid);
        xmlWriter->writeAttribute(QStringLiteral("id"), d->stanzaId);
        if (!d->stanzaIdBy.isNull())
            xmlWriter->writeAttribute(QStringLiteral("by"), d->stanzaIdBy);
        xmlWriter->writeEndElement();
    }

    if (!d->originId.isNull()) {
        xmlWriter->writeStartElement(QStringLiteral("origin-id"));
        xmlWriter->writeDefaultNamespace(ns_sid);
        xmlWriter->writeAttribute(QStringLiteral("id"), d->originId);
        xmlWriter->writeEndElement();
    }

    // XEP-0367: Message Attaching
    if (!d->attachId.isEmpty()) {
        xmlWriter->writeStartElement(QStringLiteral("attach-to"));
        xmlWriter->writeDefaultNamespace(ns_message_attaching);
        xmlWriter->writeAttribute(QStringLiteral("id"), d->attachId);
        xmlWriter->writeEndElement();
    }

    // XEP-0369: Mediated Information eXchange (MIX)
    if (!d->mixUserJid.isEmpty() || !d->mixUserNick.isEmpty()) {
        xmlWriter->writeStartElement(QStringLiteral("mix"));
        xmlWriter->writeDefaultNamespace(ns_mix);
        helperToXmlAddTextElement(xmlWriter, QStringLiteral("jid"), d->mixUserJid);
        helperToXmlAddTextElement(xmlWriter, QStringLiteral("nick"), d->mixUserNick);
        xmlWriter->writeEndElement();
    }

    // XEP-0380: Explicit Message Encryption
    if (!d->encryptionMethod.isEmpty()) {
        xmlWriter->writeStartElement(QStringLiteral("encryption"));
        xmlWriter->writeDefaultNamespace(ns_eme);
        xmlWriter->writeAttribute(QStringLiteral("namespace"), d->encryptionMethod);
        helperToXmlAddAttribute(xmlWriter, QStringLiteral("name"), d->encryptionName);
        xmlWriter->writeEndElement();
    }

    // XEP-0382: Spoiler messages
    if (d->isSpoiler) {
        xmlWriter->writeStartElement(QStringLiteral("spoiler"));
        xmlWriter->writeDefaultNamespace(ns_spoiler);
        xmlWriter->writeCharacters(d->spoilerHint);
        xmlWriter->writeEndElement();
    }

    // XEP-0407: Mediated Information eXchange (MIX): Miscellaneous Capabilities
    if (d->mixInvitation) {
        d->mixInvitation->toXml(xmlWriter);
    }

    // XEP-0428: Fallback Indication
    if (d->isFallback) {
        xmlWriter->writeStartElement(QStringLiteral("fallback"));
        xmlWriter->writeDefaultNamespace(ns_fallback_indication);
        xmlWriter->writeEndElement();
    }

    // other extensions
    QXmppStanza::extensionsToXml(xmlWriter);

    xmlWriter->writeEndElement();
}
/// \endcond

///
/// Parses message extensions
///
/// \param element child element of the message to be parsed
/// \param unknownExtensions extensions not known are added to this list as
/// QXmppElement
///
void QXmppMessage::parseExtension(const QDomElement &element, QXmppElementList &unknownExtensions)
{
    if (element.tagName() == QStringLiteral("x")) {
        parseXElement(element, unknownExtensions);
    } else if (checkElement(element, QStringLiteral("html"), ns_xhtml_im)) {
        // XEP-0071: XHTML-IM
        QDomElement bodyElement = element.firstChildElement(QStringLiteral("body"));
        if (!bodyElement.isNull() && bodyElement.namespaceURI() == ns_xhtml) {
            QTextStream stream(&d->xhtml, QIODevice::WriteOnly);
            bodyElement.save(stream, 0);

            d->xhtml = d->xhtml.mid(d->xhtml.indexOf('>') + 1);
            d->xhtml.replace(
                QStringLiteral(" xmlns=\"http://www.w3.org/1999/xhtml\""),
                QString());
            d->xhtml.replace(QStringLiteral("</body>"), QString());
            d->xhtml = d->xhtml.trimmed();
        }
    } else if (element.namespaceURI() == ns_chat_states) {
        // XEP-0085: Chat State Notifications
        int i = CHAT_STATES.indexOf(element.tagName());
        if (i > 0)
            d->state = static_cast<QXmppMessage::State>(i);
    } else if (checkElement(element, QStringLiteral("received"), ns_message_receipts)) {
        // XEP-0184: Message Delivery Receipts
        d->receiptId = element.attribute(QStringLiteral("id"));

        // compatibility with old-style XEP
        if (d->receiptId.isEmpty())
            d->receiptId = id();
    } else if (checkElement(element, QStringLiteral("request"), ns_message_receipts)) {
        d->receiptRequested = true;
    } else if (checkElement(element, QStringLiteral("delay"), ns_delayed_delivery)) {
        // XEP-0203: Delayed Delivery
        d->stamp = QXmppUtils::datetimeFromString(
            element.attribute(QStringLiteral("stamp")));
        d->stampType = DelayedDelivery;
    } else if (checkElement(element, QStringLiteral("attention"), ns_attention)) {
        // XEP-0224: Attention
        d->attentionRequested = true;
    } else if (QXmppBitsOfBinaryData::isBitsOfBinaryData(element)) {
        // XEP-0231: Bits of Binary
        QXmppBitsOfBinaryData data;
        data.parseElementFromChild(element);
        d->bitsOfBinaryData << data;
    } else if (checkElement(element, QStringLiteral("private"), ns_carbons)) {
        // XEP-0280: Message Carbons
        d->privatemsg = true;
    } else if (checkElement(element, QStringLiteral("replace"), ns_message_correct)) {
        // XEP-0308: Last Message Correction
        d->replaceId = element.attribute(QStringLiteral("id"));
    } else if (element.namespaceURI() == ns_chat_markers) {
        // XEP-0333: Chat Markers
        if (element.tagName() == QStringLiteral("markable")) {
            d->markable = true;
        } else {
            int marker = MARKER_TYPES.indexOf(element.tagName());
            if (marker != -1) {
                d->marker = static_cast<QXmppMessage::Marker>(marker);
                d->markedId = element.attribute(QStringLiteral("id"));
                d->markedThread = element.attribute(QStringLiteral("thread"));
            }
        }
    } else if (element.namespaceURI() == ns_message_processing_hints &&
               HINT_TYPES.contains(element.tagName())) {
        // XEP-0334: Message Processing Hints
        addHint(Hint(1 << HINT_TYPES.indexOf(element.tagName())));
    } else if (checkElement(element, QStringLiteral("stanza-id"), ns_sid)) {
        // XEP-0359: Unique and Stable Stanza IDs
        d->stanzaId = element.attribute(QStringLiteral("id"));
        d->stanzaIdBy = element.attribute(QStringLiteral("by"));
    } else if (checkElement(element, QStringLiteral("origin-id"), ns_sid)) {
        d->originId = element.attribute(QStringLiteral("id"));
    } else if (checkElement(element, QStringLiteral("attach-to"), ns_message_attaching)) {
        // XEP-0367: Message Attaching
        d->attachId = element.attribute(QStringLiteral("id"));
    } else if (checkElement(element, QStringLiteral("mix"), ns_mix)) {
        // XEP-0369: Mediated Information eXchange (MIX)
        d->mixUserJid = element.firstChildElement(QStringLiteral("jid")).text();
        d->mixUserNick = element.firstChildElement(QStringLiteral("nick")).text();
    } else if (checkElement(element, QStringLiteral("encryption"), ns_eme)) {
        // XEP-0380: Explicit Message Encryption
        d->encryptionMethod = element.attribute(QStringLiteral("namespace"));
        d->encryptionName = element.attribute(QStringLiteral("name"));
    } else if (checkElement(element, QStringLiteral("spoiler"), ns_spoiler)) {
        // XEP-0382: Spoiler messages
        d->isSpoiler = true;
        d->spoilerHint = element.text();
    } else if (checkElement(element, QStringLiteral("invitation"), ns_mix_misc)) {
        // XEP-0407: Mediated Information eXchange (MIX): Miscellaneous Capabilities
        QXmppMixInvitation mixInvitation;
        mixInvitation.parse(element);
        d->mixInvitation = mixInvitation;
    } else if (checkElement(element, QStringLiteral("fallback"), ns_fallback_indication)) {
        // XEP-0428: Fallback Indication
        d->isFallback = true;
    } else {
        // other extensions
        unknownExtensions << QXmppElement(element);
    }
}

///
/// Parses &lt;x/&gt; child elements of the message
///
/// \param element child element of the message to be parsed
/// \param unknownExtensions extensions not known are added to this list as
/// QXmppElement
///
void QXmppMessage::parseXElement(const QDomElement &element, QXmppElementList &unknownExtensions)
{
    if (element.namespaceURI() == ns_legacy_delayed_delivery) {
        // if XEP-0203 exists, XEP-0091 has no need to parse because XEP-0091
        // is no more standard protocol)
        if (d->stamp.isNull()) {
            // XEP-0091: Legacy Delayed Delivery
            d->stamp = QDateTime::fromString(
                element.attribute(QStringLiteral("stamp")),
                QStringLiteral("yyyyMMddThh:mm:ss"));
            d->stamp.setTimeSpec(Qt::UTC);
            d->stampType = LegacyDelayedDelivery;
        }
    } else if (element.namespaceURI() == ns_conference) {
        // XEP-0249: Direct MUC Invitations
        d->mucInvitationJid = element.attribute(QStringLiteral("jid"));
        d->mucInvitationPassword = element.attribute(QStringLiteral("password"));
        d->mucInvitationReason = element.attribute(QStringLiteral("reason"));
    } else if (element.namespaceURI() == ns_oob) {
        // XEP-0066: Out of Band Data
        d->outOfBandUrl = element.firstChildElement(QStringLiteral("url")).text();
    } else {
        unknownExtensions << QXmppElement(element);
    }
}

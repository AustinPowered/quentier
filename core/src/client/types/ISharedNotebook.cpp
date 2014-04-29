#include "ISharedNotebook.h"
#include "QEverCloudOptionalQString.hpp"
#include "../Utility.h"

namespace qute_note {

ISharedNotebook::ISharedNotebook()
{}

ISharedNotebook::~ISharedNotebook()
{}

bool ISharedNotebook::operator==(const ISharedNotebook & other) const
{
    const qevercloud::SharedNotebook & sharedNotebook = GetEnSharedNotebook();
    const qevercloud::SharedNotebook & otherSharedNotebook = other.GetEnSharedNotebook();

    return (sharedNotebook == otherSharedNotebook);
}

bool ISharedNotebook::operator!=(const ISharedNotebook & other) const
{
    return !(*this == other);
}

bool ISharedNotebook::hasId() const
{
    return GetEnSharedNotebook().id.isSet();
}

qint64 ISharedNotebook::id() const
{
    return GetEnSharedNotebook().id;
}

void ISharedNotebook::setId(const qint64 id)
{
    GetEnSharedNotebook().id = id;
}

bool ISharedNotebook::hasUserId() const
{
    return GetEnSharedNotebook().userId.isSet();
}

qint32 ISharedNotebook::userId() const
{
    return GetEnSharedNotebook().userId;
}

void ISharedNotebook::setUserId(const qint32 userId)
{
    GetEnSharedNotebook().userId = userId;
}

const bool ISharedNotebook::hasNotebookGuid() const
{
    return GetEnSharedNotebook().notebookGuid.isSet();
}

const QString & ISharedNotebook::notebookGuid() const
{
    return GetEnSharedNotebook().notebookGuid;
}

void ISharedNotebook::setNotebookGuid(const QString & notebookGuid)
{
    GetEnSharedNotebook().notebookGuid = notebookGuid;
}

const bool ISharedNotebook::hasEmail() const
{
    return GetEnSharedNotebook().email.isSet();
}

const QString & ISharedNotebook::email() const
{
    return GetEnSharedNotebook().email;
}

void ISharedNotebook::setEmail(const QString & email)
{
    GetEnSharedNotebook().email = email;
}

bool ISharedNotebook::hasCreationTimestamp() const
{
    return GetEnSharedNotebook().serviceCreated.isSet();
}

qint64 ISharedNotebook::creationTimestamp() const
{
    return GetEnSharedNotebook().serviceCreated;
}

void ISharedNotebook::setCreationTimestamp(const qint64 timestamp)
{
    // TODO: verify whether it really matters
    if (timestamp >= 0) {
        GetEnSharedNotebook().serviceCreated = timestamp;
    }
    else {
        GetEnSharedNotebook().serviceCreated.clear();
    }
}

bool ISharedNotebook::hasModificationTimestamp() const
{
    return GetEnSharedNotebook().serviceUpdated.isSet();
}

qint64 ISharedNotebook::modificationTimestamp() const
{
    return GetEnSharedNotebook().serviceUpdated;
}

void ISharedNotebook::setModificationTimestamp(const qint64 timestamp)
{
    // TODO: verify whether it really matters
    if (timestamp >= 0) {
        GetEnSharedNotebook().serviceUpdated = timestamp;
    }
    else {
        GetEnSharedNotebook().serviceUpdated.clear();
    }
}

bool ISharedNotebook::hasShareKey() const
{
    return GetEnSharedNotebook().shareKey.isSet();
}

const QString & ISharedNotebook::shareKey() const
{
    return GetEnSharedNotebook().shareKey;
}

void ISharedNotebook::setShareKey(const QString & shareKey)
{
    GetEnSharedNotebook().shareKey = shareKey;
}

bool ISharedNotebook::hasUsername() const
{
    return GetEnSharedNotebook().username.isSet();
}

const QString & ISharedNotebook::username() const
{
    return GetEnSharedNotebook().username;
}

void ISharedNotebook::setUsername(const QString & username)
{
    GetEnSharedNotebook().username = username;
}

bool ISharedNotebook::hasPrivilegeLevel() const
{
    return GetEnSharedNotebook().privilege.isSet();
}

ISharedNotebook::SharedNotebookPrivilegeLevel ISharedNotebook::privilegeLevel() const
{
    return GetEnSharedNotebook().privilege;
}

void ISharedNotebook::setPrivilegeLevel(const SharedNotebookPrivilegeLevel privilegeLevel)
{
    GetEnSharedNotebook().privilege = privilegeLevel;
}

void ISharedNotebook::setPrivilegeLevel(const qint8 privilegeLevel)
{
    if (privilegeLevel <= static_cast<qint8>(qevercloud::SharedNotebookPrivilegeLevel::BUSINESS_FULL_ACCESS)) {
        GetEnSharedNotebook().privilege = static_cast<qevercloud::SharedNotebookPrivilegeLevel::type>(privilegeLevel);
    }
    else {
        GetEnSharedNotebook().privilege.clear();
    }
}

bool ISharedNotebook::hasAllowPreview() const
{
    return GetEnSharedNotebook().allowPreview.isSet();
}

bool ISharedNotebook::allowPreview() const
{
    return GetEnSharedNotebook().allowPreview;
}

void ISharedNotebook::setAllowPreview(const bool allowPreview)
{
    GetEnSharedNotebook().allowPreview = allowPreview;
}

bool ISharedNotebook::hasReminderNotifyEmail() const
{
    const qevercloud::SharedNotebook & sharedNotebook = GetEnSharedNotebook();
    return sharedNotebook.recipientSettings.isSet() && sharedNotebook.recipientSettings->reminderNotifyEmail.isSet();
}

bool ISharedNotebook::reminderNotifyEmail() const
{
    return GetEnSharedNotebook().recipientSettings->reminderNotifyEmail;
}

#define CHECK_AND_SET_RECIPIENT_SETTINGS \
    qevercloud::SharedNotebook & sharedNotebook = GetEnSharedNotebook(); \
    if (!sharedNotebook.recipientSettings.isSet()) { \
        sharedNotebook.recipientSettings = qevercloud::SharedNotebookRecipientSettings(); \
    }

void ISharedNotebook::setReminderNotifyEmail(const bool notifyEmail)
{
    CHECK_AND_SET_RECIPIENT_SETTINGS
    sharedNotebook.recipientSettings->reminderNotifyEmail = notifyEmail;
}

bool ISharedNotebook::hasReminderNotifyApp() const
{
    const qevercloud::SharedNotebook & sharedNotebook = GetEnSharedNotebook();
    return sharedNotebook.recipientSettings.isSet() && sharedNotebook.recipientSettings->reminderNotifyInApp.isSet();
}

bool ISharedNotebook::reminderNotifyApp() const
{
    return GetEnSharedNotebook().recipientSettings->reminderNotifyInApp;
}

void ISharedNotebook::setReminderNotifyApp(const bool notifyApp)
{
    CHECK_AND_SET_RECIPIENT_SETTINGS
    sharedNotebook.recipientSettings->reminderNotifyInApp = notifyApp;
}

#undef CHECK_AND_SET_RECIPIENT_SETTINGS

ISharedNotebook::ISharedNotebook(const ISharedNotebook & other) :
    TypeWithError(other)
{}

ISharedNotebook & ISharedNotebook::operator=(const ISharedNotebook & other)
{
    if (this != &other) {
        TypeWithError::operator=(other);
    }

    return *this;
}

QTextStream & ISharedNotebook::Print(QTextStream & strm) const
{
    strm << "SharedNotebook { \n";

    const qevercloud::SharedNotebook & sharedNotebook = GetEnSharedNotebook();

#define INSERT_DELIMITER \
    strm << "; \n";

    if (sharedNotebook.id.isSet()) {
        strm << "id: " << QString::number(sharedNotebook.id);
    }
    else {
        strm << "id is not set";
    }
    INSERT_DELIMITER

    if (sharedNotebook.userId.isSet()) {
        strm << "userId: " << QString::number(sharedNotebook.userId);
    }
    else {
        strm << "userId is not set";
    }
    INSERT_DELIMITER

    if (sharedNotebook.notebookGuid.isSet()) {
        strm << "notebookGuid: " << sharedNotebook.notebookGuid;
    }
    else {
        strm << "notebookGuid is not set";
    }
    INSERT_DELIMITER

    if (sharedNotebook.email.isSet()) {
        strm << "email: " << sharedNotebook.email;
    }
    else {
        strm << "email is not set";
    }
    INSERT_DELIMITER

    if (sharedNotebook.serviceCreated.isSet()) {
        strm << "creationTimestamp: " << QString::number(sharedNotebook.serviceCreated)
             << ", datetime: " << PrintableDateTimeFromTimestamp(sharedNotebook.serviceCreated);
    }
    else {
        strm << "creationTimestamp is not set";
    }
    INSERT_DELIMITER

    if (sharedNotebook.serviceUpdated.isSet()) {
        strm << "modificationTimestamp: " << QString::number(sharedNotebook.serviceUpdated)
             << ", datetime: " << PrintableDateTimeFromTimestamp(sharedNotebook.serviceUpdated);
    }
    else {
        strm << "modificationTimestamp is not set";
    }
    INSERT_DELIMITER

    if (sharedNotebook.shareKey.isSet()) {
        strm << "shareKey: " << sharedNotebook.shareKey;
    }
    else {
        strm << "shareKey is not set";
    }
    INSERT_DELIMITER

    if (sharedNotebook.username.isSet()) {
        strm << "username: " << sharedNotebook.username;
    }
    else {
        strm << "username is not set";
    }
    INSERT_DELIMITER

    if (sharedNotebook.privilege.isSet()) {
        // TODO: (re)implement printing of SharedNotebookPrivilegeLevel
        strm << "privilegeLevel: " << QString::number(sharedNotebook.privilege);
    }
    else {
        strm << "privilegeLevel is not set";
    }
    strm << "; \n";

    if (sharedNotebook.allowPreview.isSet()) {
        strm << "allowPreview: " << (sharedNotebook.allowPreview ? "true" : "false");
    }
    else {
        strm << "allowPreview is not set";
    }
    INSERT_DELIMITER

    if (sharedNotebook.recipientSettings.isSet())
    {
        const qevercloud::SharedNotebookRecipientSettings & recipientSettings = sharedNotebook.recipientSettings;

        if (recipientSettings.reminderNotifyEmail.isSet()) {
            strm << "reminderNotifyEmail: " << (recipientSettings.reminderNotifyEmail ? "true" : "false");
        }
        else {
            strm << "reminderNotifyEmail is not set";
        }
        INSERT_DELIMITER

        if (recipientSettings.reminderNotifyInApp.isSet()) {
            strm << "reminderNotifyApp: " << (recipientSettings.reminderNotifyInApp ? "true" : "false");
        }
        else {
            strm << "reminderNotifyApp is not set";
        }
        INSERT_DELIMITER
    }
    else {
        strm << "recipientSettings are not set; \n";
    }

#undef INSERT_DELIMITER

    return strm;
}

} // namespace qute_note

namespace qevercloud {

bool operator==(const SharedNotebook & lhs, const SharedNotebook & rhs)
{
    if (lhs.id != rhs.id) {
        return false;
    }
    else if (lhs.userId != rhs.userId) {
        return false;
    }
    else if (lhs.notebookGuid != rhs.notebookGuid) {
        return false;
    }
    else if (lhs.email != rhs.email) {
        return false;
    }
    else if (lhs.serviceCreated != rhs.serviceCreated) {
        return false;
    }
    else if (lhs.serviceUpdated != rhs.serviceUpdated) {
        return false;
    }
    else if (lhs.shareKey != rhs.shareKey) {
        return false;
    }
    else if (lhs.username != rhs.username) {
        return false;
    }
    else if (lhs.privilege != rhs.privilege) {
        return false;
    }
    else if (lhs.allowPreview != rhs.allowPreview) {
        return false;
    }
    else if (lhs.recipientSettings.isSet() != rhs.recipientSettings.isSet()) {
        return false;
    }

    if (lhs.recipientSettings.isSet())
    {
        const SharedNotebookRecipientSettings & recipientSettings = lhs.recipientSettings;
        const SharedNotebookRecipientSettings & otherRecipientSettings = rhs.recipientSettings;

        if (recipientSettings.reminderNotifyEmail != otherRecipientSettings.reminderNotifyEmail) {
            return false;
        }
        else if (recipientSettings.reminderNotifyInApp != otherRecipientSettings.reminderNotifyInApp) {
            return false;
        }
    }

    return true;
}

bool operator!=(const SharedNotebook & lhs, const SharedNotebook & rhs)
{
    return !(lhs == rhs);
}

} // namespace qevercloud

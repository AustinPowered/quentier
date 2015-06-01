#include "NoteDecryptionDialog.h"
#include "ui_NoteDecryptionDialog.h"
#include <tools/QuteNoteCheckPtr.h>
#include <logging/QuteNoteLogger.h>
#include <QSettings>

namespace qute_note {

NoteDecryptionDialog::NoteDecryptionDialog(const QString & encryptedText,
                                           const QString & cipher,
                                           const QString & hint, const size_t keyLength,
                                           QSharedPointer<EncryptionManager> encryptionManager,
                                           DecryptedTextCachePtr decryptedTextCache,
                                           QWidget * parent) :
    QDialog(parent),
    m_pUI(new Ui::NoteDecryptionDialog),
    m_encryptedText(encryptedText),
    m_cipher(cipher),
    m_hint(hint),
    m_cachedDecryptedText(),
    m_keyLength(keyLength),
    m_encryptionManager(encryptionManager),
    m_decryptedTextCache(decryptedTextCache)
{
    m_pUI->setupUi(this);
    QUTE_NOTE_CHECK_PTR(encryptionManager.data())
    QUTE_NOTE_CHECK_PTR(decryptedTextCache.data())

    setHint(m_hint);

    bool rememberPassphraseForSessionDefault = false;
    QSettings settings;
    QVariant rememberPassphraseForSessionSetting = settings.value("General/rememberPassphraseForSession");
    if (!rememberPassphraseForSessionSetting.isNull()) {
        rememberPassphraseForSessionDefault = rememberPassphraseForSessionSetting.toBool();
    }

    setRememberPassphraseDefaultState(rememberPassphraseForSessionDefault);
    m_pUI->onErrorTextLabel->setVisible(false);

    QObject::connect(m_pUI->rememberPasswordCheckBox, SIGNAL(stateChanged(int)),
                     this, SLOT(onRememberPassphraseStateChanged()));
}

NoteDecryptionDialog::~NoteDecryptionDialog()
{
    delete m_pUI;
}

QString NoteDecryptionDialog::passphrase() const
{
    return m_pUI->passwordLineEdit->text();
}

bool NoteDecryptionDialog::rememberPassphrase() const
{
    return m_pUI->rememberPasswordCheckBox->isChecked();
}

QString NoteDecryptionDialog::decryptedText() const
{
    return m_cachedDecryptedText;
}

void NoteDecryptionDialog::setError(const QString & error)
{
    m_pUI->onErrorTextLabel->setText(error);
    m_pUI->onErrorTextLabel->setVisible(true);
}

void NoteDecryptionDialog::setHint(const QString & hint)
{
    m_pUI->hintLabel->setText(QObject::tr("Hint: ") +
                              (hint.isEmpty() ? QObject::tr("No hint available") : hint));
}

void NoteDecryptionDialog::setRememberPassphraseDefaultState(const bool checked)
{
    m_pUI->rememberPasswordCheckBox->setChecked(checked);
}

void NoteDecryptionDialog::onRememberPassphraseStateChanged()
{
    QSettings settings;
    if (!settings.isWritable()) {
        QNINFO("Can't persist remember passphrase for session setting: settings are not writable");
    }
    else {
        settings.setValue("General/rememberPassphraseForSession",
                          QVariant(m_pUI->rememberPasswordCheckBox->isChecked()));
    }
}

void NoteDecryptionDialog::accept()
{
    QString passphrase = m_pUI->passwordLineEdit->text();

    const QPair<QString, bool> * cachedEntry = m_decryptedTextCache->object(passphrase);
    if (cachedEntry)
    {
        QNTRACE("Found cached decrypted text for passphrase");
        m_cachedDecryptedText = cachedEntry->first;
    }
    else
    {
        QString errorDescription;
        bool res = m_encryptionManager->decrypt(m_encryptedText, passphrase, m_cipher,
                                                m_keyLength, m_cachedDecryptedText,
                                                errorDescription);
        if (!res) {
            errorDescription.prepend(QT_TR_NOOP("Failed attempt to decrypt text: "));
            QNINFO(errorDescription);
            setError(errorDescription);
            return;
        }

        const bool rememberForSession = m_pUI->rememberPasswordCheckBox->isChecked();
        QPair<QString, bool> cacheEntry(m_cachedDecryptedText, rememberForSession);
        if (rememberForSession)
        {
            m_decryptedTextCache->insert(m_encryptedText, &cacheEntry);
            QNTRACE("Cached decrypted text by encryptedText (per session passphrase storage): "
                    << m_encryptedText);
        }
        else
        {
            m_decryptedTextCache->insert(passphrase, &cacheEntry);
            QNTRACE("Cached decrypted text by passphrase (no per session passphrase storage, "
                    "just the internal cache for faster handling");
        }
    }

    QDialog::accept();
}

} // namespace qute_note

#include "ENMLConverterTests.h"
#include <client/enml/ENMLConverter.h>
#include <logging/QuteNoteLogger.h>
#include <QXmlStreamReader>
#include <QFile>

void __initENMLConversionTestResources();

namespace qute_note {
namespace test {

bool convertNoteToHtmlAndBackImpl(const QString & noteContent, QString & error, DecryptedTextCachePtr decryptedTextCache = DecryptedTextCachePtr());
bool compareEnml(const QString & original, const QString & processed, QString & error);

bool convertSimpleNoteToHtmlAndBack(QString & error)
{
    QString noteContent = "<en-note>"
                          "<span style=\"font-weight:bold;color:red;\">Here's some bold red text!</span>"
                          "<div>Hickory, dickory, dock,</div>"
                          "<div>The mouse ran up the clock.</div>"
                          "<div>The clock struck one,</div>"
                          "<div>The mouse ran down,</div>"
                          "<div>Hickory, dickory, dock.</div>"
                          "<div><br/></div>"
                          "<div>-- Author unknown</div>"
                          "</en-note>";
    return convertNoteToHtmlAndBackImpl(noteContent, error);
}

bool convertNoteWithToDoTagsToHtmlAndBack(QString & error)
{
    QString noteContent = "<en-note>"
                          "<h1>Hello, world!</h1>"
                          "<div>Here's the note with some todo tags</div>"
                          "<en-todo/>An item that I haven't completed yet"
                          "<br/>"
                          "<en-todo checked=\"true\"/>A completed item"
                          "<br/>"
                          "<en-todo checked=\"false\"/>Another not yet completed item"
                          "</en-note>";
    return convertNoteToHtmlAndBackImpl(noteContent, error);
}

bool convertNoteWithEncryptionToHtmlAndBack(QString & error)
{
    QString noteContent = "<en-note>"
                          "<h3>This note contains encrypted text</h3>"
                          "<br/>"
                          "<div>Here's the encrypted text containing only the hint attribute</div>"
                          "<en-crypt hint=\"this is my rifle, this is my gun\">RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
                          "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
                          "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
                          "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
                          "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
                          "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
                          "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt>"
                          "<br/><div>Here's the encrypted text containing only the cipher attribute</div>"
                          "<en-crypt cipher=\"AES\">RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
                          "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
                          "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
                          "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
                          "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
                          "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
                          "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt>"
                          "<br/><div>Here's the encrypted text containing only the length attribute</div>"
                          "<en-crypt length=\"128\">RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
                          "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
                          "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
                          "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
                          "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
                          "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
                          "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt>"
                          "<br/><div>Here's the encrypted text containing cipher and length attributes</div>"
                          "<en-crypt cipher=\"AES\" length=\"128\">RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
                          "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
                          "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
                          "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
                          "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
                          "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
                          "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt>"
                          "<br/><div>Here's the encrypted text containing cipher and hint attributes</div>"
                          "<en-crypt hint=\"this is my rifle, this is my gun\" cipher=\"AES\">"
                          "RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
                          "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
                          "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
                          "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
                          "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
                          "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
                          "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt>"
                          "<br/><div>Here's the encrypted text containing length and hint attributes</div>"
                          "<en-crypt hint=\"this is my rifle, this is my gun\" length=\"128\">"
                          "RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
                          "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
                          "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
                          "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
                          "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
                          "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
                          "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt>"
                          "<br/><div>Here's the encrypted text containing cipher, length and hint attributes</div>"
                          "<en-crypt hint=\"this is my rifle, this is my gun\" cipher=\"AES\" length=\"128\">"
                          "RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
                          "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
                          "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
                          "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
                          "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
                          "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
                          "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt>"
                          "<div>Here's the text encrypted with RC2 which should reside in decrypted text cache</div>"
                          "<en-crypt hint=\"my_own_encryption_key_1988\">"
                          "K+sUXSxI2Mt075+pSDxR/gnCNIEnk5XH1P/D0Eie17"
                          "JIWgGnNo5QeMo3L0OeBORARGvVtBlmJx6vJY2Ij/2En"
                          "MVy6/aifSdZXAxRlfnTLvI1IpVgHpTMzEfy6zBVMo+V"
                          "Bt2KglA+7L0iSjA0hs3GEHI6ZgzhGfGj</en-crypt>"
                          "<div>Here's the text encrypted with AES which should reside in decrypted text cache</div>"
                          "<en-crypt hint=\"MyEncryptionPassword\">"
                          "RU5DMBwXjfKR+x9ksjSJhtiF+CxfwXn2Hf/WqdVwLwJDX9YX5R34Z5SBMSCIOFF"
                          "r1MUeNkzHGVP5fHEppUlIExDG/Vpjh9KK1uu0VqTFoUWA0IXAAMA5eHnbxhBrjvL"
                          "3CoTQV7prRqJVLpUX77Q0vbNims1quxVWaf7+uVeK60YoiJnSOHvEYptoOs1FVfZ"
                          "AwnDDBoCUOsAb2nCh2UZ6LSFneb58xQ/6WeoQ7QDDHLSoUIXn</en-crypt>"
                          "</en-note>";
    DecryptedTextCachePtr decryptedTextCache(new DecryptedTextCachePtr::value_type);
    Q_UNUSED(decryptedTextCache->insert("K+sUXSxI2Mt075+pSDxR/gnCNIEnk5XH1P/D0Eie17"
                                        "JIWgGnNo5QeMo3L0OeBORARGvVtBlmJx6vJY2Ij/2En"
                                        "MVy6/aifSdZXAxRlfnTLvI1IpVgHpTMzEfy6zBVMo+V"
                                        "Bt2KglA+7L0iSjA0hs3GEHI6ZgzhGfGj",
                                        QPair<QString,bool>("<span style=\"display: inline !important; float: none; \">"
                                                            "Ok, here's a piece of text I'm going to encrypt now</span>", true)));
    Q_UNUSED(decryptedTextCache->insert("RU5DMBwXjfKR+x9ksjSJhtiF+CxfwXn2Hf/WqdVwLwJDX9YX5R34Z5SBMSCIOFF"
                                        "r1MUeNkzHGVP5fHEppUlIExDG/Vpjh9KK1uu0VqTFoUWA0IXAAMA5eHnbxhBrjvL"
                                        "3CoTQV7prRqJVLpUX77Q0vbNims1quxVWaf7+uVeK60YoiJnSOHvEYptoOs1FVfZ"
                                        "AwnDDBoCUOsAb2nCh2UZ6LSFneb58xQ/6WeoQ7QDDHLSoUIXn",
                                        QPair<QString,bool>("Sample text said to be the decrypted one", true)));
    return convertNoteToHtmlAndBackImpl(noteContent, error, decryptedTextCache);
}

bool convertNoteWithResourcesToHtmlAndBack(QString & error)
{
    QString noteContent = "<en-note>"
                          "<div>Here's the note with some embedded resources</div>"
                          "<br/>"
                          "<div>The first resource: simple image</div>"
                          "<en-media width=\"640\" height=\"480\" align=\"right\" type=\"image/jpeg\" hash=\"f03c1c2d96bc67eda02968c8b5af9008\"/>"
                          "<div>The second resource: embedded pdf</div>"
                          "<en-media width=\"600\" height=\"800\" title=\"My cool pdf\" type=\"application/pdf\" hash=\"6051a24c8677fd21c65c1566654c228\"/>"
                          "</en-note>";
    return convertNoteToHtmlAndBackImpl(noteContent, error);
}

bool convertComplexNoteToHtmlAndBack(QString & error)
{
    __initENMLConversionTestResources();

    QFile file(":/tests/complexNote1.txt");
    if (!file.open(QIODevice::ReadOnly)) {
        error = "Can't open resource with complex note #1 for reading";
        return false;
    }

    QString noteContent = file.readAll();
    return convertNoteToHtmlAndBackImpl(noteContent, error);
}

bool convertNoteToHtmlAndBackImpl(const QString & noteContent, QString & error, DecryptedTextCachePtr decryptedTextCachePtr)
{
    QString originalNoteContent = noteContent;

    ENMLConverter converter;
    QString html;
    bool res = converter.noteContentToHtml(originalNoteContent, html, error, decryptedTextCachePtr);
    if (!res) {
        error.prepend("Unable to convert the note content to HTML: ");
        QNWARNING(error);
        return false;
    }

    html.prepend("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">"
                 "<html><head>"
                 "<meta http-equiv=\"Content-Type\" content=\"text/html\" charset=\"UTF-8\" />"
                 "<title></title></head>");
    html.append("</html>");

    QString processedNoteContent;
    res = converter.htmlToNoteContent(html, processedNoteContent, error);
    if (!res) {
        error.prepend("Unable to convert HTML to note content: ");
        QNWARNING(error);
        return false;
    }

    res = compareEnml(originalNoteContent, processedNoteContent, error);
    if (!res) {
        QNWARNING("\n\nHTML: " << html);
        return false;
    }

    return true;
}

bool compareEnml(const QString & original, const QString & processed, QString & error)
{
    QString originalSimplified = original.simplified();
    QString processedSimplified = processed.simplified();

    QXmlStreamReader readerOriginal(originalSimplified);
    QXmlStreamReader readerProcessed(processedSimplified);

#define PRINT_WARNING(err) \
    QNWARNING(err << "\n\nContext in the original ENML: <" << readerOriginal.name() << ">: " << readerOriginal.readElementText() \
              << "\n\nContext in the processed ENML: <" << readerProcessed.name() << ">: " << readerProcessed.readElementText() \
              << "\n\nFull simplified original ENML: " << originalSimplified << "\n\nFull simplified processed ENML: " << processedSimplified);

    while(!readerOriginal.atEnd() && !readerProcessed.atEnd())
    {
        Q_UNUSED(readerOriginal.readNext());
        Q_UNUSED(readerProcessed.readNext());

        bool checkForEmptyCharacters = true;
        while(checkForEmptyCharacters)
        {
            if (readerOriginal.isCharacters())
            {
                QString textOriginal = readerOriginal.readElementText();
                if (textOriginal.simplified().isEmpty()) {
                    Q_UNUSED(readerOriginal.readNext());
                    continue;
                }
            }

            checkForEmptyCharacters = false;
        }

        checkForEmptyCharacters = true;
        while(checkForEmptyCharacters)
        {
            if (readerProcessed.isCharacters())
            {
                QString textProcessed = readerProcessed.readElementText();
                if (textProcessed.simplified().isEmpty()) {
                    Q_UNUSED(readerProcessed.readNext());
                    continue;
                }
            }

            checkForEmptyCharacters = false;
        }

        if (readerOriginal.isStartDocument() && !readerProcessed.isStartDocument()) {
            error = "QXmlStreamReader of the original ENML is at the start of the document while the reader of the processed ENML is not";
            PRINT_WARNING(error);
            return false;
        }

        if (readerOriginal.isStartElement())
        {
            if (!readerProcessed.isStartElement()) {
                error = "QXmlStreamReader of the original ENML is at the start of the element while the reader of the processed ENML is not";
                PRINT_WARNING(error);
                return false;
            }

            QStringRef originalName = readerOriginal.name();
            QStringRef processedName = readerProcessed.name();
            if (originalName != processedName) {
                error = "Found a tag in the original ENML which name doesn't match the name of the corresponding element in the processed ENML";
                PRINT_WARNING(error);
                return false;
            }

            QXmlStreamAttributes originalAttributes = readerOriginal.attributes();
            QXmlStreamAttributes processedAttributes = readerProcessed.attributes();

            if (originalName == "en-todo")
            {
                bool originalChecked = false;
                if (originalAttributes.hasAttribute("checked")) {
                    QStringRef originalCheckedStr = originalAttributes.value("checked");
                    if (originalCheckedStr == "true") {
                        originalChecked = true;
                    }
                }

                bool processedChecked = false;
                if (processedAttributes.hasAttribute("checked")) {
                    QStringRef processedCheckedStr = processedAttributes.value("checked");
                    if (processedCheckedStr == "true") {
                        processedChecked = true;
                    }
                }

                if (originalChecked != processedChecked) {
                    error = "Checked state of ToDo item from the original ENML doesn't match the state of the item from "
                            "the processed ENML";
                    PRINT_WARNING(error);
                    return false;
                }
            }
            else
            {
                const int numOriginalAttributes = originalAttributes.size();
                const int numProcessedAttributes = processedAttributes.size();

                if (numOriginalAttributes != numProcessedAttributes) {
                    error = "The number of attributes in tag " + originalName.toString() + " doesn't match in the original and the processed ENMLs";
                    PRINT_WARNING(error);
                    return false;
                }

                for(int i = 0; i < numOriginalAttributes; ++i)
                {
                    const QXmlStreamAttribute & originalAttribute = originalAttributes[i];
                    const QXmlStreamAttribute & processedAttribute = processedAttributes[i];

                    if (originalAttribute != processedAttribute) {
                        error = "The corresponding attributes within the original and the processed ENMLs do not match";
                        QNWARNING(error << "; original ENML: " << originalSimplified << "\nProcessed ENML: " << processedSimplified
                                  << ", index within attributes = " << i << "\nOriginal attribute: name = "
                                  << originalAttribute.name() << ", namespace uri = " << originalAttribute.namespaceUri()
                                  << ", qualified name = " << originalAttribute.qualifiedName() << ", is default = "
                                  << (originalAttribute.isDefault() ? "true" : "false") << ", value = " << originalAttribute.value()
                                  << ", prefix = " << originalAttribute.prefix() << "\nProcessed attribute: name = "
                                  << processedAttribute.name() << ", namespace uri = " << processedAttribute.namespaceUri()
                                  << ", qualified name = " << processedAttribute.qualifiedName() << ", is default = "
                                  << (processedAttribute.isDefault() ? "true" : "false") << ", value = " << processedAttribute.value()
                                  << ", prefix = " << processedAttribute.prefix());
                        return false;
                    }
                }
            }
        }

        if (readerOriginal.isEndElement() && !readerProcessed.isEndElement()) {
            error = "QXmlStreamReader of the original ENML is at the end of the element while the reader of the processed ENML is not";
            PRINT_WARNING(error);
            return false;
        }

        if (readerOriginal.isCharacters())
        {
            if (!readerProcessed.isCharacters())
            {
                QStringRef textOriginal = readerOriginal.text();
                if (textOriginal.toString().simplified().isEmpty()) {
                    continue;
                }

                error = "QXmlStreamReader of the original ENML points to characters while the reader of the processed ENML does not";
                QNWARNING(error << "; original ENML: " << originalSimplified << "\nProcessed ENML: " << processedSimplified);
                return false;
            }

            if (readerOriginal.isCDATA())
            {
                if (!readerProcessed.isCDATA()) {
                    error = "QXmlStreamReader of the original ENML points to CDATA while the reader of the processed ENML does not";
                    QNWARNING(error << "; original ENML: " << originalSimplified << "\nProcessed ENML: " << processedSimplified);
                    return false;
                }
            }

            QString textOriginal = readerOriginal.text().toString().simplified();
            QString textProcessed = readerProcessed.text().toString().simplified();

            if (textOriginal != textProcessed) {
                error = "The text extracted from the corresponding elements of both the original ENML and the processed ENML does not match";
                QNWARNING(error << "; original ENML: " << originalSimplified << "\nProcessed ENML: " << processedSimplified
                          << "\nOriginal element text: " << textOriginal << "\nProcessed element text: " << textProcessed);
                return false;
            }
        }

        if (readerOriginal.isEndDocument() && !readerProcessed.isEndDocument()) {
            error = "QXmlStreamReader of the original ENML is at the end of the document while the reader of the processed ENML is not";
            QNWARNING(error << "; original ENML: " << originalSimplified << "\nProcessed ENML: " << processedSimplified);
            return false;
        }
    }

    if (readerOriginal.atEnd() != readerProcessed.atEnd()) {
        error = "QXmlStreamReaders for the original ENML and the processed ENML have not both came to their ends after the checking loop";
        QNWARNING(error << "; original ENML: " << originalSimplified << "\nProcessed ENML: " << processedSimplified);
        return false;
    }

    return true;
}

} // namespace test
} // namespace qute_note

void __initENMLConversionTestResources()
{
    Q_INIT_RESOURCE(test_resources);
}


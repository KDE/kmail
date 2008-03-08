/*
    This file was taken from the KDE 4.x libraries and backported to Qt 3.

    Copyright (C) 1999 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2007 Nick Shaforostoff (shafff@ukr.net)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/
#ifndef ENCODINGDETECTOR_H
#define ENCODINGDETECTOR_H

#include <qstring.h>

class QTextCodec;
class QTextDecoder;
class EncodingDetectorPrivate;

/**
 * @short Provides encoding detection capabilities.
 *
 * Searches for encoding declaration inside raw data -- meta and xml tags. 
 * In the case it can't find it, uses heuristics for specified language.
 *
 * If it finds unicode BOM marks, it changes encoding regardless of what the user has told
 *
 * Intended lifetime of the object: one instance per document.
 *
 * Typical use:
 * \code
 * QByteArray data;
 * ...
 * EncodingDetector detector;
 * detector.setAutoDetectLanguage(EncodingDetector::Cyrillic);
 * QString out=detector.decode(data);
 * \endcode
 *
 *
 * Do not mix decode() with decodeWithBuffering()
 *
 * @short Guess encoding of char array
 *
 */
class EncodingDetector
{
public:
    enum EncodingChoiceSource
    {
        DefaultEncoding,
        AutoDetectedEncoding,
        BOM,
        EncodingFromXMLHeader,
        EncodingFromMetaTag,
        EncodingFromHTTPHeader,
        UserChosenEncoding
    };

    enum AutoDetectScript
    {
        None,
        SemiautomaticDetection,
        Arabic,
        Baltic,
        CentralEuropean,
        ChineseSimplified,
        ChineseTraditional,
        Cyrillic,
        Greek,
        Hebrew,
        Japanese,
        Korean,
        NorthernSaami,
        SouthEasternEurope,
        Thai,
        Turkish,
        Unicode,
        WesternEuropean
    };

    /**
     * Default codec is latin1 (as html spec says), EncodingChoiceSource is default, AutoDetectScript=Semiautomatic
     */
    EncodingDetector();

    /**
     * Allows to set Default codec, EncodingChoiceSource, AutoDetectScript
     */
    EncodingDetector(QTextCodec* codec, EncodingChoiceSource source, AutoDetectScript script=None);
    ~EncodingDetector();

    //const QTextCodec* codec() const;

    /**
    * @returns true if specified encoding was recognized
    */
    bool setEncoding(const char *encoding, EncodingChoiceSource type);

    /**
    * Convenience method.
    * @returns mime name of detected encoding
    */
    const char* encoding() const;

    bool visuallyOrdered() const;

//     void setAutoDetectLanguage( const QString& );
//     const QString& autoDetectLanguage() const;

    void setAutoDetectLanguage( AutoDetectScript );
    AutoDetectScript autoDetectLanguage() const;

    EncodingChoiceSource encodingChoiceSource() const;

    /**
    * Analyze text data.
    * @returns true if there was enough data for accurate detection
    */
    bool analyze( const char *data, int len );

    /**
    * Analyze text data.
    * @returns true if there was enough data for accurate detection
    */
    bool analyze( const QByteArray &data );

    /**
     * Takes lang name _after_ it were i18n()'ed
     */
    static AutoDetectScript scriptForName(const QString& lang);
    static QString nameForScript(AutoDetectScript);
    static AutoDetectScript scriptForLanguageCode(const QString &lang);
    static bool hasAutoDetectionForScript(AutoDetectScript);

protected:
    /**
     * Check if we are really utf8. Taken from kate
     *
     * @returns true if current encoding is utf8 and the text cannot be in this encoding
     *
     * Please somebody read http://de.wikipedia.org/wiki/UTF-8 and check this code...
     */
    bool errorsIfUtf8 (const char* data, int length);

    /**
    * @returns QTextDecoder for detected encoding
    */
    QTextDecoder* decoder();

private:
    EncodingDetectorPrivate* const d;
};

#endif

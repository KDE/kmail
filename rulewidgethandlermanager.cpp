/*  -*- mode: C++; c-file-style: "gnu" -*-
    rulewidgethandlermanager.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2004 Ingo Kloecker <kloecker@kde.org>

    KMail is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/


#include "rulewidgethandlermanager.h"

#include "interfaces/rulewidgethandler.h"
#include "stl_util.h"

#include <kdebug.h>
#include <kiconloader.h>

#include <QStackedWidget>
#include <QString>
#include <QObject>

#include <assert.h>

#include <algorithm>
using std::for_each;
using std::remove;

KMail::RuleWidgetHandlerManager * KMail::RuleWidgetHandlerManager::self = 0;

namespace {
  class TextRuleWidgetHandler : public KMail::RuleWidgetHandler {
  public:
    TextRuleWidgetHandler() : KMail::RuleWidgetHandler() {}
    ~TextRuleWidgetHandler() {}

    QWidget * createFunctionWidget( int number,
                                    QStackedWidget *functionStack,
                                    const QObject *receiver ) const;
    QWidget * createValueWidget( int number,
                                 QStackedWidget *valueStack,
                                 const QObject *receiver ) const;
    KMSearchRule::Function function( const QByteArray & field,
                                     const QStackedWidget *functionStack ) const;
    QString value( const QByteArray & field,
                   const QStackedWidget *functionStack,
                   const QStackedWidget *valueStack ) const;
    QString prettyValue( const QByteArray & field,
                         const QStackedWidget *functionStack,
                         const QStackedWidget *valueStack ) const;
    bool handlesField( const QByteArray & field ) const;
    void reset( QStackedWidget *functionStack,
                QStackedWidget *valueStack ) const;
    bool setRule( QStackedWidget *functionStack,
                  QStackedWidget *valueStack,
                  const KMSearchRule *rule ) const;
    bool update( const QByteArray & field,
                 QStackedWidget *functionStack,
                 QStackedWidget *valueStack ) const;

 private:
    KMSearchRule::Function currentFunction( const QStackedWidget *functionStack ) const;
    QString currentValue( const QStackedWidget *valueStack,
                          KMSearchRule::Function func ) const;
  };

  class MessageRuleWidgetHandler : public KMail::RuleWidgetHandler {
  public:
    MessageRuleWidgetHandler() : KMail::RuleWidgetHandler() {}
    ~MessageRuleWidgetHandler() {}

    QWidget * createFunctionWidget( int number,
                                    QStackedWidget *functionStack,
                                    const QObject *receiver ) const;
    QWidget * createValueWidget( int number,
                                 QStackedWidget *valueStack,
                                 const QObject *receiver ) const;
    KMSearchRule::Function function( const QByteArray & field,
                                     const QStackedWidget *functionStack ) const;
    QString value( const QByteArray & field,
                   const QStackedWidget *functionStack,
                   const QStackedWidget *valueStack ) const;
    QString prettyValue( const QByteArray & field,
                         const QStackedWidget *functionStack,
                         const QStackedWidget *valueStack ) const;
    bool handlesField( const QByteArray & field ) const;
    void reset( QStackedWidget *functionStack,
                QStackedWidget *valueStack ) const;
    bool setRule( QStackedWidget *functionStack,
                  QStackedWidget *valueStack,
                  const KMSearchRule *rule ) const;
    bool update( const QByteArray & field,
                 QStackedWidget *functionStack,
                 QStackedWidget *valueStack ) const;

 private:
    KMSearchRule::Function currentFunction( const QStackedWidget *functionStack ) const;
    QString currentValue( const QStackedWidget *valueStack,
                          KMSearchRule::Function func ) const;
  };


  class StatusRuleWidgetHandler : public KMail::RuleWidgetHandler {
  public:
    StatusRuleWidgetHandler() : KMail::RuleWidgetHandler() {}
    ~StatusRuleWidgetHandler() {}

    QWidget * createFunctionWidget( int number,
                                    QStackedWidget *functionStack,
                                    const QObject *receiver ) const;
    QWidget * createValueWidget( int number,
                                 QStackedWidget *valueStack,
                                 const QObject *receiver ) const;
    KMSearchRule::Function function( const QByteArray & field,
                                     const QStackedWidget *functionStack ) const;
    QString value( const QByteArray & field,
                   const QStackedWidget *functionStack,
                   const QStackedWidget *valueStack ) const;
    QString prettyValue( const QByteArray & field,
                         const QStackedWidget *functionStack,
                         const QStackedWidget *valueStack ) const;
    bool handlesField( const QByteArray & field ) const;
    void reset( QStackedWidget *functionStack,
                QStackedWidget *valueStack ) const;
    bool setRule( QStackedWidget *functionStack,
                  QStackedWidget *valueStack,
                  const KMSearchRule *rule ) const;
    bool update( const QByteArray & field,
                 QStackedWidget *functionStack,
                 QStackedWidget *valueStack ) const;

  private:
    KMSearchRule::Function currentFunction( const QStackedWidget *functionStack ) const;
    int currentStatusValue( const QStackedWidget *valueStack ) const;
  };

  class NumericRuleWidgetHandler : public KMail::RuleWidgetHandler {
  public:
    NumericRuleWidgetHandler() : KMail::RuleWidgetHandler() {}
    ~NumericRuleWidgetHandler() {}

    QWidget * createFunctionWidget( int number,
                                    QStackedWidget *functionStack,
                                    const QObject *receiver ) const;
    QWidget * createValueWidget( int number,
                                 QStackedWidget *valueStack,
                                 const QObject *receiver ) const;
    KMSearchRule::Function function( const QByteArray & field,
                                     const QStackedWidget *functionStack ) const;
    QString value( const QByteArray & field,
                   const QStackedWidget *functionStack,
                   const QStackedWidget *valueStack ) const;
    QString prettyValue( const QByteArray & field,
                         const QStackedWidget *functionStack,
                         const QStackedWidget *valueStack ) const;
    bool handlesField( const QByteArray & field ) const;
    void reset( QStackedWidget *functionStack,
                QStackedWidget *valueStack ) const;
    bool setRule( QStackedWidget *functionStack,
                  QStackedWidget *valueStack,
                  const KMSearchRule *rule ) const;
    bool update( const QByteArray & field,
                 QStackedWidget *functionStack,
                 QStackedWidget *valueStack ) const;

  private:
    KMSearchRule::Function currentFunction( const QStackedWidget *functionStack ) const;
    QString currentValue( const QStackedWidget *valueStack ) const;
  };
}

KMail::RuleWidgetHandlerManager::RuleWidgetHandlerManager()
{
  registerHandler( new NumericRuleWidgetHandler() );
  registerHandler( new StatusRuleWidgetHandler() );
  registerHandler( new MessageRuleWidgetHandler() );
   // the TextRuleWidgetHandler is the fallback handler, so it has to be added
  // as last handler
  registerHandler( new TextRuleWidgetHandler() );
}

KMail::RuleWidgetHandlerManager::~RuleWidgetHandlerManager()
{
  for_each( mHandlers.begin(), mHandlers.end(),
	    DeleteAndSetToZero<RuleWidgetHandler>() );
}

void KMail::RuleWidgetHandlerManager::registerHandler( const RuleWidgetHandler * handler )
{
  if ( !handler )
    return;
  unregisterHandler( handler ); // don't produce duplicates
  mHandlers.push_back( handler );
}

void KMail::RuleWidgetHandlerManager::unregisterHandler( const RuleWidgetHandler * handler )
{
  // don't delete them, only remove them from the list!
  mHandlers.erase( remove( mHandlers.begin(), mHandlers.end(), handler ), mHandlers.end() );
}

namespace {
  /** Returns the number of immediate children of parent with the given object
      name. Used by RuleWidgetHandlerManager::createWidgets().
  */
  int childCount( const QObject *parent, const QString &objName )
  {
    QObjectList list = parent->children();
    QObject *item;
    int count = 0;
    foreach( item, list ) {
      if ( item->objectName() == objName )
        count++;
    }
    return count;
  }
}

void KMail::RuleWidgetHandlerManager::createWidgets( QStackedWidget *functionStack,
                                                     QStackedWidget *valueStack,
                                                     const QObject *receiver ) const
{
  for ( const_iterator it = mHandlers.begin(); it != mHandlers.end(); ++it ) {
    QWidget *w = 0;
    for ( int i = 0;
          ( w = (*it)->createFunctionWidget( i, functionStack, receiver ) );
          ++i ) {
      if ( childCount( functionStack, w->objectName() ) < 2 ) {
        // there wasn't already a widget with this name, so add this widget
        functionStack->addWidget( w );
      }
      else {
        // there was already a widget with this name, so discard this widget
        kDebug(5006) <<"RuleWidgetHandlerManager::createWidgets:"
                      << w->objectName() << "already exists in functionStack";
        delete w; w = 0;
      }
    }
    for ( int i = 0;
          ( w = (*it)->createValueWidget( i, valueStack, receiver ) );
          ++i ) {
      if ( childCount( valueStack, w->objectName() ) < 2 ) {
        // there wasn't already a widget with this name, so add this widget
        valueStack->addWidget( w );
      }
      else {
        // there was already a widget with this name, so discard this widget
        kDebug(5006) <<"RuleWidgetHandlerManager::createWidgets:"
                      << w->objectName() << "already exists in valueStack";
        delete w; w = 0;
      }
    }
  }
}

KMSearchRule::Function KMail::RuleWidgetHandlerManager::function( const QByteArray& field,
                                                                  const QStackedWidget *functionStack ) const
{
  for ( const_iterator it = mHandlers.begin(); it != mHandlers.end(); ++it ) {
    const KMSearchRule::Function func = (*it)->function( field,
                                                         functionStack );
    if ( func != KMSearchRule::FuncNone )
      return func;
  }
  return KMSearchRule::FuncNone;
}

QString KMail::RuleWidgetHandlerManager::value( const QByteArray& field,
                                                const QStackedWidget *functionStack,
                                                const QStackedWidget *valueStack ) const
{
  for ( const_iterator it = mHandlers.begin(); it != mHandlers.end(); ++it ) {
    const QString val = (*it)->value( field, functionStack, valueStack );
    if ( !val.isEmpty() )
      return val;
  }
  return QString();
}

QString KMail::RuleWidgetHandlerManager::prettyValue( const QByteArray& field,
                                                      const QStackedWidget *functionStack,
                                                      const QStackedWidget *valueStack ) const
{
  for ( const_iterator it = mHandlers.begin(); it != mHandlers.end(); ++it ) {
    const QString val = (*it)->prettyValue( field, functionStack, valueStack );
    if ( !val.isEmpty() )
      return val;
  }
  return QString();
}

void KMail::RuleWidgetHandlerManager::reset( QStackedWidget *functionStack,
                                             QStackedWidget *valueStack ) const
{
  for ( const_iterator it = mHandlers.begin(); it != mHandlers.end(); ++it ) {
    (*it)->reset( functionStack, valueStack );
  }
  update( "", functionStack, valueStack );
}

void KMail::RuleWidgetHandlerManager::setRule( QStackedWidget *functionStack,
                                               QStackedWidget *valueStack,
                                               const KMSearchRule *rule ) const
{
  assert( rule );
  reset( functionStack, valueStack );
  for ( const_iterator it = mHandlers.begin(); it != mHandlers.end(); ++it ) {
    if ( (*it)->setRule( functionStack, valueStack, rule ) )
      return;
  }
}

void KMail::RuleWidgetHandlerManager::update( const QByteArray &field,
                                              QStackedWidget *functionStack,
                                              QStackedWidget *valueStack ) const
{
  //kDebug(5006) <<"RuleWidgetHandlerManager::update( \"" << field
  //              << "\", ... )";
  for ( const_iterator it = mHandlers.begin(); it != mHandlers.end(); ++it ) {
    if ( (*it)->update( field, functionStack, valueStack ) )
      return;
  }
}

//-----------------------------------------------------------------------------

namespace {
  // FIXME (Qt >= 4.0):
  // This is a simplified and constified copy of QObject::child(). According
  // to a comment in qobject.h QObject::child() will be made const in Qt 4.0.
  // So once we require Qt 4.0 this can be removed.
  QObject* QObject_child_const( const QObject *parent,
                                const char *objName )
  {
    const QObjectList list = parent->children();
    if ( list.isEmpty() )
      return 0;

    QObject *obj = 0;
    Q_FOREACH( obj, list ) {
      if ( !objName || qstrcmp( objName, obj->objectName().toLatin1().data() ) == 0 )
        break;
    }
    return obj;
  }
}

//-----------------------------------------------------------------------------

// these includes are temporary and should not be needed for the code
// above this line, so they appear only here:
#include "kmaddrbook.h"
#include "kmsearchpattern.h"
#include "regexplineedit.h"
using KMail::RegExpLineEdit;

#include <klocale.h>
#include <knuminput.h>

#include <QComboBox>
#include <QLabel>

//=============================================================================
//
// class TextRuleWidgetHandler
//
//=============================================================================

namespace {
  // also see KMSearchRule::matches() and KMSearchRule::Function
  // if you change the following strings!
  static const struct {
    const KMSearchRule::Function id;
    const char *displayName;
  } TextFunctions[] = {
    { KMSearchRule::FuncContains,           I18N_NOOP( "contains" )          },
    { KMSearchRule::FuncContainsNot,        I18N_NOOP( "does not contain" )   },
    { KMSearchRule::FuncEquals,             I18N_NOOP( "equals" )            },
    { KMSearchRule::FuncNotEqual,           I18N_NOOP( "does not equal" )     },
    { KMSearchRule::FuncRegExp,             I18N_NOOP( "matches regular expr." ) },
    { KMSearchRule::FuncNotRegExp,          I18N_NOOP( "does not match reg. expr." ) },
    { KMSearchRule::FuncIsInAddressbook,    I18N_NOOP( "is in address book" ) },
    { KMSearchRule::FuncIsNotInAddressbook, I18N_NOOP( "is not in address book" ) },
    { KMSearchRule::FuncIsInCategory,       I18N_NOOP( "is in category" ) },
    { KMSearchRule::FuncIsNotInCategory,    I18N_NOOP( "is not in category" ) }
  };
  static const int TextFunctionCount =
    sizeof( TextFunctions ) / sizeof( *TextFunctions );

  //---------------------------------------------------------------------------

  QWidget * TextRuleWidgetHandler::createFunctionWidget( int number,
                                                         QStackedWidget *functionStack,
                                                         const QObject *receiver ) const
  {
    if ( number != 0 )
      return 0;

    QComboBox *funcCombo = new QComboBox( functionStack );
    funcCombo->setObjectName( "textRuleFuncCombo" );
    for ( int i = 0; i < TextFunctionCount; ++i ) {
      funcCombo->addItem( i18n( TextFunctions[i].displayName ) );
    }
    funcCombo->adjustSize();
    QObject::connect( funcCombo, SIGNAL( activated( int ) ),
                      receiver, SLOT( slotFunctionChanged() ) );
    return funcCombo;
  }

  //---------------------------------------------------------------------------

  QWidget * TextRuleWidgetHandler::createValueWidget( int number,
                                                      QStackedWidget *valueStack,
                                                      const QObject *receiver ) const
  {
    if ( number == 0 ) {
      RegExpLineEdit *lineEdit =
        new RegExpLineEdit( valueStack );
      lineEdit->setObjectName( "regExpLineEdit" );
      QObject::connect( lineEdit, SIGNAL( textChanged( const QString & ) ),
                        receiver, SLOT( slotValueChanged() ) );
      return lineEdit;
    }

    // blank QLabel to hide value widget for in-address-book rule
    if ( number == 1 ) {
      QLabel *label = new QLabel( valueStack );
      label->setObjectName( "textRuleValueHider" );
      label->setBuddy( valueStack );
      return label;
    }

    if ( number == 2 ) {
      QComboBox *combo =  new QComboBox( valueStack );
      combo->setObjectName( "categoryCombo" );
      QStringList categories = KabcBridge::categories();
      combo->addItems( categories );
      QObject::connect( combo, SIGNAL( activated( int ) ),
                        receiver, SLOT( slotValueChanged() ) );
      return combo;
    }

    return 0;
  }

  //---------------------------------------------------------------------------

  KMSearchRule::Function TextRuleWidgetHandler::currentFunction( const QStackedWidget *functionStack ) const
  {
    const QComboBox *funcCombo =
      dynamic_cast<QComboBox*>( QObject_child_const( functionStack,
                                                     "textRuleFuncCombo" ) );
    // FIXME (Qt >= 4.0): Use the following when QObject::child() is const.
    //  dynamic_cast<QComboBox*>( functionStack->child( "textRuleFuncCombo",
    //                                                  0, false ) );
    if ( funcCombo && funcCombo->currentIndex() >= 0) {
      return TextFunctions[funcCombo->currentIndex()].id;
    }
    else
      kDebug(5006) <<"TextRuleWidgetHandler::currentFunction:"
                       "textRuleFuncCombo not found.";
    return KMSearchRule::FuncNone;
  }

  //---------------------------------------------------------------------------

  KMSearchRule::Function TextRuleWidgetHandler::function( const QByteArray &,
                                                          const QStackedWidget *functionStack ) const
  {
    return currentFunction( functionStack );
  }

  //---------------------------------------------------------------------------

  QString TextRuleWidgetHandler::currentValue( const QStackedWidget *valueStack,
                                               KMSearchRule::Function func ) const
  {
    // here we gotta check the combobox which contains the categories
    if ( func  == KMSearchRule::FuncIsInCategory ||
         func  == KMSearchRule::FuncIsNotInCategory ) {
      const QComboBox *combo=
        dynamic_cast<QComboBox*>( QObject_child_const( valueStack,
                                                       "categoryCombo" ) );
    // FIXME (Qt >= 4.0): Use the following when QObject::child() is const.
    //  dynamic_cast<QComboBox*>( valueStack->child( "categoryCombo",
    //                                               0, false ) );
      if ( combo ) {
        return combo->currentText();
      }
      else {
        kDebug(5006) <<"TextRuleWidgetHandler::currentValue:"
                         "categoryCombo not found.";
        return QString();
      }
    }

    //in other cases of func it is a lineedit
    const RegExpLineEdit *lineEdit =
      dynamic_cast<RegExpLineEdit*>( QObject_child_const( valueStack,
                                                          "regExpLineEdit" ) );
    // FIXME (Qt >= 4.0): Use the following when QObject::child() is const.
    //  dynamic_cast<RegExpLineEdit*>( valueStack->child( "regExpLineEdit",
    //                                                    0, false ) );
    if ( lineEdit ) {
      return lineEdit->text();
      }
    else
      kDebug(5006) <<"TextRuleWidgetHandler::currentValue:"
                       "regExpLineEdit not found.";

    // or anything else, like addressbook
    return QString();
  }

  //---------------------------------------------------------------------------

  QString TextRuleWidgetHandler::value( const QByteArray &,
                                        const QStackedWidget *functionStack,
                                        const QStackedWidget *valueStack ) const
  {
    KMSearchRule::Function func = currentFunction( functionStack );
    if ( func == KMSearchRule::FuncIsInAddressbook )
      return "is in address book"; // just a non-empty dummy value
    else if ( func == KMSearchRule::FuncIsNotInAddressbook )
      return "is not in address book"; // just a non-empty dummy value
    else
      return currentValue( valueStack, func );
  }

  //---------------------------------------------------------------------------

  QString TextRuleWidgetHandler::prettyValue( const QByteArray &,
                                              const QStackedWidget *functionStack,
                                              const QStackedWidget *valueStack ) const
  {
    KMSearchRule::Function func = currentFunction( functionStack );
    if ( func == KMSearchRule::FuncIsInAddressbook )
      return i18n( "is in address book" );
    else if ( func == KMSearchRule::FuncIsNotInAddressbook )
      return i18n( "is not in address book" );
    else
      return currentValue( valueStack, func );
  }

  //---------------------------------------------------------------------------

  bool TextRuleWidgetHandler::handlesField( const QByteArray & ) const
  {
    return true; // we handle all fields (as fallback)
  }

  //---------------------------------------------------------------------------

  void TextRuleWidgetHandler::reset( QStackedWidget *functionStack,
                                     QStackedWidget *valueStack ) const
  {

    // reset the function combo box
    QComboBox *funcCombo =
            functionStack->findChild<QComboBox*>( "textRuleFuncCombo" );

    if ( funcCombo ) {
      funcCombo->blockSignals( true );
      funcCombo->setCurrentIndex( 0 );
      funcCombo->blockSignals( false );
    }

    // reset the value widget
    RegExpLineEdit *lineEdit =
            valueStack->findChild<RegExpLineEdit*>( "regExpLineEdit");
    if ( lineEdit ) {
      lineEdit->blockSignals( true );
      lineEdit->clear();
      lineEdit->blockSignals( false );
      lineEdit->showEditButton( false );
      valueStack->setCurrentWidget( lineEdit );
    }

    QComboBox *combo =
            valueStack->findChild<QComboBox*>( "categoryCombo" );

    if (combo) {
      combo->blockSignals( true );
      combo->setCurrentIndex( 0 );
      combo->blockSignals( false );
    }
  }

  //---------------------------------------------------------------------------

  bool TextRuleWidgetHandler::setRule( QStackedWidget *functionStack,
                                       QStackedWidget *valueStack,
                                       const KMSearchRule *rule ) const
  {

    if ( !rule ) {
      reset( functionStack, valueStack );
      return false;
    }

    const KMSearchRule::Function func = rule->function();
    int i = 0;
    for ( ; i < TextFunctionCount; ++i )
      if ( func == TextFunctions[i].id )
        break;
    QComboBox *funcCombo =
            functionStack->findChild<QComboBox*>( "textRuleFuncCombo" );

    if ( funcCombo ) {
      funcCombo->blockSignals( true );
      if ( i < TextFunctionCount )
        funcCombo->setCurrentIndex( i );
      else {
        kDebug(5006) <<"TextRuleWidgetHandler::setRule("
                      << rule->asString()
                      << "): unhandled function";
        funcCombo->setCurrentIndex( 0 );
      }
      funcCombo->blockSignals( false );
      functionStack->setCurrentWidget( funcCombo );
    }

    if ( func == KMSearchRule::FuncIsInAddressbook ||
         func == KMSearchRule::FuncIsNotInAddressbook ) {
      QWidget *w =
              valueStack->findChild<QWidget*>( "textRuleValueHider");
      valueStack->setCurrentWidget( w );
    }
    else if ( func == KMSearchRule::FuncIsInCategory ||
              func == KMSearchRule::FuncIsNotInCategory) {
      QComboBox *combo =
              valueStack->findChild<QComboBox*>( "categoryCombo" );

      combo->blockSignals( true );
      for ( i = 0; i < combo->count(); ++i )
        if ( rule->contents() == combo->itemText( i ) ) {
          combo->setCurrentIndex( i );
          break;
        }
      if ( i == combo->count() )
        combo->setCurrentIndex( 0 );

      combo->blockSignals( false );
      valueStack->setCurrentWidget( combo );
    }
    else {
      RegExpLineEdit *lineEdit =
              valueStack->findChild<RegExpLineEdit*>( "regExpLineEdit" );

      if ( lineEdit ) {
        lineEdit->blockSignals( true );
        lineEdit->setText( rule->contents() );
        lineEdit->blockSignals( false );
        lineEdit->showEditButton( func == KMSearchRule::FuncRegExp ||
                                  func == KMSearchRule::FuncNotRegExp );
        valueStack->setCurrentWidget( lineEdit );
      }
    }
    return true;
  }


  //---------------------------------------------------------------------------

  bool TextRuleWidgetHandler::update( const QByteArray &,
                                      QStackedWidget *functionStack,
                                      QStackedWidget *valueStack ) const
  {
    // raise the correct function widget
    functionStack->setCurrentWidget(
            functionStack->findChild<QWidget*>( "textRuleFuncCombo" ));

    // raise the correct value widget
    KMSearchRule::Function func = currentFunction( functionStack );
    if ( func == KMSearchRule::FuncIsInAddressbook ||
         func == KMSearchRule::FuncIsNotInAddressbook ) {
      valueStack->setCurrentWidget(
              valueStack->findChild<QWidget*>( "textRuleValueHider" ));
    }
    else if ( func == KMSearchRule::FuncIsInCategory ||
              func == KMSearchRule::FuncIsNotInCategory) {
      valueStack->setCurrentWidget(
              valueStack->findChild<QWidget*>( "categoryCombo" ));
    }
    else {
        RegExpLineEdit *lineEdit =
                valueStack->findChild<RegExpLineEdit*>( "regExpLineEdit" );

      if ( lineEdit ) {
        lineEdit->showEditButton( func == KMSearchRule::FuncRegExp ||
                                  func == KMSearchRule::FuncNotRegExp );
        valueStack->setCurrentWidget( lineEdit );
      }
    }
    return true;
  }

} // anonymous namespace for TextRuleWidgetHandler


//=============================================================================
//
// class MessageRuleWidgetHandler
//
//=============================================================================

namespace {
  // also see KMSearchRule::matches() and KMSearchRule::Function
  // if you change the following strings!
  static const struct {
    const KMSearchRule::Function id;
    const char *displayName;
  } MessageFunctions[] = {
    { KMSearchRule::FuncContains,        I18N_NOOP( "contains" )          },
    { KMSearchRule::FuncContainsNot,     I18N_NOOP( "does not contain" )  },
    { KMSearchRule::FuncRegExp,          I18N_NOOP( "matches regular expr." ) },
    { KMSearchRule::FuncNotRegExp,       I18N_NOOP( "does not match reg. expr." ) },
    { KMSearchRule::FuncHasAttachment,   I18N_NOOP( "has an attachment" ) },
    { KMSearchRule::FuncHasNoAttachment, I18N_NOOP( "has no attachment" ) },
  };
  static const int MessageFunctionCount =
    sizeof( MessageFunctions ) / sizeof( *MessageFunctions );

  //---------------------------------------------------------------------------

  QWidget * MessageRuleWidgetHandler::createFunctionWidget( int number,
                                                            QStackedWidget *functionStack,
                                                            const QObject *receiver ) const
  {
    if ( number != 0 )
      return 0;

    QComboBox *funcCombo = new QComboBox( functionStack );
    funcCombo->setObjectName( "messageRuleFuncCombo" );
    for ( int i = 0; i < MessageFunctionCount; ++i ) {
      funcCombo->addItem( i18n( MessageFunctions[i].displayName ) );
    }
    funcCombo->adjustSize();
    QObject::connect( funcCombo, SIGNAL( activated( int ) ),
                      receiver, SLOT( slotFunctionChanged() ) );
    return funcCombo;
  }

  //---------------------------------------------------------------------------

  QWidget * MessageRuleWidgetHandler::createValueWidget( int number,
                                                         QStackedWidget *valueStack,
                                                         const QObject *receiver ) const
  {
    if ( number == 0 ) {
      RegExpLineEdit *lineEdit =
        new RegExpLineEdit( valueStack );
      lineEdit->setObjectName( "regExpLineEdit" );
      QObject::connect( lineEdit, SIGNAL( textChanged( const QString & ) ),
                        receiver, SLOT( slotValueChanged() ) );
      return lineEdit;
    }

    // blank QLabel to hide value widget for has-attachment rule
    if ( number == 1 ) {
      QLabel *label = new QLabel( valueStack );
      label->setObjectName( "textRuleValueHider" );
      label->setBuddy( valueStack );
      return label;
    }

    return 0;
  }

  //---------------------------------------------------------------------------

  KMSearchRule::Function MessageRuleWidgetHandler::currentFunction( const QStackedWidget *functionStack ) const
  {
    const QComboBox *funcCombo =
      dynamic_cast<QComboBox*>( QObject_child_const( functionStack,
                                                     "messageRuleFuncCombo" ) );
    // FIXME (Qt >= 4.0): Use the following when QObject::child() is const.
    //  dynamic_cast<QComboBox*>( functionStack->child( "messageRuleFuncCombo",
    //                                                  0, false ) );
    if ( funcCombo && funcCombo->currentIndex() >= 0) {
      return MessageFunctions[funcCombo->currentIndex()].id;
    }
    else
      kDebug(5006) <<"MessageRuleWidgetHandler::currentFunction:"
                       "messageRuleFuncCombo not found.";
    return KMSearchRule::FuncNone;
  }

  //---------------------------------------------------------------------------

  KMSearchRule::Function MessageRuleWidgetHandler::function( const QByteArray & field,
                                                             const QStackedWidget *functionStack ) const
  {
    if ( !handlesField( field ) )
      return KMSearchRule::FuncNone;

    return currentFunction( functionStack );
  }

  //---------------------------------------------------------------------------

  QString MessageRuleWidgetHandler::currentValue( const QStackedWidget *valueStack,
                                                  KMSearchRule::Function ) const
  {
    const RegExpLineEdit *lineEdit =
      dynamic_cast<RegExpLineEdit*>( QObject_child_const( valueStack,
                                                          "regExpLineEdit" ) );
    // FIXME (Qt >= 4.0): Use the following when QObject::child() is const.
    //  dynamic_cast<RegExpLineEdit*>( valueStack->child( "regExpLineEdit",
    //                                                    0, false ) );
    if ( lineEdit ) {
      return lineEdit->text();
    }
    else
      kDebug(5006) <<"MessageRuleWidgetHandler::currentValue:"
                       "regExpLineEdit not found.";

    return QString();
  }

  //---------------------------------------------------------------------------

  QString MessageRuleWidgetHandler::value( const QByteArray & field,
                                           const QStackedWidget *functionStack,
                                           const QStackedWidget *valueStack ) const
  {
    if ( !handlesField( field ) )
      return QString();

    KMSearchRule::Function func = currentFunction( functionStack );
    if ( func == KMSearchRule::FuncHasAttachment )
      return "has an attachment"; // just a non-empty dummy value
    else if ( func == KMSearchRule::FuncHasNoAttachment )
      return "has no attachment"; // just a non-empty dummy value
    else
      return currentValue( valueStack, func );
  }

  //---------------------------------------------------------------------------

  QString MessageRuleWidgetHandler::prettyValue( const QByteArray & field,
                                                 const QStackedWidget *functionStack,
                                                 const QStackedWidget *valueStack ) const
  {
    if ( !handlesField( field ) )
      return QString();

    KMSearchRule::Function func = currentFunction( functionStack );
    if ( func == KMSearchRule::FuncHasAttachment )
      return i18n( "has an attachment" );
    else if ( func == KMSearchRule::FuncHasNoAttachment )
      return i18n( "has no attachment" );
    else
      return currentValue( valueStack, func );
  }

  //---------------------------------------------------------------------------

  bool MessageRuleWidgetHandler::handlesField( const QByteArray & field ) const
  {
    return ( field == "<message>" );
  }

  //---------------------------------------------------------------------------

  void MessageRuleWidgetHandler::reset( QStackedWidget *functionStack,
                                        QStackedWidget *valueStack ) const
  {
    // reset the function combo box
    QComboBox *funcCombo =
            functionStack->findChild<QComboBox*>( "messageRuleFuncCombo" );

    if ( funcCombo ) {
      funcCombo->blockSignals( true );
      funcCombo->setCurrentIndex( 0 );
      funcCombo->blockSignals( false );
    }

    // reset the value widget
    RegExpLineEdit *lineEdit =
            valueStack->findChild<RegExpLineEdit*>( "regExpLineEdit" );

    if ( lineEdit ) {
      lineEdit->blockSignals( true );
      lineEdit->clear();
      lineEdit->blockSignals( false );
      lineEdit->showEditButton( false );
      valueStack->setCurrentWidget( lineEdit );
    }
  }

  //---------------------------------------------------------------------------

  bool MessageRuleWidgetHandler::setRule( QStackedWidget *functionStack,
                                          QStackedWidget *valueStack,
                                          const KMSearchRule *rule ) const
  {
    if ( !rule || !handlesField( rule->field() ) ) {
      reset( functionStack, valueStack );
      return false;
    }

    const KMSearchRule::Function func = rule->function();
    int i = 0;
    for ( ; i < MessageFunctionCount; ++i )
      if ( func == MessageFunctions[i].id )
        break;
    QComboBox *funcCombo =
            functionStack->findChild<QComboBox*>( "messageRuleFuncCombo" );

    if ( funcCombo ) {
      funcCombo->blockSignals( true );
      if ( i < MessageFunctionCount )
        funcCombo->setCurrentIndex( i );
      else {
        kDebug(5006) <<"MessageRuleWidgetHandler::setRule("
                      << rule->asString()
                      << "): unhandled function";
        funcCombo->setCurrentIndex( 0 );
      }
      funcCombo->blockSignals( false );
      functionStack->setCurrentWidget( funcCombo );
    }

    if ( func == KMSearchRule::FuncHasAttachment  ||
         func == KMSearchRule::FuncHasNoAttachment ) {
      QWidget *w =
              valueStack->findChild<QWidget*>( "textRuleValueHider" );

      valueStack->setCurrentWidget( w );
    }
    else {
      RegExpLineEdit *lineEdit =
              valueStack->findChild<RegExpLineEdit*>( "regExpLineEdit" );

      if ( lineEdit ) {
        lineEdit->blockSignals( true );
        lineEdit->setText( rule->contents() );
        lineEdit->blockSignals( false );
        lineEdit->showEditButton( func == KMSearchRule::FuncRegExp ||
                                  func == KMSearchRule::FuncNotRegExp );
        valueStack->setCurrentWidget( lineEdit );
      }
    }
    return true;
  }


  //---------------------------------------------------------------------------

  bool MessageRuleWidgetHandler::update( const QByteArray & field,
                                      QStackedWidget *functionStack,
                                      QStackedWidget *valueStack ) const
  {
    if ( !handlesField( field ) )
      return false;
    // raise the correct function widget
    functionStack->setCurrentWidget(
            functionStack->findChild<QWidget*>( "messageRuleFuncCombo" ));

    // raise the correct value widget
    KMSearchRule::Function func = currentFunction( functionStack );
    if ( func == KMSearchRule::FuncHasAttachment  ||
         func == KMSearchRule::FuncHasNoAttachment ) {
      QWidget *w = valueStack->findChild<QWidget*>( "textRuleValueHider" );

      valueStack->setCurrentWidget( w );
    }
    else {
      RegExpLineEdit *lineEdit =
              valueStack->findChild<RegExpLineEdit*>( "regExpLineEdit" );

      if ( lineEdit ) {
        lineEdit->showEditButton( func == KMSearchRule::FuncRegExp ||
                                  func == KMSearchRule::FuncNotRegExp );
        valueStack->setCurrentWidget( lineEdit );
      }
    }
    return true;
  }

} // anonymous namespace for MessageRuleWidgetHandler


//=============================================================================
//
// class StatusRuleWidgetHandler
//
//=============================================================================

namespace {
  static const struct {
    const KMSearchRule::Function id;
    const char *displayName;
  } StatusFunctions[] = {
    { KMSearchRule::FuncContains,    I18N_NOOP( "is" )    },
    { KMSearchRule::FuncContainsNot, I18N_NOOP( "is not" ) }
  };
  static const int StatusFunctionCount =
    sizeof( StatusFunctions ) / sizeof( *StatusFunctions );

  //---------------------------------------------------------------------------

  QWidget * StatusRuleWidgetHandler::createFunctionWidget( int number,
                                                           QStackedWidget *functionStack,
                                                           const QObject *receiver ) const
  {
    if ( number != 0 )
      return 0;

    QComboBox *funcCombo = new QComboBox( functionStack );
    funcCombo->setObjectName( "statusRuleFuncCombo" );
    for ( int i = 0; i < StatusFunctionCount; ++i ) {
      funcCombo->addItem( i18n( StatusFunctions[i].displayName ) );
    }
    funcCombo->adjustSize();
    QObject::connect( funcCombo, SIGNAL( activated( int ) ),
                      receiver, SLOT( slotFunctionChanged() ) );
    return funcCombo;
  }

  //---------------------------------------------------------------------------

  QWidget * StatusRuleWidgetHandler::createValueWidget( int number,
                                                        QStackedWidget *valueStack,
                                                        const QObject *receiver ) const
  {
    if ( number != 0 )
      return 0;

    QComboBox *statusCombo = new QComboBox( valueStack );
    statusCombo->setObjectName( "statusRuleValueCombo" );
    for ( int i = 0; i < KMail::StatusValueCountWithoutHidden; ++i ) {
      statusCombo->addItem( UserIcon( KMail::StatusValues[ i ].icon ), i18n( KMail::StatusValues[ i ].text ) );
    }
    statusCombo->adjustSize();
    QObject::connect( statusCombo, SIGNAL( activated( int ) ),
                      receiver, SLOT( slotValueChanged() ) );
    return statusCombo;
  }

  //---------------------------------------------------------------------------

  KMSearchRule::Function StatusRuleWidgetHandler::currentFunction( const QStackedWidget *functionStack ) const
  {
    const QComboBox *funcCombo =
      dynamic_cast<QComboBox*>( QObject_child_const( functionStack,
                                                     "statusRuleFuncCombo" ) );
    // FIXME (Qt >= 4.0): Use the following when QObject::child() is const.
    //  dynamic_cast<QComboBox*>( functionStack->child( "statusRuleFuncCombo",
    //                                                  0, false ) );
    if ( funcCombo && funcCombo->currentIndex() >= 0) {
      return StatusFunctions[funcCombo->currentIndex()].id;
    }
    else
      kDebug(5006) <<"StatusRuleWidgetHandler::currentFunction:"
                       "statusRuleFuncCombo not found.";
    return KMSearchRule::FuncNone;
  }

  //---------------------------------------------------------------------------

  KMSearchRule::Function StatusRuleWidgetHandler::function( const QByteArray & field,
                                                            const QStackedWidget *functionStack ) const
  {
    if ( !handlesField( field ) )
      return KMSearchRule::FuncNone;

    return currentFunction( functionStack );
  }

  //---------------------------------------------------------------------------

  int StatusRuleWidgetHandler::currentStatusValue( const QStackedWidget *valueStack ) const
  {
    const QComboBox *statusCombo =
      dynamic_cast<QComboBox*>( QObject_child_const( valueStack,
                                                     "statusRuleValueCombo" ) );
    // FIXME (Qt >= 4.0): Use the following when QObject::child() is const.
    //  dynamic_cast<QComboBox*>( valueStack->child( "statusRuleValueCombo",
    //                                               0, false ) );
    if ( statusCombo ) {
      return statusCombo->currentIndex();
    }
    else
      kDebug(5006) <<"StatusRuleWidgetHandler::currentStatusValue:"
                       "statusRuleValueCombo not found.";
    return -1;
  }

  //---------------------------------------------------------------------------

  QString StatusRuleWidgetHandler::value( const QByteArray & field,
                                          const QStackedWidget *,
                                          const QStackedWidget *valueStack ) const
  {
    if ( !handlesField( field ) )
      return QString();

    const int status = currentStatusValue( valueStack );
    if ( status != -1 )
      return QString::fromLatin1( KMail::StatusValues[ status ].text );
    else
      return QString();
  }

  //---------------------------------------------------------------------------

  QString StatusRuleWidgetHandler::prettyValue( const QByteArray & field,
                                                const QStackedWidget *,
                                                const QStackedWidget *valueStack ) const
  {
    if ( !handlesField( field ) )
      return QString();

    const int status = currentStatusValue( valueStack );
    if ( status != -1 )
      return i18n( KMail::StatusValues[ status ].text );
    else
      return QString();
  }

  //---------------------------------------------------------------------------

  bool StatusRuleWidgetHandler::handlesField( const QByteArray & field ) const
  {
    return ( field == "<status>" );
  }

  //---------------------------------------------------------------------------

  void StatusRuleWidgetHandler::reset( QStackedWidget *functionStack,
                                       QStackedWidget *valueStack ) const
  {
    // reset the function combo box
    QComboBox *funcCombo =
            functionStack->findChild<QComboBox*>( "statusRuleFuncCombo" );

    if ( funcCombo ) {
      funcCombo->blockSignals( true );
      funcCombo->setCurrentIndex( 0 );
      funcCombo->blockSignals( false );
    }

    // reset the status value combo box
    QComboBox *statusCombo =
            valueStack->findChild<QComboBox*>( "statusRuleValueCombo" );

    if ( statusCombo ) {
      statusCombo->blockSignals( true );
      statusCombo->setCurrentIndex( 0 );
      statusCombo->blockSignals( false );
    }
  }

  //---------------------------------------------------------------------------

  bool StatusRuleWidgetHandler::setRule( QStackedWidget *functionStack,
                                         QStackedWidget *valueStack,
                                         const KMSearchRule *rule ) const
  {
    if ( !rule || !handlesField( rule->field() ) ) {
      reset( functionStack, valueStack );
      return false;
    }

    // set the function
    const KMSearchRule::Function func = rule->function();
    int funcIndex = 0;
    for ( ; funcIndex < StatusFunctionCount; ++funcIndex )
      if ( func == StatusFunctions[funcIndex].id )
        break;
    QComboBox *funcCombo =
            functionStack->findChild<QComboBox*>( "statusRuleFuncCombo" );

    if ( funcCombo ) {
      funcCombo->blockSignals( true );
      if ( funcIndex < StatusFunctionCount )
        funcCombo->setCurrentIndex( funcIndex );
      else {
        kDebug(5006) <<"StatusRuleWidgetHandler::setRule("
                      << rule->asString()
                      << "): unhandled function";
        funcCombo->setCurrentIndex( 0 );
      }
      funcCombo->blockSignals( false );
      functionStack->setCurrentWidget( funcCombo );
    }

    // set the value
    const QString value = rule->contents();
    int valueIndex = 0;
    for ( ; valueIndex < KMail::StatusValueCountWithoutHidden; ++valueIndex )
      if ( value == QString::fromLatin1(
               KMail::StatusValues[ valueIndex ].text ) )
        break;
    QComboBox *statusCombo =
            valueStack->findChild<QComboBox*>( "statusRuleValueCombo" );

    if ( statusCombo ) {
      statusCombo->blockSignals( true );
      if ( valueIndex < KMail::StatusValueCountWithoutHidden )
        statusCombo->setCurrentIndex( valueIndex );
      else {
        kDebug(5006) <<"StatusRuleWidgetHandler::setRule("
                      << rule->asString()
                      << "): unhandled value";
        statusCombo->setCurrentIndex( 0 );
      }
      statusCombo->blockSignals( false );
      valueStack->setCurrentWidget( statusCombo );
    }
    return true;
  }


  //---------------------------------------------------------------------------

  bool StatusRuleWidgetHandler::update( const QByteArray &field,
                                        QStackedWidget *functionStack,
                                        QStackedWidget *valueStack ) const
  {
    if ( !handlesField( field ) )
      return false;

    // raise the correct function widget
    functionStack->setCurrentWidget(
            functionStack->findChild<QWidget*>( "statusRuleFuncCombo" ));

    // raise the correct value widget
    valueStack->setCurrentWidget(
            valueStack->findChild<QWidget*>( "statusRuleValueCombo" ));

    return true;
  }

} // anonymous namespace for StatusRuleWidgetHandler


//=============================================================================
//
// class NumericRuleWidgetHandler
//
//=============================================================================

namespace {
  static const struct {
    const KMSearchRule::Function id;
    const char *displayName;
  } NumericFunctions[] = {
    { KMSearchRule::FuncEquals,           I18N_NOOP( "is equal to" )         },
    { KMSearchRule::FuncNotEqual,         I18N_NOOP( "is not equal to" )      },
    { KMSearchRule::FuncIsGreater,        I18N_NOOP( "is greater than" )     },
    { KMSearchRule::FuncIsLessOrEqual,    I18N_NOOP( "is less than or equal to" ) },
    { KMSearchRule::FuncIsLess,           I18N_NOOP( "is less than" )        },
    { KMSearchRule::FuncIsGreaterOrEqual, I18N_NOOP( "is greater than or equal to" ) }
  };
  static const int NumericFunctionCount =
    sizeof( NumericFunctions ) / sizeof( *NumericFunctions );

  //---------------------------------------------------------------------------

  QWidget * NumericRuleWidgetHandler::createFunctionWidget( int number,
                                                            QStackedWidget *functionStack,
                                                            const QObject *receiver ) const
  {
    if ( number != 0 )
      return 0;

    QComboBox *funcCombo = new QComboBox( functionStack );
    funcCombo->setObjectName( "numericRuleFuncCombo" );
    for ( int i = 0; i < NumericFunctionCount; ++i ) {
      funcCombo->addItem( i18n( NumericFunctions[i].displayName ) );
    }
    funcCombo->adjustSize();
    QObject::connect( funcCombo, SIGNAL( activated( int ) ),
                      receiver, SLOT( slotFunctionChanged() ) );
    return funcCombo;
  }

  //---------------------------------------------------------------------------

  QWidget * NumericRuleWidgetHandler::createValueWidget( int number,
                                                         QStackedWidget *valueStack,
                                                         const QObject *receiver ) const
  {
    if ( number != 0 )
      return 0;

    KIntNumInput *numInput = new KIntNumInput( valueStack );
    numInput->setSliderEnabled( false );
    numInput->setObjectName( "KIntNumInput" );
    QObject::connect( numInput, SIGNAL( valueChanged( int ) ),
                      receiver, SLOT( slotValueChanged() ) );
    return numInput;
  }

  //---------------------------------------------------------------------------

  KMSearchRule::Function NumericRuleWidgetHandler::currentFunction( const QStackedWidget *functionStack ) const
  {
    const QComboBox *funcCombo =
      dynamic_cast<QComboBox*>( QObject_child_const( functionStack,
                                                     "numericRuleFuncCombo" ) );
    // FIXME (Qt >= 4.0): Use the following when QObject::child() is const.
    //  dynamic_cast<QComboBox*>( functionStack->child( "numericRuleFuncCombo",
    //                                                  0, false ) );
    if ( funcCombo && funcCombo->currentIndex() >= 0 ) {
      return NumericFunctions[funcCombo->currentIndex()].id;
    }
    else
      kDebug(5006) <<"NumericRuleWidgetHandler::currentFunction:"
                       "numericRuleFuncCombo not found.";
    return KMSearchRule::FuncNone;
  }

  //---------------------------------------------------------------------------

  KMSearchRule::Function NumericRuleWidgetHandler::function( const QByteArray & field,
                                                             const QStackedWidget *functionStack ) const
  {
    if ( !handlesField( field ) )
      return KMSearchRule::FuncNone;

    return currentFunction( functionStack );
  }

  //---------------------------------------------------------------------------

  QString NumericRuleWidgetHandler::currentValue( const QStackedWidget *valueStack ) const
  {
    const KIntNumInput *numInput =
      dynamic_cast<KIntNumInput*>( QObject_child_const( valueStack,
                                                        "KIntNumInput" ) );
    // FIXME (Qt >= 4.0): Use the following when QObject::child() is const.
    //  dynamic_cast<KIntNumInput*>( valueStack->child( "KIntNumInput",
    //                                                  0, false ) );
    if ( numInput ) {
      return QString::number( numInput->value() );
    }
    else
      kDebug(5006) <<"NumericRuleWidgetHandler::currentValue:"
                       "KIntNumInput not found.";
    return QString();
  }

  //---------------------------------------------------------------------------

  QString NumericRuleWidgetHandler::value( const QByteArray & field,
                                           const QStackedWidget *,
                                           const QStackedWidget *valueStack ) const
  {
    if ( !handlesField( field ) )
      return QString();

    return currentValue( valueStack );
  }

  //---------------------------------------------------------------------------

  QString NumericRuleWidgetHandler::prettyValue( const QByteArray & field,
                                                 const QStackedWidget *,
                                                 const QStackedWidget *valueStack ) const
  {
    if ( !handlesField( field ) )
      return QString();

    return currentValue( valueStack );
  }

  //---------------------------------------------------------------------------

  bool NumericRuleWidgetHandler::handlesField( const QByteArray & field ) const
  {
    return ( field == "<size>" || field == "<age in days>" );
  }

  //---------------------------------------------------------------------------

  void NumericRuleWidgetHandler::reset( QStackedWidget *functionStack,
                                        QStackedWidget *valueStack ) const
  {
    // reset the function combo box
    QComboBox *funcCombo =
            functionStack->findChild<QComboBox*>( "numericRuleFuncCombo" );

    if ( funcCombo ) {
      funcCombo->blockSignals( true );
      funcCombo->setCurrentIndex( 0 );
      funcCombo->blockSignals( false );
    }

    // reset the value widget
    KIntNumInput *numInput =
            valueStack->findChild<KIntNumInput*>( "KIntNumInput" );

    if ( numInput ) {
      numInput->blockSignals( true );
      numInput->setValue( 0 );
      numInput->blockSignals( false );
    }
  }

  //---------------------------------------------------------------------------

  void initNumInput( KIntNumInput *numInput, const QByteArray &field )
  {
    if ( field == "<size>" ) {
      numInput->setMinimum( 0 );
      numInput->setSuffix( i18n( " bytes" ) );
      numInput->setSliderEnabled( false );
    }
    else {
      numInput->setMinimum( -10000 );
      numInput->setSuffix( i18n( " days" ) );
      numInput->setSliderEnabled( false );
    }
  }

  //---------------------------------------------------------------------------

  bool NumericRuleWidgetHandler::setRule( QStackedWidget *functionStack,
                                          QStackedWidget *valueStack,
                                          const KMSearchRule *rule ) const
  {
    if ( !rule || !handlesField( rule->field() ) ) {
      reset( functionStack, valueStack );
      return false;
    }

    // set the function
    const KMSearchRule::Function func = rule->function();
    int funcIndex = 0;
    for ( ; funcIndex < NumericFunctionCount; ++funcIndex )
      if ( func == NumericFunctions[funcIndex].id )
        break;
    QComboBox *funcCombo =
            functionStack->findChild<QComboBox*>( "numericRuleFuncCombo" );

    if ( funcCombo ) {
      funcCombo->blockSignals( true );
      if ( funcIndex < NumericFunctionCount )
        funcCombo->setCurrentIndex( funcIndex );
      else {
        kDebug(5006) <<"NumericRuleWidgetHandler::setRule("
                      << rule->asString()
                      << "): unhandled function";
        funcCombo->setCurrentIndex( 0 );
      }
      funcCombo->blockSignals( false );
      functionStack->setCurrentWidget( funcCombo );
    }

    // set the value
    bool ok;
    int value = rule->contents().toInt( &ok );
    if ( !ok )
      value = 0;
    KIntNumInput *numInput =
            valueStack->findChild<KIntNumInput*>( "KIntNumInput" );

    if ( numInput ) {
      initNumInput( numInput, rule->field() );
      numInput->blockSignals( true );
      numInput->setValue( value );
      numInput->blockSignals( false );
      valueStack->setCurrentWidget( numInput );
    }
    return true;
  }


  //---------------------------------------------------------------------------

  bool NumericRuleWidgetHandler::update( const QByteArray &field,
                                         QStackedWidget *functionStack,
                                         QStackedWidget *valueStack ) const
  {
    if ( !handlesField( field ) )
      return false;

    // raise the correct function widget
    functionStack->setCurrentWidget(
            functionStack->findChild<QWidget*>( "numericRuleFuncCombo" ));

    // raise the correct value widget
    KIntNumInput *numInput =
            valueStack->findChild<KIntNumInput*>( "KIntNumInput" );

    if ( numInput ) {
      initNumInput( numInput, field );
      valueStack->setCurrentWidget( numInput );
    }
    return true;
  }

} // anonymous namespace for NumericRuleWidgetHandler


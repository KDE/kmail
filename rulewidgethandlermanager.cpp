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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "rulewidgethandlermanager.h"

#include "interfaces/rulewidgethandler.h"
#include "stl_util.h"

#include <kdebug.h>

#include <qwidgetstack.h>
#include <qstring.h>
#include <qcstring.h>
#include <qobject.h>
#include <qobjectlist.h>

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
                                    QWidgetStack *functionStack,
                                    const QObject *receiver ) const;
    QWidget * createValueWidget( int number,
                                 QWidgetStack *valueStack,
                                 const QObject *receiver ) const;
    KMSearchRule::Function function( const QCString & field,
                                     const QWidgetStack *functionStack ) const;
    QString value( const QCString & field,
                   const QWidgetStack *functionStack,
                   const QWidgetStack *valueStack ) const;
    QString prettyValue( const QCString & field,
                         const QWidgetStack *functionStack,
                         const QWidgetStack *valueStack ) const;
    bool handlesField( const QCString & field ) const;
    void reset( QWidgetStack *functionStack,
                QWidgetStack *valueStack ) const;
    bool setRule( QWidgetStack *functionStack,
                  QWidgetStack *valueStack,
                  const KMSearchRule *rule ) const;
    bool update( const QCString & field,
                 QWidgetStack *functionStack,
                 QWidgetStack *valueStack ) const;

 private:
    KMSearchRule::Function currentFunction( const QWidgetStack *functionStack ) const;
    QString currentValue( const QWidgetStack *valueStack,
                          KMSearchRule::Function func ) const;
  };

  class MessageRuleWidgetHandler : public KMail::RuleWidgetHandler {
  public:
    MessageRuleWidgetHandler() : KMail::RuleWidgetHandler() {}
    ~MessageRuleWidgetHandler() {}

    QWidget * createFunctionWidget( int number,
                                    QWidgetStack *functionStack,
                                    const QObject *receiver ) const;
    QWidget * createValueWidget( int number,
                                 QWidgetStack *valueStack,
                                 const QObject *receiver ) const;
    KMSearchRule::Function function( const QCString & field,
                                     const QWidgetStack *functionStack ) const;
    QString value( const QCString & field,
                   const QWidgetStack *functionStack,
                   const QWidgetStack *valueStack ) const;
    QString prettyValue( const QCString & field,
                         const QWidgetStack *functionStack,
                         const QWidgetStack *valueStack ) const;
    bool handlesField( const QCString & field ) const;
    void reset( QWidgetStack *functionStack,
                QWidgetStack *valueStack ) const;
    bool setRule( QWidgetStack *functionStack,
                  QWidgetStack *valueStack,
                  const KMSearchRule *rule ) const;
    bool update( const QCString & field,
                 QWidgetStack *functionStack,
                 QWidgetStack *valueStack ) const;

 private:
    KMSearchRule::Function currentFunction( const QWidgetStack *functionStack ) const;
    QString currentValue( const QWidgetStack *valueStack,
                          KMSearchRule::Function func ) const;
  };


  class StatusRuleWidgetHandler : public KMail::RuleWidgetHandler {
  public:
    StatusRuleWidgetHandler() : KMail::RuleWidgetHandler() {}
    ~StatusRuleWidgetHandler() {}

    QWidget * createFunctionWidget( int number,
                                    QWidgetStack *functionStack,
                                    const QObject *receiver ) const;
    QWidget * createValueWidget( int number,
                                 QWidgetStack *valueStack,
                                 const QObject *receiver ) const;
    KMSearchRule::Function function( const QCString & field,
                                     const QWidgetStack *functionStack ) const;
    QString value( const QCString & field,
                   const QWidgetStack *functionStack,
                   const QWidgetStack *valueStack ) const;
    QString prettyValue( const QCString & field,
                         const QWidgetStack *functionStack,
                         const QWidgetStack *valueStack ) const;
    bool handlesField( const QCString & field ) const;
    void reset( QWidgetStack *functionStack,
                QWidgetStack *valueStack ) const;
    bool setRule( QWidgetStack *functionStack,
                  QWidgetStack *valueStack,
                  const KMSearchRule *rule ) const;
    bool update( const QCString & field,
                 QWidgetStack *functionStack,
                 QWidgetStack *valueStack ) const;

  private:
    KMSearchRule::Function currentFunction( const QWidgetStack *functionStack ) const;
    int currentStatusValue( const QWidgetStack *valueStack ) const;
  };

  class NumericRuleWidgetHandler : public KMail::RuleWidgetHandler {
  public:
    NumericRuleWidgetHandler() : KMail::RuleWidgetHandler() {}
    ~NumericRuleWidgetHandler() {}

    QWidget * createFunctionWidget( int number,
                                    QWidgetStack *functionStack,
                                    const QObject *receiver ) const;
    QWidget * createValueWidget( int number,
                                 QWidgetStack *valueStack,
                                 const QObject *receiver ) const;
    KMSearchRule::Function function( const QCString & field,
                                     const QWidgetStack *functionStack ) const;
    QString value( const QCString & field,
                   const QWidgetStack *functionStack,
                   const QWidgetStack *valueStack ) const;
    QString prettyValue( const QCString & field,
                         const QWidgetStack *functionStack,
                         const QWidgetStack *valueStack ) const;
    bool handlesField( const QCString & field ) const;
    void reset( QWidgetStack *functionStack,
                QWidgetStack *valueStack ) const;
    bool setRule( QWidgetStack *functionStack,
                  QWidgetStack *valueStack,
                  const KMSearchRule *rule ) const;
    bool update( const QCString & field,
                 QWidgetStack *functionStack,
                 QWidgetStack *valueStack ) const;

  private:
    KMSearchRule::Function currentFunction( const QWidgetStack *functionStack ) const;
    QString currentValue( const QWidgetStack *valueStack ) const;
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
  for_each( mHandlers.begin(), mHandlers.end(), DeleteAndSetToZero<RuleWidgetHandler> );
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
  int childCount( const QObject *parent, const char *objName )
  {
    QObjectList *list = parent->queryList( 0, objName, false, false );
    if ( !list )
      return 0;
    const int count = list->count();
    delete list; list = 0;
    return count;
  }
}

void KMail::RuleWidgetHandlerManager::createWidgets( QWidgetStack *functionStack,
                                                     QWidgetStack *valueStack,
                                                     const QObject *receiver ) const
{
  for ( const_iterator it = mHandlers.begin(); it != mHandlers.end(); ++it ) {
    QWidget *w = 0;
    for ( int i = 0;
          ( w = (*it)->createFunctionWidget( i, functionStack, receiver ) );
          ++i ) {
      if ( childCount( functionStack, w->name() ) < 2 ) {
        // there wasn't already a widget with this name, so add this widget
        functionStack->addWidget( w );
      }
      else {
        // there was already a widget with this name, so discard this widget
        kdDebug(5006) << "RuleWidgetHandlerManager::createWidgets: "
                      << w->name() << " already exists in functionStack"
                      << endl;
        delete w; w = 0;
      }
    }
    for ( int i = 0;
          ( w = (*it)->createValueWidget( i, valueStack, receiver ) );
          ++i ) {
      if ( childCount( valueStack, w->name() ) < 2 ) {
        // there wasn't already a widget with this name, so add this widget
        valueStack->addWidget( w );
      }
      else {
        // there was already a widget with this name, so discard this widget
        kdDebug(5006) << "RuleWidgetHandlerManager::createWidgets: "
                      << w->name() << " already exists in valueStack"
                      << endl;
        delete w; w = 0;
      }
    }
  }
}

KMSearchRule::Function KMail::RuleWidgetHandlerManager::function( const QCString& field,
                                                                  const QWidgetStack *functionStack ) const
{
  for ( const_iterator it = mHandlers.begin(); it != mHandlers.end(); ++it ) {
    const KMSearchRule::Function func = (*it)->function( field,
                                                         functionStack );
    if ( func != KMSearchRule::FuncNone )
      return func;
  }
  return KMSearchRule::FuncNone;
}

QString KMail::RuleWidgetHandlerManager::value( const QCString& field,
                                                const QWidgetStack *functionStack,
                                                const QWidgetStack *valueStack ) const
{
  for ( const_iterator it = mHandlers.begin(); it != mHandlers.end(); ++it ) {
    const QString val = (*it)->value( field, functionStack, valueStack );
    if ( !val.isEmpty() )
      return val;
  }
  return QString::null;
}

QString KMail::RuleWidgetHandlerManager::prettyValue( const QCString& field,
                                                      const QWidgetStack *functionStack,
                                                      const QWidgetStack *valueStack ) const
{
  for ( const_iterator it = mHandlers.begin(); it != mHandlers.end(); ++it ) {
    const QString val = (*it)->prettyValue( field, functionStack, valueStack );
    if ( !val.isEmpty() )
      return val;
  }
  return QString::null;
}

void KMail::RuleWidgetHandlerManager::reset( QWidgetStack *functionStack,
                                             QWidgetStack *valueStack ) const
{
  for ( const_iterator it = mHandlers.begin(); it != mHandlers.end(); ++it ) {
    (*it)->reset( functionStack, valueStack );
  }
  update( "", functionStack, valueStack );
}

void KMail::RuleWidgetHandlerManager::setRule( QWidgetStack *functionStack,
                                               QWidgetStack *valueStack,
                                               const KMSearchRule *rule ) const
{
  assert( rule );
  reset( functionStack, valueStack );
  for ( const_iterator it = mHandlers.begin(); it != mHandlers.end(); ++it ) {
    if ( (*it)->setRule( functionStack, valueStack, rule ) )
      return;
  }
}

void KMail::RuleWidgetHandlerManager::update( const QCString &field,
                                              QWidgetStack *functionStack,
                                              QWidgetStack *valueStack ) const
{
  //kdDebug(5006) << "RuleWidgetHandlerManager::update( \"" << field
  //              << "\", ... )" << endl;
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
    const QObjectList *list = parent->children();
    if ( !list )
      return 0;

    QObjectListIterator it( *list );
    QObject *obj;
    while ( ( obj = it.current() ) ) {
      ++it;
      if ( !objName || qstrcmp( objName, obj->name() ) == 0 )
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

#include <qcombobox.h>
#include <qlabel.h>

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
                                                         QWidgetStack *functionStack,
                                                         const QObject *receiver ) const
  {
    if ( number != 0 )
      return 0;

    QComboBox *funcCombo = new QComboBox( functionStack, "textRuleFuncCombo" );
    for ( int i = 0; i < TextFunctionCount; ++i ) {
      funcCombo->insertItem( i18n( TextFunctions[i].displayName ) );
    }
    funcCombo->adjustSize();
    QObject::connect( funcCombo, SIGNAL( activated( int ) ),
                      receiver, SLOT( slotFunctionChanged() ) );
    return funcCombo;
  }

  //---------------------------------------------------------------------------

  QWidget * TextRuleWidgetHandler::createValueWidget( int number,
                                                      QWidgetStack *valueStack,
                                                      const QObject *receiver ) const
  {
    if ( number == 0 ) {
      RegExpLineEdit *lineEdit =
        new RegExpLineEdit( valueStack, "regExpLineEdit" );
      QObject::connect( lineEdit, SIGNAL( textChanged( const QString & ) ),
                        receiver, SLOT( slotValueChanged() ) );
      return lineEdit;
    }

    // blank QLabel to hide value widget for in-address-book rule
    if ( number == 1 ) {
      return new QLabel( valueStack, "textRuleValueHider" );
    }

    if ( number == 2 ) {
      QComboBox *combo =  new QComboBox( valueStack, "categoryCombo" );
      QStringList categories = KabcBridge::categories();
      combo->insertStringList( categories );
      QObject::connect( combo, SIGNAL( activated( int ) ),
                        receiver, SLOT( slotValueChanged() ) );
      return combo;
    }

    return 0;
  }

  //---------------------------------------------------------------------------

  KMSearchRule::Function TextRuleWidgetHandler::currentFunction( const QWidgetStack *functionStack ) const
  {
    const QComboBox *funcCombo =
      dynamic_cast<QComboBox*>( QObject_child_const( functionStack,
                                                     "textRuleFuncCombo" ) );
    // FIXME (Qt >= 4.0): Use the following when QObject::child() is const.
    //  dynamic_cast<QComboBox*>( functionStack->child( "textRuleFuncCombo",
    //                                                  0, false ) );
    if ( funcCombo ) {
      return TextFunctions[funcCombo->currentItem()].id;
    }
    else
      kdDebug(5006) << "TextRuleWidgetHandler::currentFunction: "
                       "textRuleFuncCombo not found." << endl;
    return KMSearchRule::FuncNone;
  }

  //---------------------------------------------------------------------------

  KMSearchRule::Function TextRuleWidgetHandler::function( const QCString &,
                                                          const QWidgetStack *functionStack ) const
  {
    return currentFunction( functionStack );
  }

  //---------------------------------------------------------------------------

  QString TextRuleWidgetHandler::currentValue( const QWidgetStack *valueStack,
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
        kdDebug(5006) << "TextRuleWidgetHandler::currentValue: "
                         "categoryCombo not found." << endl;
        return QString::null;
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
      kdDebug(5006) << "TextRuleWidgetHandler::currentValue: "
                       "regExpLineEdit not found." << endl;

    // or anything else, like addressbook
    return QString::null;
  }

  //---------------------------------------------------------------------------

  QString TextRuleWidgetHandler::value( const QCString &,
                                        const QWidgetStack *functionStack,
                                        const QWidgetStack *valueStack ) const
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

  QString TextRuleWidgetHandler::prettyValue( const QCString &,
                                              const QWidgetStack *functionStack,
                                              const QWidgetStack *valueStack ) const
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

  bool TextRuleWidgetHandler::handlesField( const QCString & ) const
  {
    return true; // we handle all fields (as fallback)
  }

  //---------------------------------------------------------------------------

  void TextRuleWidgetHandler::reset( QWidgetStack *functionStack,
                                     QWidgetStack *valueStack ) const
  {
    // reset the function combo box
    QComboBox *funcCombo =
      dynamic_cast<QComboBox*>( functionStack->child( "textRuleFuncCombo",
                                                      0, false ) );
    if ( funcCombo ) {
      funcCombo->blockSignals( true );
      funcCombo->setCurrentItem( 0 );
      funcCombo->blockSignals( false );
    }

    // reset the value widget
    RegExpLineEdit *lineEdit =
      dynamic_cast<RegExpLineEdit*>( valueStack->child( "regExpLineEdit",
                                                        0, false ) );
    if ( lineEdit ) {
      lineEdit->blockSignals( true );
      lineEdit->clear();
      lineEdit->blockSignals( false );
      lineEdit->showEditButton( false );
      valueStack->raiseWidget( lineEdit );
    }

    QComboBox *combo =
      dynamic_cast<QComboBox*>( valueStack->child( "categoryCombo",
                                                   0, false ) );
    if (combo) {
      combo->blockSignals( true );
      combo->setCurrentItem( 0 );
      combo->blockSignals( false );
    }
  }

  //---------------------------------------------------------------------------

  bool TextRuleWidgetHandler::setRule( QWidgetStack *functionStack,
                                       QWidgetStack *valueStack,
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
      dynamic_cast<QComboBox*>( functionStack->child( "textRuleFuncCombo",
                                                      0, false ) );
    if ( funcCombo ) {
      funcCombo->blockSignals( true );
      if ( i < TextFunctionCount )
        funcCombo->setCurrentItem( i );
      else {
        kdDebug(5006) << "TextRuleWidgetHandler::setRule( "
                      << rule->asString()
                      << " ): unhandled function" << endl;
        funcCombo->setCurrentItem( 0 );
      }
      funcCombo->blockSignals( false );
      functionStack->raiseWidget( funcCombo );
    }

    if ( func == KMSearchRule::FuncIsInAddressbook ||
         func == KMSearchRule::FuncIsNotInAddressbook ) {
      QWidget *w =
        static_cast<QWidget*>( valueStack->child( "textRuleValueHider",
                                                  0, false ) );
      valueStack->raiseWidget( w );
    }
    else if ( func == KMSearchRule::FuncIsInCategory ||
              func == KMSearchRule::FuncIsNotInCategory) {
      QComboBox *combo =
        static_cast<QComboBox*>( valueStack->child( "categoryCombo",
                                                    0, false ) );
      combo->blockSignals( true );
      for ( i = 0; i < combo->count(); ++i )
        if ( rule->contents() == combo->text( i ) ) {
          combo->setCurrentItem( i );
          break;
        }
      if ( i == combo->count() )
        combo->setCurrentItem( 0 );

      combo->blockSignals( false );
      valueStack->raiseWidget( combo );
    }
    else {
      RegExpLineEdit *lineEdit =
        dynamic_cast<RegExpLineEdit*>( valueStack->child( "regExpLineEdit",
                                                          0, false ) );
      if ( lineEdit ) {
        lineEdit->blockSignals( true );
        lineEdit->setText( rule->contents() );
        lineEdit->blockSignals( false );
        lineEdit->showEditButton( func == KMSearchRule::FuncRegExp ||
                                  func == KMSearchRule::FuncNotRegExp );
        valueStack->raiseWidget( lineEdit );
      }
    }
    return true;
  }


  //---------------------------------------------------------------------------

  bool TextRuleWidgetHandler::update( const QCString &,
                                      QWidgetStack *functionStack,
                                      QWidgetStack *valueStack ) const
  {
    // raise the correct function widget
    functionStack->raiseWidget(
      static_cast<QWidget*>( functionStack->child( "textRuleFuncCombo",
                                                   0, false ) ) );

    // raise the correct value widget
    KMSearchRule::Function func = currentFunction( functionStack );
    if ( func == KMSearchRule::FuncIsInAddressbook ||
         func == KMSearchRule::FuncIsNotInAddressbook ) {
      valueStack->raiseWidget(
        static_cast<QWidget*>( valueStack->child( "textRuleValueHider",
                                                  0, false ) ) );
    }
    else if ( func == KMSearchRule::FuncIsInCategory ||
              func == KMSearchRule::FuncIsNotInCategory) {
      valueStack->raiseWidget(
        static_cast<QWidget*>( valueStack->child( "categoryCombo",
                                                  0, false ) ) );
    }
    else {
      RegExpLineEdit *lineEdit =
        dynamic_cast<RegExpLineEdit*>( valueStack->child( "regExpLineEdit",
                                                          0, false ) );
      if ( lineEdit ) {
        lineEdit->showEditButton( func == KMSearchRule::FuncRegExp ||
                                  func == KMSearchRule::FuncNotRegExp );
        valueStack->raiseWidget( lineEdit );
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
                                                            QWidgetStack *functionStack,
                                                            const QObject *receiver ) const
  {
    if ( number != 0 )
      return 0;

    QComboBox *funcCombo = new QComboBox( functionStack, "messageRuleFuncCombo" );
    for ( int i = 0; i < MessageFunctionCount; ++i ) {
      funcCombo->insertItem( i18n( MessageFunctions[i].displayName ) );
    }
    funcCombo->adjustSize();
    QObject::connect( funcCombo, SIGNAL( activated( int ) ),
                      receiver, SLOT( slotFunctionChanged() ) );
    return funcCombo;
  }

  //---------------------------------------------------------------------------

  QWidget * MessageRuleWidgetHandler::createValueWidget( int number,
                                                         QWidgetStack *valueStack,
                                                         const QObject *receiver ) const
  {
    if ( number == 0 ) {
      RegExpLineEdit *lineEdit =
        new RegExpLineEdit( valueStack, "regExpLineEdit" );
      QObject::connect( lineEdit, SIGNAL( textChanged( const QString & ) ),
                        receiver, SLOT( slotValueChanged() ) );
      return lineEdit;
    }

    // blank QLabel to hide value widget for has-attachment rule
    if ( number == 1 ) {
      return new QLabel( valueStack, "textRuleValueHider" );
    }

    return 0;
  }

  //---------------------------------------------------------------------------

  KMSearchRule::Function MessageRuleWidgetHandler::currentFunction( const QWidgetStack *functionStack ) const
  {
    const QComboBox *funcCombo =
      dynamic_cast<QComboBox*>( QObject_child_const( functionStack,
                                                     "messageRuleFuncCombo" ) );
    // FIXME (Qt >= 4.0): Use the following when QObject::child() is const.
    //  dynamic_cast<QComboBox*>( functionStack->child( "messageRuleFuncCombo",
    //                                                  0, false ) );
    if ( funcCombo ) {
      return MessageFunctions[funcCombo->currentItem()].id;
    }
    else
      kdDebug(5006) << "MessageRuleWidgetHandler::currentFunction: "
                       "messageRuleFuncCombo not found." << endl;
    return KMSearchRule::FuncNone;
  }

  //---------------------------------------------------------------------------

  KMSearchRule::Function MessageRuleWidgetHandler::function( const QCString & field,
                                                             const QWidgetStack *functionStack ) const
  {
    if ( !handlesField( field ) )
      return KMSearchRule::FuncNone;

    return currentFunction( functionStack );
  }

  //---------------------------------------------------------------------------

  QString MessageRuleWidgetHandler::currentValue( const QWidgetStack *valueStack,
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
      kdDebug(5006) << "MessageRuleWidgetHandler::currentValue: "
                       "regExpLineEdit not found." << endl;

    return QString::null;
  }

  //---------------------------------------------------------------------------

  QString MessageRuleWidgetHandler::value( const QCString & field,
                                           const QWidgetStack *functionStack,
                                           const QWidgetStack *valueStack ) const
  {
    if ( !handlesField( field ) )
      return QString::null;

    KMSearchRule::Function func = currentFunction( functionStack );
    if ( func == KMSearchRule::FuncHasAttachment )
      return "has an attachment"; // just a non-empty dummy value
    else if ( func == KMSearchRule::FuncHasNoAttachment )
      return "has no attachment"; // just a non-empty dummy value
    else
      return currentValue( valueStack, func );
  }

  //---------------------------------------------------------------------------

  QString MessageRuleWidgetHandler::prettyValue( const QCString & field,
                                                 const QWidgetStack *functionStack,
                                                 const QWidgetStack *valueStack ) const
  {
    if ( !handlesField( field ) )
      return QString::null;

    KMSearchRule::Function func = currentFunction( functionStack );
    if ( func == KMSearchRule::FuncHasAttachment )
      return i18n( "has an attachment" );
    else if ( func == KMSearchRule::FuncHasNoAttachment )
      return i18n( "has no attachment" );
    else
      return currentValue( valueStack, func );
  }

  //---------------------------------------------------------------------------

  bool MessageRuleWidgetHandler::handlesField( const QCString & field ) const
  {
    return ( field == "<message>" );
  }

  //---------------------------------------------------------------------------

  void MessageRuleWidgetHandler::reset( QWidgetStack *functionStack,
                                        QWidgetStack *valueStack ) const
  {
    // reset the function combo box
    QComboBox *funcCombo =
      dynamic_cast<QComboBox*>( functionStack->child( "messageRuleFuncCombo",
                                                      0, false ) );
    if ( funcCombo ) {
      funcCombo->blockSignals( true );
      funcCombo->setCurrentItem( 0 );
      funcCombo->blockSignals( false );
    }

    // reset the value widget
    RegExpLineEdit *lineEdit =
      dynamic_cast<RegExpLineEdit*>( valueStack->child( "regExpLineEdit",
                                                        0, false ) );
    if ( lineEdit ) {
      lineEdit->blockSignals( true );
      lineEdit->clear();
      lineEdit->blockSignals( false );
      lineEdit->showEditButton( false );
      valueStack->raiseWidget( lineEdit );
    }
  }

  //---------------------------------------------------------------------------

  bool MessageRuleWidgetHandler::setRule( QWidgetStack *functionStack,
                                          QWidgetStack *valueStack,
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
      dynamic_cast<QComboBox*>( functionStack->child( "messageRuleFuncCombo",
                                                      0, false ) );
    if ( funcCombo ) {
      funcCombo->blockSignals( true );
      if ( i < MessageFunctionCount )
        funcCombo->setCurrentItem( i );
      else {
        kdDebug(5006) << "MessageRuleWidgetHandler::setRule( "
                      << rule->asString()
                      << " ): unhandled function" << endl;
        funcCombo->setCurrentItem( 0 );
      }
      funcCombo->blockSignals( false );
      functionStack->raiseWidget( funcCombo );
    }

    if ( func == KMSearchRule::FuncHasAttachment  ||
         func == KMSearchRule::FuncHasNoAttachment ) {
      QWidget *w =
        static_cast<QWidget*>( valueStack->child( "textRuleValueHider",
                                                  0, false ) );
      valueStack->raiseWidget( w );
    }
    else {
      RegExpLineEdit *lineEdit =
        dynamic_cast<RegExpLineEdit*>( valueStack->child( "regExpLineEdit",
                                                          0, false ) );
      if ( lineEdit ) {
        lineEdit->blockSignals( true );
        lineEdit->setText( rule->contents() );
        lineEdit->blockSignals( false );
        lineEdit->showEditButton( func == KMSearchRule::FuncRegExp ||
                                  func == KMSearchRule::FuncNotRegExp );
        valueStack->raiseWidget( lineEdit );
      }
    }
    return true;
  }


  //---------------------------------------------------------------------------

  bool MessageRuleWidgetHandler::update( const QCString & field,
                                      QWidgetStack *functionStack,
                                      QWidgetStack *valueStack ) const
  {
    if ( !handlesField( field ) )
      return false;
    // raise the correct function widget
    functionStack->raiseWidget(
      static_cast<QWidget*>( functionStack->child( "messageRuleFuncCombo",
                                                   0, false ) ) );

    // raise the correct value widget
    KMSearchRule::Function func = currentFunction( functionStack );
    if ( func == KMSearchRule::FuncHasAttachment  ||
         func == KMSearchRule::FuncHasNoAttachment ) {
      QWidget *w =
        static_cast<QWidget*>( valueStack->child( "textRuleValueHider",
                                                  0, false ) );
      valueStack->raiseWidget( w );
    }
    else {
      RegExpLineEdit *lineEdit =
        dynamic_cast<RegExpLineEdit*>( valueStack->child( "regExpLineEdit",
                                                          0, false ) );
      if ( lineEdit ) {
        lineEdit->showEditButton( func == KMSearchRule::FuncRegExp ||
                                  func == KMSearchRule::FuncNotRegExp );
        valueStack->raiseWidget( lineEdit );
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
                                                           QWidgetStack *functionStack,
                                                           const QObject *receiver ) const
  {
    if ( number != 0 )
      return 0;

    QComboBox *funcCombo = new QComboBox( functionStack,
                                          "statusRuleFuncCombo" );
    for ( int i = 0; i < StatusFunctionCount; ++i ) {
      funcCombo->insertItem( i18n( StatusFunctions[i].displayName ) );
    }
    funcCombo->adjustSize();
    QObject::connect( funcCombo, SIGNAL( activated( int ) ),
                      receiver, SLOT( slotFunctionChanged() ) );
    return funcCombo;
  }

  //---------------------------------------------------------------------------

  QWidget * StatusRuleWidgetHandler::createValueWidget( int number,
                                                        QWidgetStack *valueStack,
                                                        const QObject *receiver ) const
  {
    if ( number != 0 )
      return 0;

    QComboBox *statusCombo = new QComboBox( valueStack,
                                            "statusRuleValueCombo" );
    for ( int i = 0; i < KMail::StatusValueCountWithoutHidden; ++i ) {
      statusCombo->insertItem( i18n( KMail::StatusValues[i] ) );
    }
    statusCombo->adjustSize();
    QObject::connect( statusCombo, SIGNAL( activated( int ) ),
                      receiver, SLOT( slotValueChanged() ) );
    return statusCombo;
  }

  //---------------------------------------------------------------------------

  KMSearchRule::Function StatusRuleWidgetHandler::currentFunction( const QWidgetStack *functionStack ) const
  {
    const QComboBox *funcCombo =
      dynamic_cast<QComboBox*>( QObject_child_const( functionStack,
                                                     "statusRuleFuncCombo" ) );
    // FIXME (Qt >= 4.0): Use the following when QObject::child() is const.
    //  dynamic_cast<QComboBox*>( functionStack->child( "statusRuleFuncCombo",
    //                                                  0, false ) );
    if ( funcCombo ) {
      return StatusFunctions[funcCombo->currentItem()].id;
    }
    else
      kdDebug(5006) << "StatusRuleWidgetHandler::currentFunction: "
                       "statusRuleFuncCombo not found." << endl;
    return KMSearchRule::FuncNone;
  }

  //---------------------------------------------------------------------------

  KMSearchRule::Function StatusRuleWidgetHandler::function( const QCString & field,
                                                            const QWidgetStack *functionStack ) const
  {
    if ( !handlesField( field ) )
      return KMSearchRule::FuncNone;

    return currentFunction( functionStack );
  }

  //---------------------------------------------------------------------------

  int StatusRuleWidgetHandler::currentStatusValue( const QWidgetStack *valueStack ) const
  {
    const QComboBox *statusCombo =
      dynamic_cast<QComboBox*>( QObject_child_const( valueStack,
                                                     "statusRuleValueCombo" ) );
    // FIXME (Qt >= 4.0): Use the following when QObject::child() is const.
    //  dynamic_cast<QComboBox*>( valueStack->child( "statusRuleValueCombo",
    //                                               0, false ) );
    if ( statusCombo ) {
      return statusCombo->currentItem();
    }
    else
      kdDebug(5006) << "StatusRuleWidgetHandler::currentStatusValue: "
                       "statusRuleValueCombo not found." << endl;
    return -1;
  }

  //---------------------------------------------------------------------------

  QString StatusRuleWidgetHandler::value( const QCString & field,
                                          const QWidgetStack *,
                                          const QWidgetStack *valueStack ) const
  {
    if ( !handlesField( field ) )
      return QString::null;

    const int status = currentStatusValue( valueStack );
    if ( status != -1 )
      return QString::fromLatin1( KMail::StatusValues[status] );
    else
      return QString::null;
  }

  //---------------------------------------------------------------------------

  QString StatusRuleWidgetHandler::prettyValue( const QCString & field,
                                                const QWidgetStack *,
                                                const QWidgetStack *valueStack ) const
  {
    if ( !handlesField( field ) )
      return QString::null;

    const int status = currentStatusValue( valueStack );
    if ( status != -1 )
      return i18n( KMail::StatusValues[status] );
    else
      return QString::null;
  }

  //---------------------------------------------------------------------------

  bool StatusRuleWidgetHandler::handlesField( const QCString & field ) const
  {
    return ( field == "<status>" );
  }

  //---------------------------------------------------------------------------

  void StatusRuleWidgetHandler::reset( QWidgetStack *functionStack,
                                       QWidgetStack *valueStack ) const
  {
    // reset the function combo box
    QComboBox *funcCombo =
      dynamic_cast<QComboBox*>( functionStack->child( "statusRuleFuncCombo",
                                                      0, false ) );
    if ( funcCombo ) {
      funcCombo->blockSignals( true );
      funcCombo->setCurrentItem( 0 );
      funcCombo->blockSignals( false );
    }

    // reset the status value combo box
    QComboBox *statusCombo =
      dynamic_cast<QComboBox*>( valueStack->child( "statusRuleValueCombo",
                                                   0, false ) );
    if ( statusCombo ) {
      statusCombo->blockSignals( true );
      statusCombo->setCurrentItem( 0 );
      statusCombo->blockSignals( false );
    }
  }

  //---------------------------------------------------------------------------

  bool StatusRuleWidgetHandler::setRule( QWidgetStack *functionStack,
                                         QWidgetStack *valueStack,
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
      dynamic_cast<QComboBox*>( functionStack->child( "statusRuleFuncCombo",
                                                      0, false ) );
    if ( funcCombo ) {
      funcCombo->blockSignals( true );
      if ( funcIndex < StatusFunctionCount )
        funcCombo->setCurrentItem( funcIndex );
      else {
        kdDebug(5006) << "StatusRuleWidgetHandler::setRule( "
                      << rule->asString()
                      << " ): unhandled function" << endl;
        funcCombo->setCurrentItem( 0 );
      }
      funcCombo->blockSignals( false );
      functionStack->raiseWidget( funcCombo );
    }

    // set the value
    const QString value = rule->contents();
    int valueIndex = 0;
    for ( ; valueIndex < KMail::StatusValueCountWithoutHidden; ++valueIndex )
      if ( value == QString::fromLatin1(
               KMail::StatusValues[valueIndex] ) )
        break;
    QComboBox *statusCombo =
      dynamic_cast<QComboBox*>( valueStack->child( "statusRuleValueCombo",
                                                   0, false ) );
    if ( statusCombo ) {
      statusCombo->blockSignals( true );
      if ( valueIndex < KMail::StatusValueCountWithoutHidden )
        statusCombo->setCurrentItem( valueIndex );
      else {
        kdDebug(5006) << "StatusRuleWidgetHandler::setRule( "
                      << rule->asString()
                      << " ): unhandled value" << endl;
        statusCombo->setCurrentItem( 0 );
      }
      statusCombo->blockSignals( false );
      valueStack->raiseWidget( statusCombo );
    }
    return true;
  }


  //---------------------------------------------------------------------------

  bool StatusRuleWidgetHandler::update( const QCString &field,
                                        QWidgetStack *functionStack,
                                        QWidgetStack *valueStack ) const
  {
    if ( !handlesField( field ) )
      return false;

    // raise the correct function widget
    functionStack->raiseWidget(
      static_cast<QWidget*>( functionStack->child( "statusRuleFuncCombo",
                                                   0, false ) ) );

    // raise the correct value widget
    valueStack->raiseWidget(
      static_cast<QWidget*>( valueStack->child( "statusRuleValueCombo",
                                                0, false ) ) );
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
                                                            QWidgetStack *functionStack,
                                                            const QObject *receiver ) const
  {
    if ( number != 0 )
      return 0;

    QComboBox *funcCombo = new QComboBox( functionStack,
                                          "numericRuleFuncCombo" );
    for ( int i = 0; i < NumericFunctionCount; ++i ) {
      funcCombo->insertItem( i18n( NumericFunctions[i].displayName ) );
    }
    funcCombo->adjustSize();
    QObject::connect( funcCombo, SIGNAL( activated( int ) ),
                      receiver, SLOT( slotFunctionChanged() ) );
    return funcCombo;
  }

  //---------------------------------------------------------------------------

  QWidget * NumericRuleWidgetHandler::createValueWidget( int number,
                                                         QWidgetStack *valueStack,
                                                         const QObject *receiver ) const
  {
    if ( number != 0 )
      return 0;

    KIntNumInput *numInput = new KIntNumInput( valueStack, "KIntNumInput" );
    QObject::connect( numInput, SIGNAL( valueChanged( int ) ),
                      receiver, SLOT( slotValueChanged() ) );
    return numInput;
  }

  //---------------------------------------------------------------------------

  KMSearchRule::Function NumericRuleWidgetHandler::currentFunction( const QWidgetStack *functionStack ) const
  {
    const QComboBox *funcCombo =
      dynamic_cast<QComboBox*>( QObject_child_const( functionStack,
                                                     "numericRuleFuncCombo" ) );
    // FIXME (Qt >= 4.0): Use the following when QObject::child() is const.
    //  dynamic_cast<QComboBox*>( functionStack->child( "numericRuleFuncCombo",
    //                                                  0, false ) );
    if ( funcCombo ) {
      return NumericFunctions[funcCombo->currentItem()].id;
    }
    else
      kdDebug(5006) << "NumericRuleWidgetHandler::currentFunction: "
                       "numericRuleFuncCombo not found." << endl;
    return KMSearchRule::FuncNone;
  }

  //---------------------------------------------------------------------------

  KMSearchRule::Function NumericRuleWidgetHandler::function( const QCString & field,
                                                             const QWidgetStack *functionStack ) const
  {
    if ( !handlesField( field ) )
      return KMSearchRule::FuncNone;

    return currentFunction( functionStack );
  }

  //---------------------------------------------------------------------------

  QString NumericRuleWidgetHandler::currentValue( const QWidgetStack *valueStack ) const
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
      kdDebug(5006) << "NumericRuleWidgetHandler::currentValue: "
                       "KIntNumInput not found." << endl;
    return QString::null;
  }

  //---------------------------------------------------------------------------

  QString NumericRuleWidgetHandler::value( const QCString & field,
                                           const QWidgetStack *,
                                           const QWidgetStack *valueStack ) const
  {
    if ( !handlesField( field ) )
      return QString::null;

    return currentValue( valueStack );
  }

  //---------------------------------------------------------------------------

  QString NumericRuleWidgetHandler::prettyValue( const QCString & field,
                                                 const QWidgetStack *,
                                                 const QWidgetStack *valueStack ) const
  {
    if ( !handlesField( field ) )
      return QString::null;

    return currentValue( valueStack );
  }

  //---------------------------------------------------------------------------

  bool NumericRuleWidgetHandler::handlesField( const QCString & field ) const
  {
    return ( field == "<size>" || field == "<age in days>" );
  }

  //---------------------------------------------------------------------------

  void NumericRuleWidgetHandler::reset( QWidgetStack *functionStack,
                                        QWidgetStack *valueStack ) const
  {
    // reset the function combo box
    QComboBox *funcCombo =
      dynamic_cast<QComboBox*>( functionStack->child( "numericRuleFuncCombo",
                                                      0, false ) );
    if ( funcCombo ) {
      funcCombo->blockSignals( true );
      funcCombo->setCurrentItem( 0 );
      funcCombo->blockSignals( false );
    }

    // reset the value widget
    KIntNumInput *numInput =
      dynamic_cast<KIntNumInput*>( valueStack->child( "KIntNumInput",
                                                      0, false ) );
    if ( numInput ) {
      numInput->blockSignals( true );
      numInput->setValue( 0 );
      numInput->blockSignals( false );
    }
  }

  //---------------------------------------------------------------------------

  void initNumInput( KIntNumInput *numInput, const QCString &field )
  {
    if ( field == "<size>" ) {
      numInput->setMinValue( 0 );
      numInput->setSuffix( i18n( " bytes" ) );
    }
    else {
      numInput->setMinValue( -10000 );
      numInput->setSuffix( i18n( " days" ) );
    }
  }

  //---------------------------------------------------------------------------

  bool NumericRuleWidgetHandler::setRule( QWidgetStack *functionStack,
                                          QWidgetStack *valueStack,
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
      dynamic_cast<QComboBox*>( functionStack->child( "numericRuleFuncCombo",
                                                      0, false ) );
    if ( funcCombo ) {
      funcCombo->blockSignals( true );
      if ( funcIndex < NumericFunctionCount )
        funcCombo->setCurrentItem( funcIndex );
      else {
        kdDebug(5006) << "NumericRuleWidgetHandler::setRule( "
                      << rule->asString()
                      << " ): unhandled function" << endl;
        funcCombo->setCurrentItem( 0 );
      }
      funcCombo->blockSignals( false );
      functionStack->raiseWidget( funcCombo );
    }

    // set the value
    bool ok;
    int value = rule->contents().toInt( &ok );
    if ( !ok )
      value = 0;
    KIntNumInput *numInput =
      dynamic_cast<KIntNumInput*>( valueStack->child( "KIntNumInput",
                                                      0, false ) );
    if ( numInput ) {
      initNumInput( numInput, rule->field() );
      numInput->blockSignals( true );
      numInput->setValue( value );
      numInput->blockSignals( false );
      valueStack->raiseWidget( numInput );
    }
    return true;
  }


  //---------------------------------------------------------------------------

  bool NumericRuleWidgetHandler::update( const QCString &field,
                                         QWidgetStack *functionStack,
                                         QWidgetStack *valueStack ) const
  {
    if ( !handlesField( field ) )
      return false;

    // raise the correct function widget
    functionStack->raiseWidget(
      static_cast<QWidget*>( functionStack->child( "numericRuleFuncCombo",
                                                   0, false ) ) );

    // raise the correct value widget
    KIntNumInput *numInput =
      dynamic_cast<KIntNumInput*>( valueStack->child( "KIntNumInput",
                                                      0, false ) );
    if ( numInput ) {
      initNumInput( numInput, field );
      valueStack->raiseWidget( numInput );
    }
    return true;
  }

} // anonymous namespace for NumericRuleWidgetHandler


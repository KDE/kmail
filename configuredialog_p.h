// configuredialog_p.h: classes internal to ConfigureDialog
// see configuredialog.h for details.

#ifndef _CONFIGURE_DIALOG_PRIVATE_H_
#define _CONFIGURE_DIALOG_PRIVATE_H_

#include <qstring.h>
#include <qvaluelist.h>
#include <qstringlist.h>
#include <qlineedit.h>
#include <qcombobox.h>

#include <kdialogbase.h>

class QWidget;

//
//
// Identity handling
//
//

class IdentityEntry
{
public:
  IdentityEntry() : mSignatureFileIsAProgram( false ),
    mUseSignatureFile( true ), mIsDefault( false ) {}
  /** Convenience constructor for the most-often used settings */
  IdentityEntry( const QString & fullName,
		 const QString & email,
		 const QString & organization=QString::null,
		 const QString & replyTo=QString::null )
    : mFullName( fullName ), mOrganization( organization),
    mEmailAddress( email ), mReplyToAddress( replyTo ),
    mSignatureFileIsAProgram( false ),
    mUseSignatureFile( true ), mIsDefault( false ) {}

  /** Constructs an IdentituEntry from the control center default
      settings */
  static IdentityEntry fromControlCenter();

  /** used for sorting */
  bool operator<( const IdentityEntry & other ) const {
    // default is lower than any other, so it is always sorted to the
    // top:
    if ( isDefault() ) return true;
    if ( other.isDefault() ) return false;
    return identityName() < other.identityName();
  }
  // begin stupid stl headers workaround
  bool operator>( const IdentityEntry & other ) const {
    if ( isDefault() ) return false;
    if ( other.isDefault() ) return true;
    return identityName() > other.identityName();
  }
  bool operator<=( const IdentityEntry & other ) const {
    return !operator>( other );
  }
  bool operator>=( const IdentityEntry & other ) const {
    return !operator<( other );
  }
  // end stupid stl headers workaround
  /** used for searching */
  bool operator==( const IdentityEntry & other ) const {
    return identityName() == other.identityName();
  }

  QString identityName() const { return mIdentityName; }
  QString fullName() const { return mFullName; }
  QString organization() const { return mOrganization; }
  QString pgpIdentity() const { return mPgpIdentity ; }
  QString emailAddress() const { return mEmailAddress; }
  QString replyToAddress() const { return mReplyToAddress; }
  QString signatureFileName() const { return mSignatureFileName; }
  QString signatureInlineText() const { return mSignatureInlineText; }
  bool    signatureFileIsAProgram() const { return mSignatureFileIsAProgram; }
  bool    useSignatureFile() const { return mUseSignatureFile; }
  bool    isDefault() const { return mIsDefault; }
  QString transport() const { return mTransport; }
  QString fcc() const { return mFcc; }
  
  void setIdentityName( const QString & i ) { mIdentityName = i; }
  void setFullName( const QString & f ) { mFullName = f; }
  void setOrganization( const QString & o ) { mOrganization = o; }
  void setPgpIdentity( const QString & p ) { mPgpIdentity = p; }
  void setEmailAddress( const QString & e ) { mEmailAddress = e; }
  void setReplyToAddress( const QString & r ) { mReplyToAddress = r; }
  void setSignatureFileName( const QString & s ) { mSignatureFileName = s; }
  void setSignatureInlineText( const QString & t ) { mSignatureInlineText = t; }
  void setSignatureFileIsAProgram( bool b ) { mSignatureFileIsAProgram = b; }
  void setUseSignatureFile( bool b ) { mUseSignatureFile = b; }
  void setIsDefault( bool b ) { mIsDefault = b; }
  void setTransport( const QString & t ) { mTransport = t; }
  void setFcc( const QString & f ) { mFcc = f; }

private:
  // only add members that have an operator= defined or where the
  // compiler can generate one by itself!!
  QString mIdentityName;
  QString mFullName;
  QString mOrganization;
  QString mPgpIdentity;
  QString mEmailAddress;
  QString mReplyToAddress;
  QString mSignatureFileName;
  QString mSignatureInlineText;
  QString mTransport;
  QString mFcc;
  bool    mSignatureFileIsAProgram;
  bool    mUseSignatureFile;
  bool    mIsDefault;
};


class IdentityList : public QValueList<IdentityEntry>
{
public:
  // Let the compiler generate c'tors, d'tor and operator=. They are
  // sufficient.

  /** generates a list of display names of identies for use in
      widgets. */
  QStringList names() const;
  /** Gets and @ref IdentityEntry by name */
  IdentityEntry & getByName( const QString & identity );

  // remove when no longer commented out in qvaluelist.h:
  void sort() { qHeapSort( *this ); }

  /** Load system settings */
  void importData();
  /** Save state to system */
  void exportData() const;
};


class NewIdentityDialog : public KDialogBase
{
  Q_OBJECT

public:
  enum DuplicateMode { Empty, ControlCenter, ExistingEntry };

  NewIdentityDialog( const QStringList & identities,
		     QWidget *parent=0, const char *name=0, bool modal=true );
  
  QString identityName() const { return mLineEdit->text(); }
  int duplicateIdentity() const { return mComboBox->currentItem(); }
  DuplicateMode duplicateMode() const { return mDuplicateMode; }
  
protected slots:
  virtual void slotOk();
  virtual void slotRadioClicked( int which );

private:
  QLineEdit  *mLineEdit;
  QComboBox  *mComboBox;
  DuplicateMode mDuplicateMode;
};



#endif // _CONFIGURE_DIALOG_PRIVATE_H_

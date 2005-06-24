/* -*- mode: C++; c-file-style: "gnu" -*-
 * KMComposeWin Header File
 * Author: Markus Wuebben <markus.wuebben@kde.org>
 */
#ifndef __KMAIL_KMLINEEDITSPELL_H__
#define __KMAIL_KMLINEEDITSPELL_H__

#include <libkdepim/addresseelineedit.h>

class QPopupMenu;

class KMLineEdit : public KPIM::AddresseeLineEdit
{
    Q_OBJECT
public:
    KMLineEdit(bool useCompletion, QWidget *parent = 0,
               const char *name = 0);

signals:
    void focusUp();
    void focusDown();

protected:
    // Inherited. Always called by the parent when this widget is created.
    virtual void loadContacts();

    virtual void keyPressEvent(QKeyEvent*);

    virtual QPopupMenu *createPopupMenu();

private slots:
    void editRecentAddresses();

private:
    void dropEvent( QDropEvent *event );
    void insertEmails( const QStringList & emails );
};


class KMLineEditSpell : public KMLineEdit
{
    Q_OBJECT
public:
    KMLineEditSpell(bool useCompletion, QWidget *parent = 0,
               const char *name = 0);
    void highLightWord( unsigned int length, unsigned int pos );
    void spellCheckDone( const QString &s );
    void spellCheckerMisspelling( const QString &text, const QStringList &, unsigned int pos);
    void spellCheckerCorrected( const QString &old, const QString &corr, unsigned int pos);

 signals:
  void subjectTextSpellChecked();
};

#endif // __KMAIL_KMLINEEDITSPELL_H__

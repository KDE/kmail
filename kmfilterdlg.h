/* Filter Dialog
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef kmfilterdlg_h
#define kmfilterdlg_h

#include "kmfilteraction.h"

class QBoxLayout;
class QListBox;

#define KMFilterDlgInherited KMGFilterDlg
class KMFilterDlg: public KMGFilterDlg
{
  Q_OBJECT

public:
  KMFilterDlg(QWidget* parent=NULL, const char* name=NULL);
  virtual ~KMFilterDlg();

  /** Methods for filter options: */
  virtual void addLabel(const QString label);
  virtual void addEntry(const QString label, QString value);
  virtual void addFileNameEntry(const QString label, QString value,
				const QString pattern, const QString startdir);
  virtual void addToggle(const QString label, bool* value);
  virtual void addFolderList(const QString label, KMFolder** folderPtr);
  virtual void addWidget(const QString label, QWidget* widget);

protected:
  // fill listbox with filter list
  virtual void reloadFilterList(void);

  QBoxLayout* mBox;
  QListBox* mFilterList;
};

#endif /*kmfilterdlg_h*/

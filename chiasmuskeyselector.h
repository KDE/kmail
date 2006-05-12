#ifndef CHIASMUSKEYSELECTOR_H
#define CHIASMUSKEYSELECTOR_H

#include <kdialog.h>
#include <QLabel>
class KListBox;
class KLineEdit;
class QLabel;

class ChiasmusKeySelector : public KDialog
{
  Q_OBJECT

public:
  ChiasmusKeySelector( QWidget* parent, const QString& caption,
                       const QStringList& keys, const QString& currentKey,
                       const QString& lastOptions );

  QString key() const;
  QString options() const;

private:
  QLabel* mLabel;
  KListBox* mListBox;
  KLineEdit* mOptions;
};

#endif

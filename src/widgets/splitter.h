#ifndef SPLITTER_H_
#define SPLITTER_H_

#include <QSplitter>

class Splitter : public QSplitter
{
    Q_OBJECT
public:
    using QSplitter::QSplitter;

protected:
    QSplitterHandle * createHandle() override;
};

#endif 

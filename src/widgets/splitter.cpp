#include "splitter.h"

#include <QSplitterHandle>
#include <QPaintEvent>
#include <QPainter>
#include <QStyle>
#include <QPalette>

namespace {

class SplitterHandle : public QSplitterHandle
{
    Q_OBJECT
public:
    using QSplitterHandle::QSplitterHandle;

protected:
    void paintEvent(QPaintEvent *event) override
    {
        const auto t = palette().text().color();
        const auto w = palette().window().color();
        const auto a = 0.7f;
        const auto invA = 1.f - a;

        QPainter painter(this);
        painter.fillRect(event->rect(), QColor::fromRgbF(w.redF() * a + t.redF() * invA,
                                                         w.greenF() * a + t.greenF() * invA,
                                                         w.blueF() * a + t.blueF() * invA));
    }

};

}

QSplitterHandle * Splitter::createHandle()
{
    return new SplitterHandle(orientation(), this);
}

#include "splitter.moc"

#ifndef kcursorsaver_h
#define kcursorsaver_h

#include <qcursor.h>
#include <qapplication.h>

class KCursorSaver : public Qt
{
public:
    KCursorSaver(Qt::CursorShape shape) {
        QApplication::setOverrideCursor( QCursor(shape) );
        inited = true;
    }

    KCursorSaver( const KCursorSaver &rhs ) {
        *this = rhs;
    }

    ~KCursorSaver() {
        if (inited)
            QApplication::restoreOverrideCursor();
    }

    inline void restoreCursor(void) {
        QApplication::restoreOverrideCursor();
        inited = false;
    }

protected:
    void operator=( const KCursorSaver &rhs ) {
        inited = rhs.inited;
        rhs.inited = false;
    }

private:
    mutable bool inited;
};

namespace KBusyPtr {
    inline KCursorSaver idle() {
        return KCursorSaver(QCursor::ArrowCursor);
    }
    inline KCursorSaver busy() {
        return KCursorSaver(QCursor::WaitCursor);
    }
}

#endif /*kbusyptr_h_*/

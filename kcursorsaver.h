#ifndef kcursorsaver_h
#define kcursorsaver_h

#include <qcursor.h>
#include <qapplication.h>

/**
 * @short sets a cursor and makes sure it's restored on destruction
 * Create a KCursorSaver object when you want to set the cursor.
 * As soon as it gets out of scope, it will restore the original
 * cursor.
 */
class KCursorSaver : public Qt
{
public:
    /// constructor taking QCursor shapes
    KCursorSaver(Qt::CursorShape shape) {
        QApplication::setOverrideCursor( QCursor(shape) );
        inited = true;
    }

    /// copy constructor. The right side won't restore the cursor
    KCursorSaver( const KCursorSaver &rhs ) {
        *this = rhs;
    }

    /// restore the cursor
    ~KCursorSaver() {
        if (inited)
            QApplication::restoreOverrideCursor();
    }

    /// call this to explitly restore the cursor
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

/**
 * convencience functions
 */
namespace KBusyPtr {
    inline KCursorSaver idle() {
        return KCursorSaver(QCursor::ArrowCursor);
    }
    inline KCursorSaver busy() {
        return KCursorSaver(QCursor::WaitCursor);
    }
}

#endif /*kbusyptr_h_*/

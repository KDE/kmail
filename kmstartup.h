#ifndef KMSTARTUP
#define KMSTARTUP

// Author: Don Sanders <sanders@kde.org>
// License GPL

#include <kdepimmacros.h>

extern "C" {

KDE_EXPORT void kmsetSignalHandler(void (*handler)(int));
KDE_EXPORT void kmsignalHandler(int sigId);
KDE_EXPORT void kmcrashHandler(int sigId);

}

namespace KMail
{
    KDE_EXPORT void checkConfigUpdates();
    KDE_EXPORT void lockOrDie();
    KDE_EXPORT void insertLibraryCataloguesAndIcons();
    KDE_EXPORT void cleanup();
}

#endif






#ifndef KMSTARTUP
#define KMSTARTUP

// Author: Don Sanders <sanders@kde.org>
// License GPL

extern "C" {

void kmsetSignalHandler(void (*handler)(int));
void kmsignalHandler(int sigId);
void kmcrashHandler(int sigId);

}

namespace KMail
{
    QString getMyHostName(void);
    void checkConfigUpdates();
    void lockOrDie();
    void cleanup();
}

#endif






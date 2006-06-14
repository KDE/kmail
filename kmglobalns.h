/*  KMail Global Text Completion Modes
 *
 *  This static struct is used at two files - configuredialog.cpp and
 *  kmcomposewin.cpp.
 *
 *  
 *
 *  
 */
#ifndef kmglobalns_h
#define kmglobalns_h

#include <kglobalsettings.h> //for KGlobalSettings

/** Global Completion Modes*/
// This struct comes from configuredialog.cpp. It is now being used by
// kmcomposewin.cpp also.
class KMGlobalNS
{
  public :
    struct cMode
    {
      KGlobalSettings::Completion mode;
      const char* displayName;
    };

    static const cMode *completionModes;
    static const int numCompletionModes;
};

#endif

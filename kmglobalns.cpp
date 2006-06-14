#include "kmglobalns.h"

#include <klocale.h> //for I18N_NOOP

static const struct KMGlobalNS::cMode tempModes[] = {
       { KGlobalSettings::CompletionNone, I18N_NOOP("None") },
       { KGlobalSettings::CompletionShell, I18N_NOOP("Manual") },
       { KGlobalSettings::CompletionAuto, I18N_NOOP("Automatic") },
       { KGlobalSettings::CompletionPopup, I18N_NOOP("Dropdown List") },
       { KGlobalSettings::CompletionMan, I18N_NOOP("Short Automatic") },
       { KGlobalSettings::CompletionPopupAuto, I18N_NOOP("Dropdown List && Automatic") }
     };

const KMGlobalNS::cMode *KMGlobalNS::completionModes = tempModes;

const int KMGlobalNS::numCompletionModes = sizeof tempModes / sizeof *tempModes;

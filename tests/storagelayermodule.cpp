/**
 * Copyright (C)  2005 Till Adam <adam@kde.org>
 * 
 * This software is under the "if you run these tests and they break you 
 * have to fix it" license.
 */

#include <kunittest/runner.h>
#include <kunittest/module.h>

#include "messagedicttests.h"

using namespace KUnitTest;

KUNITTEST_MODULE( kunittest_storagelayermodule, "KMail Storage Layer Tests" );
KUNITTEST_MODULE_REGISTER_TESTER( MessageDictTester );

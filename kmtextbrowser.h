#ifndef KMTEXTBROWSER_H
#define KMTEXTBROWSER_H

#include <ktextbrowser.h>

/**
 * A tiny little class to use for displaying raw messages, textual 
 * attachments etc.
 *
 * Auto-deletes itself when closed.
 */
class KMTextBrowser : public KTextBrowser
{
public:
    KMTextBrowser( QWidget *parent = 0, const char *name = 0 );
};

#endif // KMTEXTBROWSER_H

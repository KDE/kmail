// kshortcut.cpp

#include "kshortcut.h"

KShortCut::KShortCut(KConfig* cfg): KKeyConfig(cfg)
{
}

uint KShortCut::readKey(const char* keyName, uint defaultKey) const
{
  uint key;
  key = ((KShortCut*)this)->readCurrentKey(keyName);
  return (key ? key : defaultKey);
}

uint KShortCut::open(void) const
{
  return readKey("Open", CTRL+Key_O);
}

uint KShortCut::openNew(void) const
{
  return readKey("OpenNew", CTRL+Key_N);
}

uint KShortCut::close(void) const
{
  return readKey("Close", CTRL+Key_W);
}

uint KShortCut::save(void) const
{
  return readKey("Save", CTRL+Key_S);
}

uint KShortCut::print(void) const
{
  return readKey("Print", CTRL+Key_P);
}

uint KShortCut::quit(void) const
{
  return readKey("Quit", CTRL+Key_Q);
}

uint KShortCut::cut(void) const
{
  return readKey("Cut", CTRL+Key_X);
}

uint KShortCut::copy(void) const
{
  return readKey("Copy", CTRL+Key_C);
}

uint KShortCut::paste(void) const
{
  return readKey("Paste", CTRL+Key_V);
}

uint KShortCut::undo(void) const
{
  return readKey("Undo", CTRL+Key_Z);
}

uint KShortCut::find(void) const
{
  return readKey("Find", CTRL+Key_F);
}

uint KShortCut::replace(void) const
{
  return readKey("Replace", CTRL+Key_R);
}

uint KShortCut::insert(void) const
{
  return readKey("Insert", CTRL+Key_Insert);
}

uint KShortCut::home(void) const
{
  return readKey("Home", CTRL+Key_Home);
}

uint KShortCut::end(void) const
{
  return readKey("End", CTRL+Key_End);
}

uint KShortCut::prior(void) const
{
  return readKey("Prior", Key_Prior);
}

uint KShortCut::next(void) const
{
  return readKey("Next", Key_Next);
}

uint KShortCut::help(void) const
{
  return readKey("Help", Key_F1);
}


CC=gcc
CCLIENT=/usr/local/src/c-client
CFLAGS=-O2 -Wall -g -DSTDC_HEADERS -DUSE_UNISTD_H
INC=-I$(CCLIENT) -I$(KDEDIR)/include -I$(QTDIR)/include -I/usr/X11R6/include
LIB=$(CCLIENT)/c-client.a -L$(KDEDIR)/lib -lkdeui -lkdecore -lqt -L/usr/X11/lib -lXext
BIN=kmail
SRC=kmmainwin.cpp callback.cpp mclass.cpp kmfolderdia.cpp kmfoldertree.cpp kmsettings.cpp util.cpp kmaccount.cpp kmheaders.cpp ktablistbox.cpp kmcomposewin.cpp mutil.cpp
OBJS=kmmainwin.o callback.o mclass.o kmfolderdia.o kmfoldertree.o kmsettings.o util.o kmaccount.o kmheaders.o ktablistbox.o kmcomposewin.o mutil.o
HDR=kmmainwin.h mclass.h kmfolderdia.h kmfoldertree.h kmsettings.h util.h kmaccount.h kmheaders.h ktablistbox.h kmcomposewin.h mutil.h
MOC=kmmainwin.moc kmfolderdia.moc kmfoldertree.moc kmsettings.moc kmheaders.moc ktablistbox.moc kmcomposewin.moc

.SUFFIXES: .cpp

.cpp.o:
	$(CC) -c $(CFLAGS) $(INC) $<

kmail: $(SRC) $(HDR) $(MOC) $(OBJS)
	$(CC) -o $(BIN) $(OBJS) $(LIB)

%.moc:	%.h
	moc -o $@ $<

clean:
	rm -f $(OBJS) $(BIN) $(MOC) *~ core .tedfilepos


#ifndef MCLASS_H
#define MCLASS_H

#include <stdio.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include <mail.h>
#include <rfc822.h>
#include <smtp.h>
#include <misc.h>
#include <env.h>
#include <fs.h>
}

#include "mutil.h"

// So I am lazy
#define ULONG	unsigned long
#define CCHAR	const char *

#define BODYTYPE \
{ "TEXT", "MULTIPART", "TYPEMESSAGE", "APPLICATION", "AUDIO", "IMAGE",\
"VIDEO", "OTHER" }

#define ENCODING \
{ "7BIT", "8BIT", "BINARY", "BASE64", "QUOTEDPRINTABLE", "OTHER" }

#define TYPEINVALID -1
#define ENCINVALID -1

// Flags of a message
#define F_DEL	"\\DELETED"
#define F_NEW	"\\RECENT"
#define F_ANS	"\\ANSWERED"
#define	F_SEEN 	"\\SEEN"

// Length 
#define L_FROM		30
#define L_SUBJECT	25
#define	L_FILE		100

#define M_READONLY 0
#define M_WRITABLE 1

// Service available
#define SRV_MBOX 0
#define SRV_POP3 1
#define	SRV_IMAP 2

#define MBOX(s) (s->mailbox)

//////////////////////////////////////////////////////////////////////////////
// Attachment Class
//////////////////////////////////////////////////////////////////////////////
class Message;

class Attachment {
friend class Message;
private:
	short type;
	char *subtype;
	char *description;
	char *filename;
	char *charset;
	short encoding;
	void *data;
	ULONG length;
public:
	Attachment();
	~Attachment();
	long guess(CCHAR);
	long save(CCHAR);
	long isText() const { return type == TYPETEXT? T : NIL; }
	long isBinary() const { return type != TYPETEXT? T : NIL; }
	short getType() const { return type; }
	CCHAR getSubtype() const { return (CCHAR)subtype; }
	CCHAR getDescription() const { return (CCHAR)description; }
	CCHAR getFilename() const { return (CCHAR)filename; }
	short getEncoding() const { return encoding; }
	CCHAR getText(ULONG *len) { *len = length; return (CCHAR)data; }
	CCHAR getCharset() const { return (CCHAR)charset; }
	void *getData(ULONG *len) { *len = length; return data; }

	// Probably won't use 
	void setType(short t) { type = t; }
	void setSubtype(CCHAR d) {
        if (subtype)
            fs_give((void **)&subtype);
        subtype = strdup(d);
    }
	void setDescription(CCHAR d) { 
		if (description)
			fs_give((void **)&description);
		description = strdup(d); 
	}
	void setFilename(CCHAR f) {
        if (filename)
            fs_give((void **)&filename);
        filename = strdup(f);
    }
	void setCharset(CCHAR c) {
        if (charset)
            fs_give((void **)&charset);
        filename = strdup(c);
    }
	void setEncoding(short e) { encoding = e; }
	void setData(void *d) { data = d; }
};

//////////////////////////////////////////////////////////////////////////////
// Folder Class 
//////////////////////////////////////////////////////////////////////////////

class Message;

class Folder {
private:
	MAILSTREAM *stream;

public:
	Folder(){};
	~Folder();
	long open(CCHAR);
	void close(long = CL_EXPUNGE);
	long remove();
	long create(CCHAR mbox);
	long rename(CCHAR mbox);
	long status(long = SA_MESSAGES | SA_RECENT | SA_UNSEEN);
    void ping() { mail_ping(stream); status(); }
    void expunge() { mail_expunge(stream); }
	int	isValid(ULONG);
	ULONG numMsg() { return stream->nmsgs; }
	Message *getMsg(ULONG);
	long putMsg(Message *);
};

// Please don't get confused with MESSAGE of C-Client

class Message {
// Want Folder::getMsg to access the private constructor
friend Message *Folder::getMsg(ULONG);
friend long Folder::putMsg(Message *);
private:
	BODY *body;
	ENVELOPE *env;	
	ULONG msgno;
	int mode;
	MAILSTREAM *ms;

	int isValid() const {
		if (!ms)
			return 0;
		return msgno > 0 && msgno < ms->nmsgs? 1 : 0;
	}

protected:
	Message(MAILSTREAM *, ULONG);
	void setDate();
	BODY *getBodyPart(ULONG);
	char *toSTRING(ULONG *);

public:
	Message();
	~Message();

//////////////////////////////////////////////////////////////////
// Functions for accessing messages inside a mailbox
//////////////////////////////////////////////////////////////////
	// General
	CCHAR getText(ULONG *) const;
	CCHAR getHeader() const;
	void getDate(char *) const;
	void getSubject(char *) const;
	void getFrom(char *) const;
	int	getType() const;
	char getFlag() const;
	void setFlag(char *);
	void clearFlag(char *);
	void del() { setFlag(F_DEL); }
    void undel() { clearFlag(F_DEL); } 
	ULONG numAttch();
	long export(FILE *f);
	// MIME-specific
	Attachment *getAttachment(ULONG);

/////////////////////////////////////////////////////////////////
// Functions for composing/replying a message
/////////////////////////////////////////////////////////////////
	void setText(CCHAR);
	void setTo(CCHAR);
	void setFrom(CCHAR);
	void setCc(CCHAR);
	void setBcc(CCHAR);
	void setSubject(CCHAR);
	//setType(int);
	long sendSMTP(CCHAR);
	long sendMTA(CCHAR);
	// MIME-specific
	void attach(Attachment *);

	// Debug
	void debug();
};

void initCC();
long createFolder(char *);
void setBodyParamter(BODY *, char *, char *);
#endif

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

// Valid charsets
#define C_USASCII	"US-ASCII"
#define C_ISO8859_1	"ISO-8859-1"
#define C_ISO8859_2 "ISO-8859-2"
#define C_ISO8859_3 "ISO-8859-3"
#define C_ISO8859_4 "ISO-8859-4" 
#define C_ISO8859_5 "ISO-8859-5"
#define C_ISO8859_6 "ISO-8859-6" 
#define C_ISO8859_7 "ISO-8859-7"
#define C_ISO8859_8 "ISO-8859-8" 
#define C_ISO8859_9 "ISO-8859-9"

// Message mode
#define M_READONLY 0
#define M_WRITABLE 1
#define M_REPLY 2

// Length
#define L_FROM      30
#define L_SUBJECT   25
#define L_FILE      100

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
	long isText() const { return type == TYPETEXT? T : NIL; }
	long isBinary() const { return type != TYPETEXT? T : NIL; }
	short getType() const { return type; }
	CCHAR getSubtype() const { return (CCHAR)subtype; }
	CCHAR getDescription() const { return (CCHAR)description; }
	CCHAR getFilename() const { return (CCHAR)filename; }
	short getEncoding() const { return encoding; }
	CCHAR getCharset() const { return (CCHAR)charset; }
	void *getData(ULONG *len) { *len = length; return data; }

	// Probably won't use 
	void setType(short t) { type = t; }
	void setEncoding(short e) { encoding = e; }
	void setSubtype(CCHAR d) {
		if (!d)
			return;
        if (subtype)
            fs_give((void **)&subtype);
        subtype = cpystr(d);
    }
	void setDescription(CCHAR d) { 
		if (d) {
			if (description)
				fs_give((void **)&description);
			description = cpystr(d); 
		}
	}
	void setFilename(CCHAR f) {
		if (f) {
        	if (filename)
            	fs_give((void **)&filename);
        	filename = cpystr(f);
		}
    }
	void setCharset(CCHAR c) {
		if (c) {
        	if (charset)
            	fs_give((void **)&charset);
        	charset = cpystr(c);
		}
    }
	void setData(void *d, ULONG len) { data = d; length = len; }
	long save(const char *f = 0) {
		return writeFile(f? f : filename, data, length);
	}
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
	void setInReplyTo(CCHAR irt) {
		if (irt)
			env->in_reply_to = cpystr(irt);
	}
	void getAddress(ADDRESS *, char *) const;
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
	void getLongDate(char *buf) const {
		strcpy(buf, env->date);
	}
	void getSubject(char *) const;
	void getFrom(char *) const;
	int	getType() const;
	char getFlag() const;
	void getCc(char *buf) const {
		getAddress(env->cc, buf);
	}
	void getTo(char *buf) const {
		getAddress(env->to, buf);
	}
	void getReplyTo(char *buf) const {
		getAddress(env->reply_to, buf);
	}
	void setFlag(char *);
	void clearFlag(char *);
	void del() { setFlag(F_DEL); }
    void undel() { clearFlag(F_DEL); } 
	Message *reply() const;
	ULONG numAttch();
	long export(FILE *f);
	// MIME-specific
	Attachment *getAttch(ULONG);
	
/////////////////////////////////////////////////////////////////
// Functions for composing a message
/////////////////////////////////////////////////////////////////
	void setText(CCHAR);
	void setTo(CCHAR);
	void setFrom(CCHAR);
	void setCc(CCHAR);
	void setBcc(CCHAR);
	void setSubject(CCHAR);
	void setCharset(CCHAR);
	long sendSMTP(CCHAR);
	long sendMTA(CCHAR);
	// MIME-specific
	void attach(Attachment *);

	// Debug
	void debug();
};

void initCC();
long createFolder(char *);
void setBodyParameter(BODY *, const char *, const char *);
void printe(ENVELOPE *);
void printa(ADDRESS *, char *);
void printb(BODY *);
#endif

// Folder class - provides a wrapper of c-client 
#include <stdio.h>
#include "mclass.h"

struct FileType {
	const char *ext;
	short type;
	const char *subtype;
	const char *charset;
	short encoding;
};

const struct FileType defaultFileType = 
	{ 0, 	TYPEAPPLICATION,	"OCTET-STREAM", 0, ENCBASE64 };

const struct FileType fInfo[] = {
	{ "C",	TYPETEXT,	"PLAIN",	"US-ASCII",	ENCBASE64 },
	{ "Z",	TYPEAPPLICATION,	"OCTET-STREAM",	0,	ENCBASE64 },
	{ "arj",	TYPEAPPLICATION,	"OCTET-STREAM",	0,	ENCBASE64 },
	{ "au",	TYPEAUDIO,	0,	0,	ENCBASE64 },
	{ "avi",	TYPEVIDEO,	"AVI",	0,	ENCBASE64 },
	{ "c",	TYPETEXT,	"PLAIN",	"US-ASCII",	ENCBASE64 },
	{ "cpp",	TYPETEXT,	"PLAIN",	"US-ASCII",	ENCBASE64 },
	{ "cxx",	TYPETEXT,	"PLAIN",	"US-ASCII",	ENCBASE64 },
	{ "eps",	TYPEAPPLICATION,	"PostScript",	0,	ENCBASE64 },
	{ "gif",	TYPEIMAGE,	"GIF",	0,	ENCBASE64 },
	{ "gz",	TYPEAPPLICATION,	"OCTET-STREAM",	0,	ENCBASE64 },
	{ "h",	TYPETEXT,	"PLAIN",	"US-ASCII",	ENCBASE64 },
	{ "jpg",	TYPEIMAGE,	"JPEG",	0,	ENCBASE64 },
	{ "mov",	TYPEVIDEO,	"QUICKTIME",	0,	ENCBASE64 },
	{ "mpg",	TYPEVIDEO,	"MPEG",	0,	ENCBASE64 },
	{ "pcx",	TYPEIMAGE,	"PCX",	0,	ENCBASE64 },
	{ "png",	TYPEIMAGE,	"PNG",	0,	ENCBASE64 },
	{ "ps",	TYPEAPPLICATION,	"PostScript",	0,	ENCBASE64 },
	{ "sh",	TYPETEXT,	"PLAIN",	"US-ASCII",	ENCBASE64 },
	{ "tar.gz",	TYPEAPPLICATION,	"OCTET-STREAM",	0,	ENCBASE64 },
	{ "tgz",	TYPEAPPLICATION,	"OCTET-STREAM",	0,	ENCBASE64 },
	{ "tiff",	TYPEIMAGE,	"TIFF",	0,	ENCBASE64 },
	{ "txt",	TYPETEXT,	"PLAIN",	0,	ENCBASE64 },
	{ "z",	TYPEAPPLICATION,	"OCTET-STREAM",	0,	ENCBASE64 },
	{ "zip",	TYPEAPPLICATION,	"OCTET-STREAM",	0,	ENCBASE64 }
};

#define NUMTYPE	25


/* This must be called before any attempt to access the wrapper class! */
void initCC() 
{
#include <linkage.c>
}

long createFolder(char *mbox)
{
	return mail_create(0, mbox);
}

/* Folder::Folder(char *mbox)
{
}*/

long Folder::open(const char *s)
{
	char file[L_FILE];

	strcpy(file, s);
    return (stream = mail_open(0, file, NIL))? T : NIL;
}

long Folder::create(const char *mbox)
{
	char file[L_FILE];

	strcpy(file, mbox);
	if (mail_create(0, file))
		if (open(file))
			return T;
	return NIL;
}

Folder::~Folder()
{
	if (stream)
		close(0);
}

void Folder::close(long ops)
{
	stream = mail_close_full(stream, ops);
}

long Folder::status(long ops) 
{
	return mail_status(stream, MBOX(stream), ops);
}

Message *Folder::getMsg(ULONG msgno)
{
	if (msgno < 1 || msgno > stream->nmsgs)
		return 0;
	return new Message(stream, msgno);
}

// moves a local message to a remote mailbox
/*long Folder::moveDiff(char *mbox, ULONG msgno)
{
	long v;
	char *a;
	char *b;
	char *msg;
	STRING s;

	a = strdup(fetchHeader(msgno));
	b = strdup(fetchText(msgno));
	msg = (char *)malloc(strlen(a) + strlen(b) + 1);
	strcat(msg, a);
	strcat(msg, b);
	mail_string_init(&s, msg, strlen(msg));
	mail_create(0, mbox);
	v = mail_append(0, mbox, &s);
	free(a);
    free(b);
	free(msg);
	return 1;
}*/

long Folder::rename(const char *mbox)
{
	char file[L_FILE];
	
	strcpy(file, mbox);
	return mail_rename(stream, MBOX(stream), file);
}

long Folder::remove()
{
	return mail_delete(stream, MBOX(stream));
}
	
long Folder::putMsg(Message *msg)
{
	STRING bs;
	ULONG len;
	char *b;
	long rv;

	printf("putMsg()\n");
	if (!(b = msg->toSTRING(&len))) {
		printf("toSTRING failed\n");
		return NIL;
	}
	//printf("%.*s\n\nLength = %ld",(int)len, b, (int)len);
	INIT(&bs, mail_string, (void *)b, len);
	rv = mail_append_full(stream, MBOX(stream), F_SEEN, 0, &bs);
	fs_give((void **)&b);
	return rv;
} 
	

Message::Message(MAILSTREAM *s, ULONG msgnb) 
{
	mode = M_READONLY;
	msgno = msgnb;
	ms = s;
	env = mail_fetchstructure(ms, msgnb, &body);
}

// Use only when composing a new message
Message::Message()
{
	msgno = 0;
	mode = M_WRITABLE;
	body = mail_newbody();
	body->nested.part = 0;
	env = mail_newenvelope();
	setDate();
}

Message::~Message()
{
	if (mode == M_WRITABLE) {
		if (body)
			mail_free_body(&body);
		if (env)
			mail_free_envelope(&env);
	}
}

const char *Message::getText(ULONG *len) const
{
	if (!len)
		return 0;
	switch(body->type) {
	case TYPETEXT:
		return (const char *)mail_fetchtext_full(ms, msgno, len, 0);
		break;
	case TYPEMULTIPART:
		return (const char *)mail_fetchbody(ms, msgno, "1", len);
		break;
	}
	return 0;
}

const char *Message::getHeader() const
{
	return (const char *)mail_fetchheader(ms, msgno);
}

void Message::getDate(char *date) const
{
	MESSAGECACHE *m = mail_elt(ms, msgno);

	sprintf(date, "%d/%d %d:%d", m->day, m->month, m->hours, m->minutes);
}

void Message::getFrom(char *from) const
{
	mail_fetchfrom(from, ms, msgno, L_FROM);
}

void Message::getSubject(char *subj) const
{
    mail_fetchsubject(subj, ms, msgno, L_SUBJECT);
}
char Message::getFlag() const
{
	//mail_fetchstructure(ms, msgno, 0);
	MESSAGECACHE *mc = mail_elt(ms, msgno);

/* Highest to lowest priority to be shown */
	if (mc->deleted)
		return 'D';
	if (mc->answered)
        return 'A';
	if (mc->recent)
		return 'N';
	if (!mc->seen)
		return 'U';

	// Seen
	return ' ';
}

void Message::setFlag(char *flag)
{
	char seq[10];	// should be more than enuff

	sprintf(seq, "%ld", msgno);
	mail_setflag(ms, seq, flag);
}

void Message::clearFlag(char *flag)
{
	char seq[10];

	sprintf(seq, "%ld", msgno);
	mail_clearflag(ms, seq, flag);
}   

void Message::setDate() 
{
	env->date = (char *)fs_get(80);
	rfc822_date(env->date);
}

void Message::setTo(const char *to)
{
	char *tmp = (char *)fs_get(strlen(to) + 1);
	strcpy(tmp, to);
	rfc822_parse_adrlist(&(env->to), tmp, "localhost"); 
	fs_give((void **)&tmp);
}

void Message::setFrom(const char *from) 
{
	char *tmp;
	if (!from) {
		env->from = mail_newaddr();
    	env->from->personal = cpystr(getName());
    	env->from->mailbox = cpystr(getlogin());
    	env->from->host = cpystr(getHostName());
		env->return_path = mail_newaddr();
    	env->return_path->personal = cpystr(getName());
    	env->return_path->mailbox = cpystr(getlogin());
    	env->return_path->host = cpystr(getHostName());
	} else {
		tmp = (char *)fs_get(strlen(from) + 1);
		strcpy(tmp, from);
		rfc822_parse_adrlist(&(env->from), tmp, "localhost");
		rfc822_parse_adrlist(&(env->return_path), tmp, "localhost");
		fs_give((void **)&tmp);
	}
}

void Message::setCc(const char *cc) 
{
	if (!cc)
		return;
	char *tmp = (char *)fs_get(strlen(cc) + 1);
    strcpy(tmp, cc);
	rfc822_parse_adrlist(&(env->cc), tmp, "localhost");
	fs_give((void **)&tmp);
}
	
void Message::setBcc(const char *bcc)
{
	if (!bcc)
		return;
	char *tmp = (char *)fs_get(strlen(bcc) + 1);
    strcpy(tmp, bcc);
	rfc822_parse_adrlist(&(env->bcc), tmp, "localhost");
	fs_give((void **)&tmp);
}

void Message::setSubject(const char *subj) 
{
	env->subject = cpystr(subj);
}

// Use this only when the message is *NON-MULTIPART TEXT* message
void Message::setText(const char *text)
{
	BODY *b;

	if (body->type != TYPEMULTIPART) 
		b = body;	 
	else 	// Go to Part I
		b = &body->nested.part->body;
	b->type = TYPETEXT;
   	b->contents.text.data = cpystr(text);
   	b->contents.text.size = strlen(text);
}

long Message::sendSMTP(const char *h)
{
	SENDSTREAM *ss;
	char *host = cpystr(h);
	char *list[] = { host, 0 };

	if (!(ss = smtp_open(list, T)))
		return NIL;
	if (smtp_mail(ss, "MAIL", env, body)) {
		smtp_close(ss);
		return T;
	}
	smtp_close(ss);
	return NIL;
}

long Message::sendMTA(const char *m)
{                                    
	long rv = NIL;
	FILE *f = popen(m, "w");
	if (export(f))
		rv = T;
	pclose(f);
	return rv;
}

static long dump(void *stream, char *s)	
{
	FILE *f = (FILE *)stream;
	return (fprintf(f, s) < 0)? NIL : T;
}

long Message::export(FILE *f)
{
	char t[5000];

	return f? rfc822_output(t, env, body, dump, f, 0) : NIL;
}

char *Message::toSTRING(ULONG *len)
{
	char t[5000];
	char *tmpfile;
	FILE *tmp;
	ULONG size;
	char *s;

	// use tmp file as buffer
	tmpfile = tempnam(0, "KMAIL");
	printf("Tmpfile %s\n", tmpfile);
	if (!(tmp = fopen(tmpfile, "w+b"))) {
		printf("Can't open tmp file\n");
		return NIL;
	}
	chmod(tmpfile, S_IRUSR | S_IWUSR);
	switch(mode) {
	case M_WRITABLE:
		if (!rfc822_output(t, env, body, dump, tmp, 0)) {
			printf("rfc822_output failed\n");
			return NIL;
		}
		break;
	case M_READONLY:
		char *h = mail_fetchheader_full(ms, msgno, 0, &size, FT_PREFETCHTEXT);
		fwrite((void *)h, 1, size, tmp);
		h = mail_fetchtext_full(ms, msgno, &size, 0);
		fwrite((void *)h, 1, size, tmp);
		break;
	}
	*len = size = ftell(tmp);
	rewind(tmp);
	s = (char *)fs_get(size);
	if (size != fread((void *)s, 1, size, tmp)) {
		printf("Fread failed\n");
		*len = 0;
		fs_give((void **)&s);
	}
	fclose(tmp);
	unlink(tmpfile);
	free(tmpfile);
	return s;
}

// This function returns the number of attachments
// (excluding the normal text part).
// For non-multipart message the number returned is always 0
ULONG Message::numAttch()
{
	ULONG n = 0;
    PART *next;

    if (body->type != TYPEMULTIPART)
        return n;
	next = body->nested.part;
    /* traverse the list */
    while (next = next->next)
        n++;
    return n;
}

BODY *Message::getBodyPart(ULONG ptno)
{
	if (body->type != TYPEMULTIPART)
		return 0;

    PART *next = body->nested.part;
    ULONG ctr = 1;

    while (ctr < ptno && next) {
        next = next->next;
        ctr++;
    }
    if (ctr == ptno) 
		return &(next->body);
	return 0;	// incorrect body part
}

#if 0
Attachment *Message::getAttachment(ULONG ptno)
{
	BODY *ptr;
	Attachment *a;
	PARAMETER *p;
	char sec[10];
	char *txt;
	ULONG len;

	if (ptr = getBodyPart(ptno + 1))
		a = new Attachment;
	else
		return 0;
	// Copy info
	a->setType(ptr->type)
	a->setDEscription((const char *)ptr->description);
	a->setSubtype((const char *)ptr->subtype);
	a->setEncoding(ptr->encoding);
	// Get various attributes
	p = ptr->parameter;
	while (p) 
	{
		char *tmp = cpystr(p->attribute);
		if (!strcmp("NAME", ucase(tmp))) 
			a->setFilename((const char *)p->value);
		else if (!strcmp("CHARSET", ucase(tmp))) 
			a->setCharset((const char *)p->value);
		fs_give((void **)&tmp);
		p = p->next;
	}
	// Decode the data
	sprintf(sec, "%ld", ptno);
	txt = mail_fetchbody(ms, msgno, sec, &len);
	switch(ptr->encoding) {
	case ENCBASE64:
		a->setData(rfc822_base64((unsigned char *)txt, len, &len));
		break;
	case ENCQUOTEDPRINTABLE:
		a->setData((void *)rfc822_qprint((unsigned char *)txt, len, &len));
		break;
	default:	// Copy the entire thing
		void *d = fs_get(len + 1);
		memcpy(d, txt, len + 1);
		a->setData(d); 
		break; 	// Can't do anything
	}
	a->length = len;
	return a;
}
		
void setBodyParameter(Body *b, char *att, char *value)
{
	PARAMETER *p;

}

void Message::attach(Attachment *a)
{
	PART *pp;
	BODY *b;
	PARAMETER *pa;

	if (!a)
		return;
	body->type = TYPEMULTIPART;
	// If text part not present create it 
	if (!body->nested.part) 
		pp = body->nested.part = mail_newbody_part();
	// Now skip to the tail of the linked list
	while (pp->next)
		pp = pp->next;
	b = &pp->body;
	b->type = a->type;
	b->encoding = a->encoding;
	b->subtype = a->subtype;
	b->id = 0;	// Nothing right now
	b->description = a->description;
	// set up the parameters
	if (a->charset) {
		pa = b->parameter = mail_newbody_parameter();
		pa->attribute = strdup("CHARSET")
		pa->value = a->charset;
		pa = pa->next;
	}
	if (a->filename) {
		if (a->charset)
			pa = b->parameter-
        pa = mail_newbody_parameter();
        pa->attribute = strdup("NAME")
        pa->value = a->charset;
        pa = pa->next;
    }
	

void printaddr(ADDRESS *a)
{
	printf("Personal: %s\n", a->personal);
	printf("Mailbox: %s\n", a->mailbox);
	printf("Host: %s\n", a->host);
	printf("Error: %s\n", a->error);
}

// For debug purpose only, dump the text to stdout
void Message::debug() 
{
	ADDRESS *next;

	printf("From:\n");
	next = env->from;
	while (next) 
	{
		printaddr(next);
		next = next->next;
	}
	
	printf("\nTo:\n");
    next = env->to;
    while (next)
    {
        printaddr(next);
        next = next->next;
    }

	printf("\nCc:\n");
    next = env->cc;
    while (next)
    {
        printaddr(next);
        next = next->next;
    }

	printf("\nBcc:\n");
    next = env->bcc;
    while (next)
    {
        printaddr(next);
        next = next->next;
    }

	printf("Date: %s\n", env->date);
	printf("Subject: %s\n\n", env->subject);
	printf("%s\n",body->contents.text.data);
}

Attachment::Attachment()
{
	// Set up some default values
	type = TYPEAPPLICATION;
	subtype = 0;
	description = 0;
	filename = 0;
	charset = 0;
	encoding = ENCBASE64;
	length = 0;
}

Attachment::~Attachment()
{
	/*if (filename)
		fs_give(&filename);
	if (description)
		fs_give(&description);
	if (subtype)
		fs_give(&subtype);
	if (charset)
		fs_give(&charset);
	if (!data)
		fs_give(&data);*/
}

static int cmp(const void *key, const void *data)
{
	struct FileType *t = (struct FileType *)data;
	return strcmp((CCHAR)key, t->ext);
}

// It tries to guess the type of the file by its extension, and setup the
// appropriate data structure
long Attachment::guess(CCHAR filename) 
{
	char *ext;
	const char *b;
	struct FileType *f;
	short t;

	b = basename(filename)
	t = fileType(filename);
	if (t == BADFILE)
		return NIL;
	ext = strrchr(b, '.');
	ext++;
	f = (strcut FileType *)bsearch(ext, fInfo, NUMTYPE, sizeof(FileType), cmp);
	// Now setup the data of the attachment
	if (!f) 
		f = &defaultFileType;
	setFilename(b);
	setType(f->type);
	setSubtype(f->subtype);
	setEncoding(f->encoding);
	// doesn't matter if the data type is data or binary
	// C-client will take care of the encoding
	data = readFile(filename, &length);
	return data? T : NIL;
}

// Save the attachment to file 
// If file == 0 use to ./filename
long Attachment::save(CCHAR *file)
{
	return file? writeFile(file, data, length) :
}
#endif

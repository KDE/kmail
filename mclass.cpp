// Folder class - provides a wrapper of c-client 
#include <stdio.h>
#include "mclass.h"

static const char *enctable[] = ENCODING;
static const char *bodytype[] = BODYTYPE;

struct FileType {
	const char *ext;
	short type;
	const char *subtype;
	const char *charset;
};

const struct FileType defaultTextFileType =
{ 0,	TYPETEXT,	"PLAIN",	"US-ASCII" };

const struct FileType defaultBinFileType =
{ 0,	TYPEAPPLICATION,	"OCTET-STREAM",	0 };

const struct FileType fInfo[] = {
	{ "Z",	TYPEAPPLICATION,	"X-COMPRESS",	0 },
	{ "ai",	TYPEAPPLICATION,	"PostScript",	0 },
	{ "aif",	TYPEAUDIO,	"X-AIFF",	0 },
	{ "aifc",	TYPEAUDIO,	"X-AIFF",	0 },
	{ "aiff",	TYPEAUDIO,	"X-AIFF",	0 },
	{ "au",	TYPEAUDIO,	"BASIC",	0 },
	{ "avi",	TYPEVIDEO,	"X-MSVIDEO",	0 },
	{ "cpio",	TYPEAPPLICATION,	"X-CPIO",	0 },
	{ "csh",	TYPEAPPLICATION,	"X-CSH",	0 },
	{ "dvi",	TYPEAPPLICATION,	"X-DVI",	0 },
	{ "eps",	TYPEAPPLICATION,	"POSTSCRIPT",	0 },
	{ "fif",	TYPEAPPLICATION,	"FRACTALS",	0 },
	{ "gif",	TYPEIMAGE,	"GIF",	0 },
	{ "gtar",	TYPEAPPLICATION,	"X-GTAR",	0 },
	{ "gz",	TYPEAPPLICATION,	"X-GZIP",	0 },
	{ "htm",	TYPETEXT,	"HTML",	"US-ASCII" },
	{ "html",	TYPETEXT,	"HTML",	"US-ASCII" },
	{ "ief",	TYPEIMAGE,	"IEF",	0 },
	{ "jpg",	TYPEIMAGE,	"JPEG",	0 },
	{ "latex",	TYPEAPPLICATION,	"X-LATEX",	0 },
	{ "man",	TYPEAPPLICATION,	"X-TROFF-MAN",	0 },
	{ "me",	TYPEAPPLICATION,	"X-TROFF-ME",	0 },
	{ "mov",	TYPEVIDEO,	"QUICKTIME",	0 },
	{ "movie",	TYPEVIDEO,	"X-SGI-MOVIE",	0 },
	{ "mpe",	TYPEVIDEO,	"MPEG",	0 },
	{ "mpeg",	TYPEVIDEO,	"MPEG",	0 },
	{ "mpg",	TYPEVIDEO,	"MPEG",	0 },
	{ "ms",	TYPEAPPLICATION,	"X-TROFF-MS",	0 },
	{ "pbm",	TYPEIMAGE,	"X-PORTABLE-BITMAP",	0 },
	{ "pgm",	TYPEIMAGE,	"X-PORTABLE-GRAYMAP",	0 },
	{ "png",	TYPEIMAGE,	"PNG",	0 },
	{ "pnm",	TYPEIMAGE,	"X-PORTABLE-ANYMAP",	0 },
	{ "ppm",	TYPEIMAGE,	"X-PORTABLE-PIXMAP",	0 },
	{ "ps",	TYPEAPPLICATION,	"POSTSCRIPT",	0 },
	{ "qt",	TYPEVIDEO,	"QUCIKTIME",	0 },
	{ "ras",	TYPEIMAGE,	"X-CMU-RASTER",	0 },
	{ "rft",	TYPEAPPLICATION,	"X-RTF",	0 },
	{ "rgb",	TYPEIMAGE,	"X-RGB",	0 },
	{ "sgml",	TYPETEXT,	"SGML",	"US-ASCII" },
	{ "sh",	TYPEAPPLICATION,	"X-SH",	0 },
	{ "shar",	TYPEAPPLICATION,	"X-SHAR",	0 },
	{ "snd",	TYPEAUDIO,	"BASIC",	0 },
	{ "t",	TYPEAPPLICATION,	"X-TROFF",	0 },
	{ "tar",	TYPEAPPLICATION,	"X-TAR",	0 },
	{ "tcl",	TYPEAPPLICATION,	"X-TCL",	0 },
	{ "tex",	TYPEAPPLICATION,	"X-TEX",	0 },
	{ "texi",	TYPEAPPLICATION,	"X-TEXINFO",	0 },
	{ "texinfo",	TYPEAPPLICATION,	"X-TEXINFO",	0 },
	{ "text",	TYPETEXT,	"PLAIN",	"US-ASCII" },
	{ "tif",	TYPEIMAGE,	"TIFF",	0 },
	{ "tiff",	TYPEIMAGE,	"TIFF",	0 },
	{ "tr",	TYPEAPPLICATION,	"X-TROFF",	0 },
	{ "troff",	TYPEAPPLICATION,	"X-TROFF",	0 },
	{ "txt",	TYPETEXT,	"PLAIN",	"US-ASCII" },
	{ "vrml",	TYPEMODEL,	"VRML",	0 },
	{ "wav",	TYPEAUDIO,	"X-WAV",	0 },
	{ "xbm",	TYPEIMAGE,	"X-XBITMAP",	0 },
	{ "xpm",	TYPEIMAGE,	"X-XPIXMAP",	0 },
	{ "xwd",	TYPEIMAGE,	"X-XWINDOWDUMP",	0 },
	{ "zip",	TYPEAPPLICATION,	"X-ZIP-COMPRESSED",	0 }
};

#define NUMTYPES      60

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

	a = cpystr(fetchHeader(msgno));
	b = cpystr(fetchText(msgno));
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

Message *Message::reply() const {
	Message *m = new Message;

	m->setInReplyTo(env->message_id);
	return m;
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

void Message::getAddress(ADDRESS *ad, char *buf) const
{
	char tmp[100];
	char email[70];
	ADDRESS *a = ad;

	buf[0] = '\0';
	while (a) {
		if (a->host)
			sprintf(email, "%s@%s", a->mailbox, a->host);
		else
			sprintf(email, "%s", a->mailbox);
		if (a->personal) 
        	sprintf(tmp, "%s <%s>, ", a->personal, email);
		else
			sprintf(tmp, "%s, ", email);
		strcat(buf, tmp);
		a = a->next;
	}
	/* To get rid of the last "," */
	if (strlen(buf)) 
		buf[strlen(buf) - 2] = '\0';
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
	b->encoding = ENC7BIT;
   	b->contents.text.data = cpystr(text);
   	b->contents.text.size = strlen(text);
}

void Message::setCharset(const char *charset)
{
    BODY *b;

    if (body->type != TYPEMULTIPART)
        b = body;
    else    // Go to Part I
        b = &body->nested.part->body;
	setBodyParameter(b, "CHARSET", charset);
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
	ULONG n = 0L;
    PART *next;

    if (body->type != TYPEMULTIPART)
        return 0L;
	next = body->nested.part;
    /* traverse the list */
    while (next) {
		next = next->next;
        n++;
	}
	// -1 because the first test part is excluded
    return n - 1;
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
		return &next->body;
	return 0;	// incorrect body part
}

Attachment *Message::getAttch(ULONG ptno)
{
	BODY *ptr;
	Attachment *a;
	PARAMETER *p;
	char sec[10];
	char *txt;
	ULONG len;

	//printf("Get Body\n");
	if ((ptr = getBodyPart(ptno + 1)))
		a = new Attachment;
	else
		return 0;
	// Copy info
	//printf("Copy Info\n");
	//printf("Set type\n");
	a->setType(ptr->type);
	//printf("Set Desc\n");
	a->setDescription((const char *)ptr->description);
	//printf("Set Subtype\n");
	if (ptr->subtype)
	a->setSubtype((const char *)ptr->subtype);
	//printf("Set Encoding\n");
	a->setEncoding(ptr->encoding);
	// Get various attributes
	p = ptr->parameter;
	mdebug("Set para\n");
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
	mdebug("Decoding...\n");
	sprintf(sec, "%ld", ptno + 1);
	txt = mail_fetchbody(ms, msgno, sec, &len);
	switch(ptr->encoding) {
	void *da;
	case ENCBASE64:
		da = rfc822_base64((unsigned char *)txt, len, &len);
		a->setData(da, len);
		break;
	case ENCQUOTEDPRINTABLE:
		da = rfc822_qprint((unsigned char *)txt, len, &len);
		a->setData(da, len);
		break;
	default:	// Can't do anything
		break;
	}
	return a;
}
		
void setBodyParameter(BODY *b, const char *att, const char *value)
{
	PARAMETER *p = b->parameter;

	if (!b->parameter)
		p = b->parameter = mail_newbody_parameter();
	while (p->next) 
		p = p->next;
	p->attribute = cpystr(att);
	p->value = cpystr(value);
}

void Message::attach(Attachment *a)
{
	PART *part;
	BODY *b;

	if (!a)
		return;
	body->type = TYPEMULTIPART;
	body->subtype = cpystr("MIXED");
	// If text part not present create it 
	if (!body->nested.part) 
		part = body->nested.part = mail_newbody_part();
	// Now skip to the tail of the linked list
	while (part->next)
		part = part->next;
	// Create this part
	part->next = mail_newbody_part();
	b = &part->next->body;
	b->type = a->type;
	b->encoding = a->encoding;
	b->subtype = cpystr(a->subtype);
	b->id = 0;	// Nothing right now
	b->description = cpystr(a->description);
	// set up the parameters
	if (a->charset) 
		setBodyParameter(b, "CHARSET", a->charset);
	if (a->filename) 
		setBodyParameter(b, "NAME", a->filename);
	b->contents.text.data = (char *)a->data;
	b->contents.text.size = a->length;

	// Set to NULL to avoid freeing the memory twice.
	a->data = 0;
}
	
void printa(ADDRESS *a, char *title)
{
	printf("------------- ADDRESS %s--------------\n", title);
	ADDRESS *next = a;
	while (next) {
		printf("Personal: <%s> ", next->personal);
		printf("Mailbox: <%s> ", next->mailbox);
		printf("Host: <%s> ", next->host);
		printf("Error: <%s>\n", next->error);
		next = next->next;
	}
}

void printb(BODY *b)
{
    PARAMETER *p;
	printf("------------- BODY --------------\n");
    printf("Body Type: %s\n", bodytype[b->type]);
    printf("Encoding: %s\n", enctable[b->encoding]);
    printf("Subtype: %s\n", b->subtype);
    printf("Description: %s\n", b->description);
    printf("Body Content ID: %s\n", b->id);
    p = b->parameter;
    while (p)
    {
        printf("\tAttribute: %s\n", p->attribute);
        printf("\tValue: %s\n", p->value);
        p = p->next;
    }
}

void printe(ENVELOPE *e)
{
	printf("-----------------ENVELOPE---------------------\n");
	printa(e->return_path, "Return Path");
	printa(e->from, "From");
	printa(e->sender, "Sender");
	printa(e->reply_to, "Reply-To");
	printa(e->to, "To");
	printa(e->cc, "Cc");
	printa(e->bcc, "Bcc");
	printf("Remail: %s\n", e->remail);
	printf("Date: %s\n", e->date);
	printf("Subject: %s\n", e->subject);
	printf("In Reply To ID: %s\n", e->in_reply_to);
	printf("Message ID: %s\n", e->message_id);
	printf("Newsgroups: %s\n", e->newsgroups);
	printf("Follow-up to: %s\n", e->followup_to);
	printf("References: %s\n", e->references);
}

void Message::debug()
{
	PART *next = body->nested.part;
	printf("==================================================\n");
	printe(env);
	printb(body);
	while (next) {
		printb(&next->body);
		next = next->next;
	}
	printf("==================================================\n");
}

Attachment::Attachment()
{
	// Set up some default values
	type = TYPEAPPLICATION;
	subtype = cpystr("OCTET-STREAM");
	description = 0;
	filename = 0;
	charset = 0;
	encoding = ENCBASE64;
	length = (ulong)0;
	data = 0;
}

Attachment::~Attachment()
{
	if (filename)
		fs_give((void **)&filename);
	if (description)
		fs_give((void **)&description);
	if (subtype)
		fs_give((void **)&subtype);
	if (charset)
		fs_give((void **)&charset);
	if (data)
		fs_give((void **)&data);
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
	char ext2[10];
	const char *b;
	const struct FileType *f = 0;
	short t;
	
	t = fileType(filename);
	if (!t)
		return NIL;
	if (!(b = basename(filename)))
		return NIL;
	mdebug("Basename = <%s>\n", b);
	ext = strrchr(b, '.');
	mdebug("Extension = <%s>\n", ext);
	if (ext++) {
		strcpy(ext2, ext);
		f = (struct FileType *)bsearch(lcase(ext2), fInfo, NUMTYPES, sizeof(FileType), cmp);
	}
	// Unknown extension or no extension at all
	if (!f) {
		mdebug("<<<<<<Unknown/No extension>>>>>>>>\n");
		switch(t) {
		case TEXT:
			f = &defaultTextFileType;
			break;
		case BINARY:
			f = &defaultBinFileType;
			break;
		case BADFILE:
			return NIL;
			break;
		}
	}
	setFilename(b);
	setType(f->type);
	setSubtype(f->subtype);
	setCharset(f->charset);
	// Since this is an attachment we treat everything as binary
	/*if (f->charset)	
		setEncoding(ENC7BIT);
	else*/
	setEncoding(ENCBINARY);
	// doesn't matter if the data type is data or binary
	// C-client will take care of the encoding
	data = readFile(filename, &length);
	mdebug("Size of data: %ld\n", length);
	return data? T : NIL;
}


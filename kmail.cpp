//
//  This file seems to be old and is no longer used !
//  main.cpp contains now the main code.
//
//  --Stefan <taferner@alpin.or.at>
//

#ifdef OBSOLETE_CODE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mclass.h"
#include "util.h"

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define	FALSE	0
#endif

// Command code
#define CNULL	0
#define READ	1
#define	DELETE	2
#define UNDELETE 3
#define MOVE	4
#define HELP	-1
#define QUIT	-2
#define	HEADER	-3
#define SEND	-4

#define L_LINE	80

#define MAILBOX 	"/var/spool/mail/lynx"
#define MAILBOX1	"/home/lynx/Mail/remote"
#define SMTPHOST 	"uxmail.ust.hk"
#define MENU \
"usage: <command> [msgno]\n"\
"commands: [R]ead, [D]elete, [U]ndelete, [Q]uit, [H]eaders, [S]end,\n"\
"          [M]ove, [?] for Help\n"

const char *list[] = { "localhost", 0 };

int command(char *ppt, unsigned long *msgno)
{
	char *ans = (char *)malloc(10);

	printf(ppt);
	gets(ans);
	switch(ans[0]) 
	{
		case 'r':
			ans++;
			*msgno = atol(ans);
			return READ;
			break;
		case 'd':
			ans++;
			*msgno = atol(ans);
            return DELETE;
            break;
		case 'u':
            ans++;
            *msgno = atol(ans);
            return UNDELETE;
            break;
		case '?':
			return HELP;
			break;
		case 'h':
			return HEADER;
			break;
		case 's':
			return SEND;
			break;
		case 'm':
			ans++;
            *msgno = atol(ans);
			return MOVE;
			break;
        case 'q':
            return QUIT;
            break;
		case '\0':
            return CNULL;
            break;
		default:
			return -9999;
			break;
	}
}	

void compose(ENVELOPE *env, BODY *body)
{
	char line[L_LINE + 1];
	char hostname[30];
	char *text = (char *)malloc(8 * L_LINE + 1);

	gethostname(hostname, 30);
	printf("From: %s <%s@%s>\n", getGecos(), getlogin(), hostname);
	env->from = mail_newaddr();
	env->from->personal = strdup(getGecos());
	env->from->mailbox = strdup(getlogin());
	env->from->host = strdup(hostname);
	env->return_path = mail_newaddr();
    env->return_path->personal = strdup(getGecos());
    env->return_path->mailbox = strdup(getlogin());
    env->return_path->host = strdup(hostname);
	printf("To: ");
	gets(line);
	rfc822_parse_adrlist(&(env->to), line, hostname);
	printf("Subject: ");
	gets(line);
	env->subject = strdup(line);
	printf("Cc: ");
	gets(line);
	rfc822_parse_adrlist(&(env->cc), line, hostname);
	puts("Enter your message, . to end)");
	gets(line);
	while(!(line[0] == '.' && line[1] == '\0')) 
	{
		strcat(text, "\n");
		strcat(text, line);
		gets(line);
	}
	puts(text);
	rfc822_date(line);
	body->type = TYPETEXT;
	body->contents.text.data = text;
	body->contents.text.size = strlen(text);
    env->date = strdup(line);
}


void main() 
{
	startCC();

	Folder f(MAILBOX);
	ENVELOPE *env;
	BODY *body;
	int quit = FALSE;
	int cmd;
	unsigned long msgno;
	SENDSTREAM *ss;

	f.status();
	printf(MENU);
	while (!quit)
	{
		cmd = command("kmail >", &msgno);
		if (!f.isValid(msgno) && cmd > 0) 
		{
			printf("Bad Message Number\n");
			continue;
		}
		switch (cmd)
		{
			case READ:
				//printf("Date: %s\n", f.fetchDate(msgno));
				//printf("From: %s\n", f.fetchFrom(msgno));
				//printf("Subject: %s\n\n", f.fetchSubject(msgno));
				printf("%s", f.fetchText(msgno));
				break;
			case DELETE:
				f.delMsg(msgno);
				break;
			case UNDELETE:
                f.undelMsg(msgno);
                break;
			case QUIT:
				quit = TRUE;
				break;
			case HELP:
				printf(MENU);
				break;
			case HEADER:
				unsigned long int i;

				for (i = 1; i <= f.numMsg(); i++)
					printf("%ld %c %s %s %s\n", i, f.getFlag(i), f.fetchFrom(i), f.fetchDate(i), f.fetchSubject(i));
				break;
			case SEND:
				env = mail_newenvelope();
				body = mail_newbody();
				compose(env, body);
				if (ss = smtp_open(list, 1)) 
				{
					if (smtp_mail(ss, "MAIL", env, body)) 
						puts("OK");
					smtp_close(ss);
				}
				else
					puts("FAILED");
				mail_free_envelope(&env);
				mail_free_body(&body);
				break;
			case MOVE:
				f.moveDiff(MAILBOX1, msgno);
				break;
			case -9999:
				puts("Bad Command");
				break;
		}
	}
	f.close(CL_EXPUNGE);
}

#endif /* OBSOLETE_CODE */

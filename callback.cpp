//	This file contains all the callbacks required by C-Client 

#include <stdio.h>
#include "mclass.h"

void mm_flags(MAILSTREAM *stream, ULONG msgno)
{
	fprintf(stderr, "mm_flags(): %s Message #%ld\n", MBOX(stream), msgno);
}

void mm_status(MAILSTREAM *, char *mailbox, MAILSTATUS *status)
{
	fprintf(stderr, "mm_status(): %s ", mailbox);
	if (SA_MESSAGES & (status->flags))
		fprintf(stderr, "%ld messages ", status->messages);
	if (SA_RECENT & (status->flags))
		fprintf(stderr, "%ld recent ", status->recent);
	if (SA_UNSEEN & (status->flags))
        fprintf(stderr, "%ld unseen ", status->unseen);
	if (SA_UIDNEXT & (status->flags))
        fprintf(stderr, "uidnext %ld ", status->uidnext);
	if (SA_UIDVALIDITY & (status->flags))
        fprintf(stderr, "uidvalidity %ld", status->uidvalidity);
	
	fprintf(stderr, "\n");
}

void mm_searched(MAILSTREAM *stream, ULONG number)
{
	fprintf(stderr, "mm_searched():	%s %ld messages match the search\n", MBOX(stream), number);
}

void mm_exists(MAILSTREAM *stream, ULONG number)
{
	fprintf(stderr, "mm_exists(): %s %ld messages exist\n", MBOX(stream), number);
}

void mm_expunged(MAILSTREAM *stream, ULONG number)
{
	fprintf(stderr, "mm_expunged(): %s message #%ld expunged\n", MBOX(stream), number);
}

void mm_list(MAILSTREAM *, int, char *, long )
{
	// does nothing 
	fprintf(stderr, "mm_list(): \n");
}

void mm_lsub(MAILSTREAM *, int, char *, long)
{
    // does nothing
    fprintf(stderr, "mm_lsub(): \n");
}

void mm_log(char *string, long errflg)
{
	fprintf(stderr, "mm_log(): %s, Error level: ", string);
	switch(errflg) 
	{
		case NIL:
			fprintf(stderr, "NIL\n");
			break;
		case PARSE:
			fprintf(stderr, "PARSE\n");
		 	break;
		case WARN:
			fprintf(stderr, "WARN\n");
			break;
		case ERROR:
			fprintf(stderr, "ERROR\n");
			break;
		default:
			fprintf(stderr, "UNKNOWN!\n");
			break;
	}
}
			
void mm_notify(MAILSTREAM *stream, char *string, long errflg)
{
    fprintf(stderr, "mm_log(): %s %s, Error level: ", MBOX(stream), string);
    switch(errflg)
    {
        case NIL:
            fprintf(stderr, "NIL\n");
            break;
        case PARSE:
            fprintf(stderr, "PARSE\n");
            break;
        case WARN:
            fprintf(stderr, "WARN\n");
            break;
        case ERROR:
            fprintf(stderr, "ERROR\n");
            break;
        default:
            fprintf(stderr, "UNKNOWN!\n");
            break;
    }
}

void mm_dlog(char *string)
{
	fprintf(stderr, "mm_dlog(): %s\n", string);
}

void mm_login(NETMBX *mb, char *user, char *pwd, long trial)
{
	char line[80];

	printf("Host %s User %s MailBox %s Service %s", mb->host, mb->user,
			mb->mailbox, mb->service);
	printf("Trial %ld\n", trial);
	if (!strlen(user)) 
	{
		printf("User:");
		gets(line);
		strcpy(user, line);
	}
	printf("Password(will get echoed to the screen!!):");
	gets(line);
	strcpy(pwd, line);
}

void mm_critical(MAILSTREAM *stream)
{
	fprintf(stderr, "mm_critical(): %s About to enter critical region\n", MBOX(stream));
}

void mm_nocritical(MAILSTREAM *stream)
{
	fprintf(stderr, "mm_nocritical(): %s Left critical region\n", MBOX(stream));
}

long mm_diskerror(MAILSTREAM *stream, long errcode, long serious)
{
	fprintf(stderr, "mm_diskerror(): %s errcode %ld, serious %ld\n", MBOX(stream), errcode, serious);
	return NIL;
}

void mm_fatal(char *string)
{
	fprintf(stderr, "mm_fatal(): %s\n", string);
}



	

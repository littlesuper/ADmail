#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <string.h>
#include <openssl/md5.h>



//#define DEBUG
#define MAX_IGNORE_LIST 100 
#define MAX_IGNORE_LENGTH 1024 

#define SUBJECT_TXT "Oray Services" 
#define SENDMAIL_PIPE "sendmail -t"
#define SEARCH_KEY "%key%"
#define MD5LEN	16
#define MAILSTR	"email="
#define KEYSTR		"key="


const char * VERSION="1.0";
const char * BUGREPORT="littlesuper@gmail.com";
extern int optind;
 

static void md5_str (char * str, size_t size ,unsigned char *result,size_t len ,unsigned char data[2*len]){
	int i=0,j=0;
	char   ls_hex[3] = "";
	memset(ls_hex, '\0', 3);
	memset(data, '\0', 2*len);
	MD5((unsigned char *)str, size, result);
	for(i=0; i < len; i++)
	{
		sprintf(ls_hex,"%.2X", result[i]);
		data[j++] = ls_hex[0];
		data[j++] = ls_hex[1];
	}
		data[j]='\0';
}

static int cmp(char * str,char **list)
{
	int i,j,slen,llen;
	char *stail,*ltail;
	    if ( str == NULL && list==NULL )
        return 0;
	for(i=0;i<MAX_IGNORE_LIST && (*list[i]!='\0');i++)
	{
		slen=strlen(str);
		llen=strlen(list[i]);
		for(j=0,ltail=list[i]+llen-1,stail=str + slen-1;tolower(*ltail)==tolower(*stail);ltail--,stail--)
		{
			j++;
			if (j==llen)
			{
				return i+1;
			}
		}
	}
	return 0;
}

static void kill_space( char *str )

{
    char *head, *tail;
    if ( str == NULL )
        return;
    for( head = str; *head == ' ' || *head == '\t'; head ++ );
    for( tail = str + strlen(str) - 1; (*tail == ' ' || *tail == '\t' || *tail == '\n' ) && tail >= head; tail -- );
    while( head <= tail )
         *str ++ = *head ++;
    *str = 0;
}


static void
usage (void)
{
	fprintf(stderr, "Usage: sdmail [OPTION]...\n");
	fprintf(stderr, "Send mail using sendmail.\n\n");
	fprintf(stderr,
		"Supported options:\n"
		"  -u				Set Sender.\n"
		"  -l				Set To e-mail list.\n"
		"  -t				Email body for text part.\n"
		"  -m				Email body for html part.\n"
		"  -s				Set email subject.\n"
		"  -i				Set mailbox list will Ignore txt part.\n"
		"  -e				Change Email parameter values.\n"
		"  -k				MD5 key for mail address.\n"
		"  -h				Print this help information.\n"
		);
	fprintf(stderr, "Version %s. Report bugs to <%s>.\n",
		VERSION, BUGREPORT);
}
static char* open_t_buf(char * file)
{
  FILE * fp;
  long lSize;
  char * buf;
  size_t result;
   fp = fopen ( file , "rb" );
  if (fp==NULL) {fputs ("Open File error!\n",stderr); exit (1);}
  fseek (fp , 0 , SEEK_END);
  lSize = ftell (fp);
  rewind (fp);
  buf = (char*) malloc (sizeof(char)*lSize+1);
  if (buf == NULL) {fputs ("Malloc Memory error!\n",stderr); exit (2);}
  memset(buf,'\0',sizeof(char)*lSize+1);
  result = fread (buf,1,lSize,fp);
  if (result != lSize) {fputs ("Reading error!\n",stderr); exit (3);}
  buf[sizeof(char)*lSize]='\0';
  fclose (fp);
  return buf;
}
static char ** open_t_list(char * file)
{
FILE * fp;
int i=0;
char **buf=NULL;
char * line = NULL;
size_t len = 0;
ssize_t read;

fp = fopen ( file , "rb" );
if (fp==NULL) {fputs ("File error",stderr); exit (1);}

buf = (char**) malloc (sizeof(char*)*MAX_IGNORE_LIST);
if (buf == NULL) {fputs ("Malloc Memory error!\n",stderr); exit (2);}
memset(buf,'\0',sizeof(char*)*MAX_IGNORE_LIST);
		for(i=0;i<MAX_IGNORE_LIST;i++){
        buf[i] = (char*)malloc(sizeof(char)*MAX_IGNORE_LENGTH);  
		if (buf[i] == NULL) {fputs ("Malloc Memory error!\n",stderr); exit (2);}
		memset(buf[i],'\0',sizeof(char)*MAX_IGNORE_LENGTH);
		}
i=0;
while (((read=getline(&line, &len, fp))!= -1)&&(i<MAX_IGNORE_LIST)) {
		//if ( line[ strlen(line)-1 ] == '\n' ) line[ strlen(line)-1 ] = '\0';
		kill_space(line);
		if(!line[0])
        continue;


       /* buf[i] = (char*)malloc(sizeof(char)*read+1);  
		if (buf[i] == NULL) {fputs ("Malloc Memory error!\n",stderr); exit (2);}
		memset(buf[i],'\0',sizeof(char)*read+1);*/
		strcpy(buf[i],line);

		buf[i][sizeof(char)*read]='\0';
		i++;
			}
   if(line)
   free(line);
  fclose (fp);
  if(buf[0]==NULL){
	for (i=0; i<MAX_IGNORE_LIST && buf[i]; i++)   
	{	
		#ifdef DEBUG
		printf ("line %d\t\t%s\n",i,buf[i]);
		#endif
		free(buf[i]);   
		}
		free(buf);
		buf=NULL;
  }
  return buf;

}
int main(int argc,char *argv[])
{
	int c,i,j=1;
	int flag=0;
	FILE * pf,* mf;
	const char *from="Service@oray.net";
	const char *e_maillist=NULL;
	char *txt_part=NULL;
	char *html_part=NULL;
	char *subject=SUBJECT_TXT;
	char **ignore_list=NULL;
	char * pline = NULL;
    char * key = NULL;
	int change=0;
	int txt_index=-1,html_index=-1;
	size_t plen = 0;
	size_t pread;
		while ((c = getopt(argc, argv, "u:l:t:m:s:i:k:he")) != -1) {
					switch (c) {
						case 'u':
							from=optarg;
							break;
						case 'l':
							e_maillist=optarg;
							break;
						case 't':
							txt_part=open_t_buf(optarg);
							break;
						case 'm':
							html_part=open_t_buf(optarg);
							break;
						case 's':
							subject=open_t_buf(optarg);
							break;
						case 'e':
							change=1;
							break;
						case 'k':
							key=optarg;
							break;
						case 'i':
							if (html_part)
							{
							ignore_list=open_t_list(optarg);
							}
							break;
						case 'h':
						case '?':
						default:
						usage();
						exit(1);
					}
		}
		argc -= optind;
		argv += optind;
		if (argc != 0) {
		usage();
		exit(1);
	}
	if (html_part==NULL){fputs ("Not set html part!\n",stderr);usage();; exit (1);}
	if (e_maillist==NULL){fputs ("Not set email list file!\n",stderr);usage(); exit (1);}
	if ( subject&&subject[ strlen(subject)-1 ] == '\n' ) subject[ strlen(subject)-1 ] = '\0';
	if ( txt_part&&txt_part[ strlen(txt_part)-1 ] == '\n' ) txt_part[ strlen(txt_part)-1 ] = '\0';
	if ( txt_part&&html_part[ strlen(html_part)-1 ] == '\n' ) html_part[ strlen(html_part)-1 ] = '\0';
    if (change)
    {
		char *bm_result;
		if (bm_result=strstr(txt_part,SEARCH_KEY))
		txt_index=bm_result-txt_part;
		if (bm_result=strstr(html_part,SEARCH_KEY))
		html_index=bm_result-html_part;

    }
	   pf = fopen ( e_maillist , "rb" );
	if (pf==NULL) {fputs ("Open File error!\n",stderr); exit (1);}
     while ((pread = getline(&pline, &plen, pf)) != -1) 
	{
		char *md5key=NULL;
		unsigned char encryptdata[MD5LEN],md5str[MD5LEN*2+1];
		size_t key_len;
		if (key)
		key_len=strlen(key);
		else
		 key_len=0;		
		unsigned char msg[pread+key_len];
		flag=0;
		kill_space(pline);
		if(!pline[0])
        continue;
		//if ( pline[ strlen(pline)-1 ] == '\n' ) pline[ strlen(pline)-1 ] = '\0';
		if (change)
		{
		strcpy(msg,pline);
		if (key)
		strcat(msg,key);
	    md5_str((unsigned char *)msg,strlen(msg),encryptdata,MD5LEN,md5str);
		printf("Sum %d\t\tMail to: %s\t\tKEY:%s\n",j,pline,md5str);
			}else{
		printf("Sum %d\t\tMail to: %s\n",j,pline);
			}
		j++;
        mf=popen(SENDMAIL_PIPE,"w");
		if (mf==NULL) {fprintf(stderr, "Open sendmail pipe error:%s\n",pline); continue;}
         
		fprintf(mf,"From: %s\n",from);
		fprintf(mf,"To: %s\n",pline);
        fprintf(mf,"Subject: %s\n",subject);
		if (ignore_list==NULL&&html_part)
		{
		flag=1;
		}
		if(flag==1||!cmp(pline,ignore_list))
		{
			#ifdef DEBUG
			printf("%s use multipart mail\n",pline);
			#endif
			 flag=1;
			 fprintf(mf,"Content-Type: multipart/alternative; boundary=\"----=_Part_31568_15470104.1198046666179\"\n");
             fprintf(mf,"------=_Part_31568_15470104.1198046666179\n");
		}
			 fprintf(mf,"Content-Type: text/html; charset=\"GBK\"\n");
             fprintf(mf,"Content-Transfer-Encoding: 8bit\n");
             fprintf(mf,"\n");
			 if (change&&html_index>0)
			 {
			 char *html_buf=NULL;
			 size_t buf_len=strlen(html_part)+MD5LEN*2+pread+strlen(MAILSTR)+strlen(KEYSTR)+2;
					html_buf = (char*) malloc (sizeof(char)*buf_len);
				if (html_buf == NULL) {fputs ("Malloc Memory error!\n",stderr); continue;}
					memset(html_buf, '\0', sizeof(char)*buf_len);
					strncpy(html_buf,html_part,html_index);
					strcat(html_buf,MAILSTR);
					strcat(html_buf,pline);
					strcat(html_buf,"&");
					strcat(html_buf,KEYSTR);
					strcat(html_buf,md5str);
					strcat(html_buf,html_part+html_index+strlen(SEARCH_KEY));
					fprintf(mf,"%s\n",html_buf);
					free(html_buf);
			 }else{
			 fprintf(mf,"%s\n",html_part);
			 }
		if(flag){
			 fprintf(mf,"------=_Part_31568_15470104.1198046666179\n");
			 fprintf(mf,"Content-Type: text/plain; charset=\"GBK\"\n");
             fprintf(mf,"Content-Transfer-Encoding: 8bit\n");
             fprintf(mf,"\n");
			 if (change&&txt_index>0)
			 {
				 	char *txt_buf=NULL;
					size_t buf_len=strlen(txt_part)+MD5LEN*2+pread+strlen(MAILSTR)+strlen(KEYSTR)+2;
					txt_buf = (char*) malloc (sizeof(char)*buf_len);
				if (txt_buf == NULL) {fputs ("Malloc Memory error!\n",stderr); continue;}
					memset(txt_buf, '\0', sizeof(char)*buf_len);
					strncpy(txt_buf,txt_part,txt_index);
					strcat(txt_buf,MAILSTR);
					strcat(txt_buf,pline);
					strcat(txt_buf,"&");
					strcat(txt_buf,KEYSTR);
					strcat(txt_buf,md5str);
					strcat(txt_buf,txt_part+txt_index+strlen(SEARCH_KEY));
					fprintf(mf,"%s\n",txt_buf);
					free(txt_buf);
			 }else{
			 fprintf(mf,"%s\n",txt_part);
			 }
		     fprintf(mf,"------=_Part_31568_15470104.1198046666179\n");
		}
		     pclose(mf);
	}
	if (pline)
	free(pline);
	fclose(pf);

	if(txt_part){
	#ifdef DEBUG
	printf ("TEXT body part:\n");
	printf ("%s\n",txt_part);
	#endif
	free (txt_part);
	}
	if(html_part){
	#ifdef DEBUG
	printf ("HTML body part:\n");
	printf ("%s\n",html_part);
	#endif
	free (html_part);
	}
	if(subject!=SUBJECT_TXT){
	#ifdef DEBUG
	printf ("Subject:\n");
	printf ("%s\n",subject);
	#endif
	free (subject);
	}
	if(ignore_list){
	#ifdef DEBUG
	printf ("Ignore list:\n");
	#endif
	for (i=0; i<MAX_IGNORE_LIST && ignore_list[i]; i++)   
	{	
		#ifdef DEBUG
		printf ("line %d\t\t%s\n",i,ignore_list[i]);
		#endif
		free(ignore_list[i]);   
		}
		free(ignore_list);
	}
}
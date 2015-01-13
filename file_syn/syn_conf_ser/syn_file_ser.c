#include<netinet/in.h>              
#include<sys/types.h>              
#include<sys/socket.h>            
#include<stdio.h>               
#include<stdlib.h>             
#include<string.h>            
#include <unistd.h>
#include <stdarg.h>
#include <getopt.h>
#include <errno.h>
#include <syslog.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <dlfcn.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>

#define LENGTH_OF_LISTEN_QUEUE     20  
#define BUFFER_SIZE                4096
#define MAXFD						65536

static char *syn_sock = "/tmp/syn_sock.conf";
static char *syn_sock_conf_label = "socket";
static char *syn_sock_conf_ip = "syn_sock_conf_ip";
static char *syn_sock_conf_port = "syn_sock_conf_port";

typedef enum{
	TRUE = 1,
	true = 1,
	FALSE = 0,
	false = 0
}BOOL, bool;

typedef struct __conf_item
{
    char *item_name;
    char *item_value;
    struct __conf_item *item_next;
} conf_item;

typedef struct __query_conf
{
    char *label_name;
    conf_item *label_item;
    struct __query_conf *label_next;
} query_conf;

struct sock_conf
{
	unsigned int s_ip;
	unsigned short s_port;
};

char* trim(char *str);
static conf_item *deal_with_item_line(char *read_line, query_conf **p_conf_que, conf_item **p_item_tail);
query_conf *find_label(query_conf *p_query_conf, char *label_name);

char *get_value_from_label(query_conf *que, char *item_name);
unsigned int inet_atoul(const char * s);
char * inet_ultoa(unsigned int u, char * s);
char* line_from_buf(char *cursor, char *store, int storesz);
query_conf * load_configuration(const char *filepath);
char *pre_deal_with_line(char *line);
void free_configuration(query_conf **pque);

int init_conf();
int init_sock_conf(unsigned int s_ip, unsigned short s_port);
void start_to_trans_file(int argc, char **argv);

void deal_all_item_value(query_conf *que, char *item_name);
int get_fname_from_args(char* file_name);
void parse_fname(int argc, char **argv);
int send_data(char *send_buf, int send_len);

typedef unsigned long int UINT4;

typedef struct {
	UINT4 i[2];                   /* number of _bits_ handled mod 2^64 */
	UINT4 buf[4];                                    /* scratch buffer */
	unsigned char in[64];                              /* input buffer */
	unsigned char digest[16];     /* actual digest after MD5Final call */
} MD5_CTX;

void MD5Init ();
void MD5Update ();
void MD5Final ();

static void Transform ();

struct sock_conf sc;
const static char *sconf_file = "sock.conf";

char md5_src[33];
int write_new_file(char *file_buf, int file_len);
static void print_char(char ch);

static unsigned char PADDING[64] = {
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* F, G and H are basic MD5 functions: selection, majority, parity */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z))) 

/* ROTATE_LEFT rotates x left n bits */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4 */
/* Rotation is separate from addition to prevent recomputation */
#define FF(a, b, c, d, x, s, ac) \
{(a) += F ((b), (c), (d)) + (x) + (UINT4)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
}
#define GG(a, b, c, d, x, s, ac) \
{(a) += G ((b), (c), (d)) + (x) + (UINT4)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
}
#define HH(a, b, c, d, x, s, ac) \
{(a) += H ((b), (c), (d)) + (x) + (UINT4)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
}
#define II(a, b, c, d, x, s, ac) \
{(a) += I ((b), (c), (d)) + (x) + (UINT4)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
}

void MD5Init (MD5_CTX *mdContext)
{
	mdContext->i[0] = mdContext->i[1] = (UINT4)0;

	mdContext->buf[0] = (UINT4)0x67452301;
	mdContext->buf[1] = (UINT4)0xefcdab89;
	mdContext->buf[2] = (UINT4)0x98badcfe;
	mdContext->buf[3] = (UINT4)0x10325476;
}

void MD5Update (MD5_CTX *mdContext, unsigned char *inBuf, unsigned int inLen)
{
	UINT4 in[16];
	int mdi;
	unsigned int i, ii;

	mdi = (int)((mdContext->i[0] >> 3) & 0x3F);

	if ((mdContext->i[0] + ((UINT4)inLen << 3)) < mdContext->i[0])
		mdContext->i[1]++;
	mdContext->i[0] += ((UINT4)inLen << 3);
	mdContext->i[1] += ((UINT4)inLen >> 29);

	while (inLen--) {
		mdContext->in[mdi++] = *inBuf++;

		if (mdi == 0x40) {
			for (i = 0, ii = 0; i < 16; i++, ii += 4)
				in[i] = (((UINT4)mdContext->in[ii+3]) << 24) |
					(((UINT4)mdContext->in[ii+2]) << 16) |
					(((UINT4)mdContext->in[ii+1]) << 8) |
					((UINT4)mdContext->in[ii]);
			Transform (mdContext->buf, in);
			mdi = 0;
		}
	}
}

void MD5Final (MD5_CTX *mdContext)
{
	UINT4 in[16];
	int mdi;
	unsigned int i, ii;
	unsigned int padLen;

	in[14] = mdContext->i[0];
	in[15] = mdContext->i[1];

	mdi = (int)((mdContext->i[0] >> 3) & 0x3F);

	padLen = (mdi < 56) ? (56 - mdi) : (120 - mdi);
	MD5Update (mdContext, PADDING, padLen);

	for (i = 0, ii = 0; i < 14; i++, ii += 4)
		in[i] = (((UINT4)mdContext->in[ii+3]) << 24) |
			(((UINT4)mdContext->in[ii+2]) << 16) |
			(((UINT4)mdContext->in[ii+1]) << 8) |
			((UINT4)mdContext->in[ii]);
	Transform (mdContext->buf, in);

	for (i = 0, ii = 0; i < 4; i++, ii += 4) {
		mdContext->digest[ii] = (unsigned char)(mdContext->buf[i] & 0xFF);
		mdContext->digest[ii+1] =
			(unsigned char)((mdContext->buf[i] >> 8) & 0xFF);
		mdContext->digest[ii+2] =
			(unsigned char)((mdContext->buf[i] >> 16) & 0xFF);
		mdContext->digest[ii+3] =
			(unsigned char)((mdContext->buf[i] >> 24) & 0xFF);
	}
}

char *MD5_file (char *path, int md5_len)
{
	FILE *fp = fopen (path, "rb");
	MD5_CTX mdContext;
	int bytes;
	unsigned char data[1024];
	char *file_md5;
	int i;

	if (fp == NULL) {
		fprintf (stderr, "fopen %s failed\n", path);
		return NULL;
	}

	MD5Init (&mdContext);
	while ((bytes = fread (data, 1, 1024, fp)) != 0)
	{
		MD5Update (&mdContext, data, bytes);
	}
	MD5Final (&mdContext);

	file_md5 = (char *)malloc((md5_len + 1) * sizeof(char));
	if(file_md5 == NULL)
	{
		fprintf(stderr, "malloc failed.\n");
		return NULL;
	}
	memset(file_md5, 0, (md5_len + 1));

	if(md5_len == 16)
	{
		for(i=4; i<12; i++)
		{
			sprintf(&file_md5[(i-4)*2], "%02x", mdContext.digest[i]);
		}
	}
	else if(md5_len == 32)
	{
		for(i=0; i<16; i++)
		{
			sprintf(&file_md5[i*2], "%02x", mdContext.digest[i]);
		}
	}
	else
	{
		fclose(fp);
		free(file_md5);
		return NULL;
	}

	fclose (fp);
	return file_md5;
}

static void Transform (UINT4 *buf, UINT4 *in)
{
	UINT4 a = buf[0], b = buf[1], c = buf[2], d = buf[3];

	/* Round 1 */
#define S11 7
#define S12 12
#define S13 17
#define S14 22

	FF ( a, b, c, d, in[ 0], S11, 0xd76aa478); /* 1 */
	FF ( d, a, b, c, in[ 1], S12, 0xe8c7b756); /* 2 */
	FF ( c, d, a, b, in[ 2], S13, 0x242070db); /* 3 */
	FF ( b, c, d, a, in[ 3], S14, 0xc1bdceee); /* 4 */
	FF ( a, b, c, d, in[ 4], S11, 0xf57c0faf); /* 5 */
	FF ( d, a, b, c, in[ 5], S12, 0x4787c62a); /* 6 */
	FF ( c, d, a, b, in[ 6], S13, 0xa8304613); /* 7 */
	FF ( b, c, d, a, in[ 7], S14, 0xfd469501); /* 8 */
	FF ( a, b, c, d, in[ 8], S11, 0x698098d8); /* 9 */
	FF ( d, a, b, c, in[ 9], S12, 0x8b44f7af); /* 10 */
	FF ( c, d, a, b, in[10], S13, 0xffff5bb1); /* 11 */
	FF ( b, c, d, a, in[11], S14, 0x895cd7be); /* 12 */
	FF ( a, b, c, d, in[12], S11, 0x6b901122); /* 13 */
	FF ( d, a, b, c, in[13], S12, 0xfd987193); /* 14 */
	FF ( c, d, a, b, in[14], S13, 0xa679438e); /* 15 */
	FF ( b, c, d, a, in[15], S14, 0x49b40821); /* 16 */

	/* Round 2 */
#define S21 5
#define S22 9
#define S23 14
#define S24 20

	GG ( a, b, c, d, in[ 1], S21, 0xf61e2562); /* 17 */
	GG ( d, a, b, c, in[ 6], S22, 0xc040b340); /* 18 */
	GG ( c, d, a, b, in[11], S23, 0x265e5a51); /* 19 */
	GG ( b, c, d, a, in[ 0], S24, 0xe9b6c7aa); /* 20 */
	GG ( a, b, c, d, in[ 5], S21, 0xd62f105d); /* 21 */
	GG ( d, a, b, c, in[10], S22, 0x2441453); /* 22 */
	GG ( c, d, a, b, in[15], S23, 0xd8a1e681); /* 23 */
	GG ( b, c, d, a, in[ 4], S24, 0xe7d3fbc8); /* 24 */
	GG ( a, b, c, d, in[ 9], S21, 0x21e1cde6); /* 25 */
	GG ( d, a, b, c, in[14], S22, 0xc33707d6); /* 26 */
	GG ( c, d, a, b, in[ 3], S23, 0xf4d50d87); /* 27 */
	GG ( b, c, d, a, in[ 8], S24, 0x455a14ed); /* 28 */
	GG ( a, b, c, d, in[13], S21, 0xa9e3e905); /* 29 */
	GG ( d, a, b, c, in[ 2], S22, 0xfcefa3f8); /* 30 */
	GG ( c, d, a, b, in[ 7], S23, 0x676f02d9); /* 31 */
	GG ( b, c, d, a, in[12], S24, 0x8d2a4c8a); /* 32 */

	/* Round 3 */
#define S31 4
#define S32 11
#define S33 16
#define S34 23

	HH ( a, b, c, d, in[ 5], S31, 0xfffa3942); /* 33 */
	HH ( d, a, b, c, in[ 8], S32, 0x8771f681); /* 34 */
	HH ( c, d, a, b, in[11], S33, 0x6d9d6122); /* 35 */
	HH ( b, c, d, a, in[14], S34, 0xfde5380c); /* 36 */
	HH ( a, b, c, d, in[ 1], S31, 0xa4beea44); /* 37 */
	HH ( d, a, b, c, in[ 4], S32, 0x4bdecfa9); /* 38 */
	HH ( c, d, a, b, in[ 7], S33, 0xf6bb4b60); /* 39 */
	HH ( b, c, d, a, in[10], S34, 0xbebfbc70); /* 40 */
	HH ( a, b, c, d, in[13], S31, 0x289b7ec6); /* 41 */
	HH ( d, a, b, c, in[ 0], S32, 0xeaa127fa); /* 42 */
	HH ( c, d, a, b, in[ 3], S33, 0xd4ef3085); /* 43 */
	HH ( b, c, d, a, in[ 6], S34, 0x4881d05); /* 44 */
	HH ( a, b, c, d, in[ 9], S31, 0xd9d4d039); /* 45 */
	HH ( d, a, b, c, in[12], S32, 0xe6db99e5); /* 46 */
	HH ( c, d, a, b, in[15], S33, 0x1fa27cf8); /* 47 */
	HH ( b, c, d, a, in[ 2], S34, 0xc4ac5665); /* 48 */


	/* Round 4 */
#define S41 6
#define S42 10
#define S43 15
#define S44 21

	II ( a, b, c, d, in[ 0], S41, 0xf4292244); /* 49 */
	II ( d, a, b, c, in[ 7], S42, 0x432aff97); /* 50 */
	II ( c, d, a, b, in[14], S43, 0xab9423a7); /* 51 */
	II ( b, c, d, a, in[ 5], S44, 0xfc93a039); /* 52 */
	II ( a, b, c, d, in[12], S41, 0x655b59c3); /* 53 */
	II ( d, a, b, c, in[ 3], S42, 0x8f0ccc92); /* 54 */
	II ( c, d, a, b, in[10], S43, 0xffeff47d); /* 55 */
	II ( b, c, d, a, in[ 1], S44, 0x85845dd1); /* 56 */
	II ( a, b, c, d, in[ 8], S41, 0x6fa87e4f); /* 57 */
	II ( d, a, b, c, in[15], S42, 0xfe2ce6e0); /* 58 */
	II ( c, d, a, b, in[ 6], S43, 0xa3014314); /* 59 */
	II ( b, c, d, a, in[13], S44, 0x4e0811a1); /* 60 */
	II ( a, b, c, d, in[ 4], S41, 0xf7537e82); /* 61 */
	II ( d, a, b, c, in[11], S42, 0xbd3af235); /* 62 */
	II ( c, d, a, b, in[ 2], S43, 0x2ad7d2bb); /* 63 */
	II ( b, c, d, a, in[ 9], S44, 0xe886d391); /* 64 */


	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}

unsigned int inet_atoul(const char * s)
{
    int i;
    int u[4];
    unsigned int rv;

    if(sscanf(s, "%d.%d.%d.%d", &u[0], &u[1], &u[2], &u[3]) == 4) {
        for (i = 0, rv = 0; i < 4; i++) {
            rv <<= 8;
            rv |= u[i] & 0xff;
        }
        return rv;
    } else
        return 0xffffffff;
}

void free_item(conf_item **item)
{
	conf_item *item_tmp = NULL;
	if ((item == NULL) || (*item == NULL))
		return;

	item_tmp = *item;
	while (item_tmp != NULL)
	{
		*item = item_tmp;
		item_tmp = item_tmp->item_next;

		if ((*item)->item_name != NULL)
			free((*item)->item_name);
		if ((*item)->item_value != NULL)
			free((*item)->item_value);
		free(*item);
	}

	*item = NULL;
}

query_conf *find_label(query_conf *p_query_conf, char *label_name)
{
	query_conf *que = NULL;

	if ((p_query_conf == NULL) || (label_name == NULL))
		return NULL;

	for (que = p_query_conf; que != NULL; que = que->label_next)
	{
		if (strcmp(que->label_name, label_name) == 0)
			break;
	}

	return que;
}

char* trim(char *str)
{
	char *base = NULL;
	char *curr = NULL;
	if (str == NULL)
		return NULL;

	base = str;
	curr = str;
	while (*curr != '\0')
	{
		if (isspace(*curr))
		{
			++curr;
			continue;
		}
		*base++ = *curr++;
	}
	*base = '\0';

	return str;
}

char *pre_deal_with_line(char *line)
{
    char *ptr = NULL;

	if (line == NULL)
		return NULL;

    if ((ptr = strstr(line, "\n")) != NULL)
    {
        if (ptr - line > 1)
        {
            if (*(ptr - 1) == '\r')
                *(ptr - 1) = '\0';
        }
        *ptr = '\0';
    }

	if ((ptr = strstr(line, "#")) != NULL)
		*ptr = '\0'; 

    trim(line);

    if (line[0] == '\0')
         return NULL;
        
    return line;
}

static query_conf *deal_with_label_line(char *read_line, query_conf **p_conf_head, query_conf **p_conf_tail)
{
    char name[64] = {0};
    query_conf *p_conf_que = NULL;

    if (strncmp(read_line, "[", 1) == 0)
    {
        if (strncmp(read_line, "[/", 2) == 0)
            return NULL;

        sscanf(read_line+1, "%[^]]", name);
        if (name[0] == '\0')
            return NULL;

        p_conf_que = (query_conf *)malloc(sizeof(query_conf));
        p_conf_que->label_name = strdup(name);
        p_conf_que->label_item = NULL;
        p_conf_que->label_next = NULL;

        if ((*p_conf_head) == NULL)
        {
            (*p_conf_head) = p_conf_que;
        }
        else
        {
            (*p_conf_tail)->label_next = p_conf_que;
        }
        (*p_conf_tail) = p_conf_que;

    }

    return *p_conf_tail;
}

static conf_item *deal_with_item_line(char *read_line, query_conf **p_conf_que, conf_item **p_item_tail)
{
    char name[64] = {0};
    char value[512] = {0};
    char *p_equal_sign = NULL;

	if ((read_line == NULL) || (p_conf_que == NULL) || (*p_conf_que == NULL) || (p_item_tail == NULL))
		return NULL;

    if ((p_equal_sign = strchr(read_line, '=')) != NULL)
    {
        name[0] = '\0';
        value[0] = '\0';

        if ((*p_conf_que)->label_name[0] == '\0')
            return NULL;

        if ( p_equal_sign[1] == '\0')
        {
            sscanf(read_line, "%[^=]", name);
            sprintf(value, "%s", "");
        }
        else
        {
            sscanf(read_line, "%[^=]", name);
            p_equal_sign += 1;
            sscanf(p_equal_sign, "%[^ ]", value);
        }

        conf_item *p_item_node = (conf_item *)malloc(sizeof(conf_item));
        p_item_node->item_name = strdup(name);
        p_item_node->item_value = strdup(value);

        if ((*p_conf_que)->label_item == NULL)
            (*p_conf_que)->label_item = p_item_node;
        else
            (*p_item_tail)->item_next = p_item_node;
        *p_item_tail = p_item_node;
    }

    return *p_item_tail;
}

char* line_from_buf(char *cursor, char *store, int storesz)
{
    if ((cursor == NULL) || (store == NULL) || (storesz <= 0))
        return NULL;
    if (*cursor == '\0')
    {
        store[0] = '\0';
        return NULL;
    }

    char *ptr = strstr(cursor, "\n");
    int size = 0;
    if (ptr != NULL)
    {
        if (ptr - cursor > storesz - 1)
            return NULL;
        memcpy(store, cursor, ptr - cursor + 1);
        ptr += 1;
        if (*ptr == '\0')
            return NULL;
    }
    else
    {
        size = strlen(cursor);
        if (size > storesz - 1)
            return NULL;
        strcpy(store, cursor);
    }

    return ptr;
}

query_conf * load_configuration(const char *filepath)
{
    FILE *fp = NULL;

    char read_line[2048] = {0};
    char *line_flg = NULL;
    char *pBuf = NULL;
	char *pFlg = NULL;

	struct stat fs;
	size_t filesz = 0;

	long nread = 0;

    query_conf *p_conf_head = NULL;
    query_conf *p_conf_tail = NULL;
    conf_item *p_item_tail = NULL;

    if (filepath == NULL)
        return NULL;

	sleep(5);
    if ((fp = fopen(filepath, "r")) == NULL)
    {
        return NULL;
    }
	sleep(5);

	fstat(fp->_fileno, &fs);
	filesz = fs.st_size;
	if (filesz == 0)
    {
        fclose(fp);
		return NULL;
    }
	pBuf = (char*)malloc(filesz);
	if (pBuf == NULL)
    {
        fclose(fp);
		return NULL;
    }
	nread = fread(pBuf, filesz, 1, fp);
	if (nread != 1)
	{
        fclose(fp);
		free(pBuf);
		return NULL;
	}

    pFlg = strstr(pBuf, "[");
    if (pFlg == NULL)
    {

    }
    else 
        pFlg = strstr(pFlg, "]");
    if (pFlg == NULL)
    {
        char *general_name = (char*)malloc(32);
        p_conf_head = (query_conf *)malloc(sizeof(query_conf));
        strcpy(general_name, "default");
        p_conf_head->label_name = general_name;
        p_conf_head->label_item = NULL;
        p_conf_head->label_next = NULL;
        p_conf_tail = p_conf_head;
    }

    pFlg = pBuf;
    while ((pFlg = line_from_buf(pFlg, read_line, sizeof(read_line))) != NULL)
    {
        line_flg = pre_deal_with_line(read_line);
        if (line_flg == NULL)
            continue;
        puts(read_line);

        if (strncmp(read_line, "[", 1) == 0)
        {
            if (deal_with_label_line(read_line, &p_conf_head, &p_conf_tail) == NULL)
                continue;
        }

        if (strstr(read_line, "=") != NULL)
        {
            if (deal_with_item_line(read_line, &p_conf_tail, &p_item_tail) == NULL)
                continue; 
        }

        memset(read_line, 0, sizeof(read_line));
    }

    if (p_item_tail != NULL)
		p_item_tail->item_next = NULL;
    if (p_conf_tail != NULL)
		p_conf_tail->label_next = NULL;

    return p_conf_head;
}

void free_configuration(query_conf **pque)
{
	query_conf *que = NULL;

	if ((pque == NULL) || (*pque == NULL))
		return;
	
	que = *pque;
	while (que != NULL)
	{
		*pque = que;
		que = que->label_next;

		free_item(&((*pque)->label_item));
	}
	*pque = NULL;
}

int replace_conf_file(char *fsrc, char *fdst)
{
	FILE *stream;
	char cmd[128] = {0};
	char buf[1024] = {0};
	int ret = 0;

	sprintf(cmd, "mv %s %s", fsrc, fdst);
	stream = popen(cmd, "r");
	if (stream == NULL)
	{
		printf("stream is null\n");
		return -1;
	}
	ret = fread(buf, 1, sizeof(buf), stream);
	if (ret == 0)
		printf("rename success\n");
	else
		printf("rename failed\n");

	pclose(stream);

	return ret;
}
	
int init_sock_conf(unsigned int s_ip, unsigned short s_port)
{
	struct sockaddr_in   server_addr;  
	socklen_t server_addr_length = 0;
	int server_socket = 0;
	int reuse = 1;

    memset(&server_addr, 0x00, sizeof(server_addr));  
    server_addr.sin_family = AF_INET;  
    server_addr.sin_addr.s_addr = htonl(s_ip);  
    server_addr.sin_port = htons(s_port);  
  
    server_socket = socket(PF_INET, SOCK_STREAM, 0);  
    if (server_socket < 0)  
    {  
        printf("Create Socket Failed!\n");  
		return -1;
    }  

	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,	(const char*)&reuse, sizeof(reuse)) < 0)
	{
		printf("set sockopt error %s", strerror(errno));
		return -1;
	}

	//if (fcntl(server_socket, F_SETFL, fcntl(server_socket, F_GETFD, 0)|O_NONBLOCK) == -1)
	//		return -1;
  
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)))  
    {  
        printf("Server Bind Port: %d Failed!\n", server_addr.sin_port);  
		return -1;
    }  
    server_addr_length = sizeof(server_addr);  
 
    if (listen(server_socket, LENGTH_OF_LISTEN_QUEUE))  
    {  
        printf("Server Listen Failed!\n");  
		return -1;
    }  

	return server_socket;
}

char *get_value_from_label(query_conf *que, char *item_name)
{
	conf_item *item = NULL;
	char *res = NULL;
	if ((que == NULL) || (item_name == NULL))
		return NULL;

	item = que->label_item;
	while (item != NULL)
	{
		if (strcmp(item->item_name, item_name) == 0)
		{
			res = item->item_value;
			break;
		}
		item = item->item_next;
	}
	return res;
}

int read_sock_conf()
{
	char *val = NULL;
    struct	sockaddr_in s_addr;

	query_conf *general = NULL;
	query_conf *conf = NULL;


	if ((conf = load_configuration(syn_sock)) == NULL)
	{
        return -1;
	}

	if ((general = find_label(conf, syn_sock_conf_label)) == NULL)
	{
		free_configuration(&conf);
		return -1;
	}
	
	if ((val = get_value_from_label(general, syn_sock_conf_ip)) != NULL)
        sc.s_ip = inet_atoul(val);

	if ((val = get_value_from_label(general, syn_sock_conf_port)) != NULL)
        sc.s_port = (unsigned short)atoi(val);

	free_configuration(&conf);

	return 0;
}

int recv_file_data()
{
	struct sockaddr_in   client_addr;  
	int	server_socket = 0;
    socklen_t length = sizeof(client_addr);  
        
	if (read_sock_conf() < 0)
		return -1;

	server_socket = init_sock_conf(sc.s_ip, sc.s_port);	
	if (server_socket < 0)
	{
		printf("create socket failed!\n");
		return -1;
	}

	while (1)
	{
		int recv_len = 0;
		int total_len = 0;
		int tmp_len = 0;
		char buffer[BUFFER_SIZE] = {0};  
		char *file_buf = NULL;
		struct timeval timeout = {10, 0};
		int new_server_socket = 0;
		
		//if (fcntl(server_socket, F_SETFL, fcntl(server_socket, F_GETFD, 0)|O_NONBLOCK) == -1)
		//		return -1;
		setsockopt(new_server_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval));

		new_server_socket = accept(server_socket, (struct sockaddr*)&client_addr, &length);  
        if (new_server_socket < 0)  
        {  
            printf("Server Accept Failed!\n");  
            break;  
        }  

        length = recv(new_server_socket, buffer, sizeof(int), 0);  
        if (length < 0)  
        {  
            printf("Server Recieve Data Failed!\n");  
            break;  
        }  

		memcpy(&total_len, buffer, 4);
		total_len -= 4;

		file_buf = (char*)malloc(total_len);
		memset(file_buf, 0x00, total_len);

		while (tmp_len != total_len)
		{	
			recv_len = recv(new_server_socket, buffer, BUFFER_SIZE , 0);  
			if (recv_len < 0)  
			{  
				printf("Server Recieve Data Failed!\n");  
				break;  
			}  
			
			memcpy(file_buf + tmp_len, buffer, recv_len);
			tmp_len = tmp_len + recv_len;	
		}
		write_new_file(file_buf, total_len);
		free(file_buf);
		close(new_server_socket);
	}

	close(server_socket);
}

int write_new_file(char *file_buf, int file_len)
{
	int tmp_len = 0;
	int fname_len = 0;
	int fdata_len = 0;
	int write_len = 0;

	char fname[256] = {0};
	char fdata[4096] = {0};
	char ftmp[256] = {0};

	unsigned short chk_src = 0;
	unsigned short chk_dst = 0;

	FILE *fp = NULL;
	char *md5_dst = NULL;

	if ((file_buf == NULL)||(file_len == 0))
	{
		printf("file is not exist!\n");
		return 0;
	}

	memcpy(&fname_len, file_buf, sizeof(int));
	memcpy(fname, file_buf + sizeof(int), fname_len);
	memcpy(&fdata_len, file_buf + sizeof(int) + fname_len, sizeof(int));
	memcpy(md5_src, file_buf + 2*sizeof(int) + fname_len + fdata_len, 32);

	sprintf(ftmp, "%s.tmp", fname);

	fp = fopen(ftmp, "w+");  
    if (fp == NULL)  
    {  
        printf("File:\t%s Can Not Open To Write!\n", fname);  
        return -1;  
    }  

	while (tmp_len < fdata_len)
	{
		write_len = fwrite(file_buf+2*sizeof(int)+fname_len, sizeof(char), fdata_len, fp);  
		tmp_len = tmp_len + write_len;
    } 

	fclose(fp);
	
	md5_dst = MD5_file(ftmp, 32);
	if (!memcmp(md5_dst, md5_src, 32)) 
	{
		if (replace_conf_file(ftmp, fname) < 0)
			printf("move file failed");
		free(md5_dst);
	}
	else
	{
		printf("md5 is not same\n");
		free(md5_dst);
	}

	return 1;
}

pid_t create_daemon()
{
	int i;
	
	pid_t pid;

	if ((pid = fork()) != 0)
		return pid;
		//exit(0);
		
	if (setsid() < 0)
		return -1;

	for(i=0; i<MAXFD; i++)
		close(i);
	
	//chdir("/");
	open("/dev/null", STDIN_FILENO);
	open("/dev/null", STDOUT_FILENO);
	open("/dev/null", STDERR_FILENO);

	return pid;
}

int main(int argc, char **argv)  
{  
	int opt = -1;
	int optIdx = -1;
	bool daemon = true;

	struct option opts[] =
	{
		{"debug", no_argument, NULL, 'd'}
	};

	while ((opt = getopt_long(argc, argv, "d", opts, &optIdx)) != -1)
	{
		switch (opt) {
			case 'd':
				daemon = false;
				break;
		}
	}

	pid_t d_pid = create_daemon();
	if (daemon && (d_pid != 0))
	{
		return 0;
	}
	
	if (recv_file_data() < 0)
		return -1;

   return 0;  
}
  
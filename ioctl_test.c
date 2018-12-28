#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <sys/utsname.h>
#include <limits.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/sockios.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <endian.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#define GETMODULEINFO 0x00000042
#define GETMODULEEEPROM 0x00000043
#define SIOCETHTOOL 0x8946
#define LIST_HEAD_INIT(name) {&(name), &(name)}
struct cmd_context {
	const char *devname;
	int fd;
	struct ifreq ifr;
	int argc;
	char **argp;
};

struct modinfo {
	__u32 cmd;
	__u32 type;
	__u32 eeprom_len;
	__u32 reserved[8];
};

struct eeprominfo {
	__u32 cmd;
	__u32 magic;
	__u32 offest;
	__u32 len;
	__u8 data[0];
};

int send_ioctl(struct cmd_context *ctx, void *cmd)
{
	ctx->ifr.ifr_data = cmd;
	return ioctl(ctx->fd, SIOCETHTOOL, &ctx->ifr);
}

struct list_head{ struct list_head *next, *prev;};
static void list_add(struct list_head *new, struct list_head *head)
{
	head->next->prev = new;
	new->next = head->next;
	new->prev = head;
	head->next = new;
}

static struct list_head malloc_list = {&malloc_list,&malloc_list};
void *do_malloc(size_t size)
{
	struct list_head *block = malloc(sizeof(*block) + size);
	if (!block)
		return NULL;
	list_add(block, &malloc_list);
	return block+1;
}
void *do_calloc(size_t nmemb, size_t size)
{
	void *ptr = do_malloc(nmemb * size);
	if (ptr)
		memset(ptr, 0, nmemb * size);
	return ptr;
}


int main(int argc, char **argp)
{
	struct cmd_context ctx;
	struct modinfo mdf;
	struct eeprominfo *eeprom;
	int geeprom_length = 0;
	int err = 0;
	argp++;
	ctx.devname = *argp;
	printf("devname=%s",ctx.devname);
	if (ctx.devname == NULL)
	{
		printf("No devname,exit\n");
		return 71;
	}
	memset(&ctx.ifr, 0, sizeof(ctx.ifr));
	strcpy(ctx.ifr.ifr_name, ctx.devname);
	ctx.fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (ctx.fd < 0){
		perror("Cannot get control socket");
		return 70;
	}
	ctx.argc = argc;
	ctx.argp = argp;
	mdf.cmd = GETMODULEINFO;
	err = send_ioctl(&ctx, &mdf);
	if (err < 0){
		perror("Cannot get module EEPROM information");
		return 1;
	}
	else{
		geeprom_length = mdf.eeprom_len;
	}
	eeprom = calloc(1,sizeof(*eeprom)+geeprom_length);
	eeprom->cmd = GETMODULEEEPROM;
	eeprom->len = geeprom_length;
	eeprom->offest = 0;
	err = send_ioctl(&ctx,eeprom);
	if (err < 0){
		perror("Cannot get Module EEPROM data");
		free(eeprom);
		return 1;
	}
	else{
		printf("get the module data! length of data is %d:\n",eeprom->len);
		printf("*************************\n");
		for(int i=0; i<eeprom->len; i++)
			printf("%u",eeprom->data[i]);
		printf("\n*************************\n");
	}
	//free(eeprom);
	return 1;
	
}

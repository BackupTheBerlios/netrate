/* $Id: if_freebsd.c,v 1.1 2004/08/18 19:23:26 kman Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

/* FreeBSD 2.x has some missing features that are not strictly required */
#if FREEBSD > 2
#include <net/if_var.h>
#endif

#include <fcntl.h>
#include <kvm.h>
#include <nlist.h>


#include "interfaces.h"

#if FREEBSD > 2
	#define GetNextIface(ifnet) TAILQ_NEXT(&ifnet, if_link)
#else
	TAILQ_HEAD(ifnethead, ifnet);
	#define GetNextIface(ifnet) ifnet.if_next
#endif

#if FREEBSD >= 5
	#define DeviceName(a) a.if_dname
	#define DeviceUnit(a) a.if_dunit
#else
	#define DeviceName(a) a.if_name
	#define DeviceUnit(a) a.if_unit
#endif

static struct nlist nl[] = {
	{ "_ifnet" },
	{ "" }
};

static kvm_t	*kvmd;
static int	init_ok = 0;

/* initialize - non-zero on success, print reason on failure */
int
if_init(void)
{
	kvmd = kvm_open(NULL, NULL, NULL, O_RDONLY, "kvm_open");

	if (!kvmd)
		return 0;

	if (kvm_nlist(kvmd, nl) != 0)
	{
		printf("Error in nlist\n");
		return 0;
	}

	if (nl[0].n_type == 0)
	{
		printf("nlist: no namelist\n");
		return 0;
	}

	init_ok = 1;

	return 1;
}

int if_count(void)
{
	struct ifnethead	ifnethead;
	struct ifnet		ifnet;
	u_long			ifnetaddr;

	int			count;

	if(!init_ok)
	{
		printf("Error: if_init not called, or call failed!\n");
		exit(1);
	}

	if (kvm_read(kvmd, nl[0].n_value, (void *)&ifnethead, sizeof(ifnethead)) < sizeof(ifnethead))
	{
		printf("error in kvm_read of ifnethead\n");
		return 0;
	}
	
	ifnetaddr = (u_long) TAILQ_FIRST(&ifnethead);

	count = 0;
	while(ifnetaddr)
	{
		if (ifnet.if_flags & IFF_UP)
			count++;
		kvm_read(kvmd, ifnetaddr, (void*) &ifnet, sizeof ifnet);
		ifnetaddr = (u_long) GetNextIface(ifnet); /* TAILQ_NEXT(&ifnet, IF_LINK); */
	}

	return count;
}

/* get interface names */
int
if_getnames(int if_nr, char **names)
{
	struct ifnethead	ifnethead;
	struct ifnet		ifnet;
	u_long			ifnetaddr;

	int			i;
	char			*name;
	char			tmpname[IFNAME_MAXLEN];
	
	if(!init_ok)
	{
		printf("Error: if_init not called, or call failed!\n");
		exit(1);
	}

	if (kvm_read(kvmd, nl[0].n_value, (void *)&ifnethead, sizeof (ifnethead)) < sizeof (ifnethead))
	{
		printf("error in kvm_read of ifnethead\n");
		return 0;
	}
	
	ifnetaddr = (u_long) TAILQ_FIRST(&ifnethead);

	i = 0;
	while (i < if_nr && ifnetaddr)
	{
		kvm_read(kvmd, ifnetaddr, (void*) &ifnet, sizeof ifnet);
		ifnetaddr = (u_long) GetNextIface(ifnet); /* TAILQ_NEXT(&ifnet, IF_LINK); */

		name = (char*) malloc(IFNAME_MAXLEN);
		kvm_read(kvmd, (u_long)DeviceName(ifnet), (void *)tmpname, IFNAME_MAXLEN);
		tmpname[IFNAME_MAXLEN-1] = 0;
		snprintf(name, IFNAME_MAXLEN, "%s%d", tmpname, DeviceUnit(ifnet));
		names[i]=name;
		
		if (ifnet.if_flags & IFF_UP)
			i++;
		else
			free(names[i]);
	}
}

/* get status of at most if_nr interfaces; return how many exactly */
int
if_getstatus(int if_nr, struct interface_data *data)
{
	struct ifnethead	ifnethead;
	struct ifnet		ifnet;
	u_long			ifnetaddr;

	int			i;
	
	if(!init_ok){
		printf("Error: if_init not called, or call failed!\n");
		exit(1);
	}

	if (kvm_read(kvmd, nl[0].n_value, (void *)&ifnethead, sizeof(ifnethead)) < sizeof(ifnethead)){
		printf("error in kvm_read of ifnethead\n");
		return 0;
	}

	ifnetaddr = (u_long) TAILQ_FIRST(&ifnethead);

	i = 0;
	while (i < if_nr && ifnetaddr){
		kvm_read(kvmd, ifnetaddr, (void*) &ifnet, sizeof ifnet);
		ifnetaddr = (u_long) GetNextIface(ifnet); /* TAILQ_NEXT(&ifnet, IF_LINK); */

		if (ifnet.if_flags & IFF_UP)
		{
			data[i].bytes_in = ifnet.if_data.ifi_ibytes;
			data[i].bytes_out = ifnet.if_data.ifi_obytes;

			i++;
		}
	}

	return i;
}

/* end interface data grabbing */
void
if_close(void)
{
	init_ok = 0;
	if (kvm_close(kvmd) == -1)
		printf("Error in kvm_close\n");
}

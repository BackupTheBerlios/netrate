/* $Id: if_openbsd.c,v 1.1 2004/08/18 19:23:26 kman Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <fcntl.h>
#include <kvm.h>
#include <nlist.h>


#include "interfaces.h"

static struct nlist nl[] = {
	{ "_ifnet" },
	{ "" }
};

static kvm_t	*kvmd;
static int	init_ok = 0;

int
if_init(void)
{
	kvmd = kvm_open(NULL, NULL, NULL, O_RDONLY, "kvm_open");

	if (!kvmd)
		return 0;

	if (kvm_nlist(kvmd, nl) != 0)
	{
		fprintf(stderr, "Error in nlist\n");
		return 0;
	}

	if (nl[0].n_type == 0)
	{
		fprintf(stderr, "nlist: no namelist\n");
		return 0;
	}

	init_ok = 1;

	return 1;
}

int if_count(void)
{
	struct ifnet_head	ifnethead;
	struct ifnet		ifnet;
	u_long			ifnetaddr;

	int			count;

	if(!init_ok)
	{
		fprintf(stderr, "Error: if_init not called, or call failed!\n");
		exit(1);
	}

	if (kvm_read(kvmd, nl[0].n_value, (void *)&ifnethead, sizeof(ifnethead)) < sizeof(ifnethead))
	{
		fprintf(stderr, "error in kvm_read of ifnethead\n");
		return 0;
	}
	
	ifnetaddr = (u_long) TAILQ_FIRST(&ifnethead);

	count = 0;
	while(ifnetaddr)
	{
		kvm_read(kvmd, ifnetaddr, (void*) &ifnet, sizeof ifnet);
		if (ifnet.if_flags & IFF_UP) 
			count++;
		ifnetaddr = (u_long) TAILQ_NEXT(&ifnet, if_list);
	}
	
	return count;
}

int
if_getnames(int if_nr, char **names)
{
	struct ifnet_head	ifnethead;
	struct ifnet		ifnet;
	u_long			ifnetaddr;

	int			i;
	
	if(!init_ok)
	{
		fprintf(stderr, "Error: if_init not called, or call failed!\n");
		exit(1);
	}

	if (kvm_read(kvmd, nl[0].n_value, (void *)&ifnethead, sizeof (ifnethead)) < sizeof (ifnethead))
	{
		fprintf(stderr, "error in kvm_read of ifnethead\n");
		return 0;
	}
	
	ifnetaddr = (u_long) TAILQ_FIRST(&ifnethead);

	i = 0;
	while (i < if_nr && ifnetaddr)
	{
		kvm_read(kvmd, ifnetaddr, (void*) &ifnet, sizeof ifnet);
		if (ifnet.if_flags & IFF_UP)
		{
			names[i] = strdup(ifnet.if_xname);
			i++;
		}
		ifnetaddr = (u_long) TAILQ_NEXT(&ifnet, if_list);
	}
}

int
if_getstatus(int if_nr, struct interface_data *data)
{
	struct ifnet_head	ifnethead;
	struct ifnet		ifnet;
	u_long			ifnetaddr;

	int			i;
	
	if(!init_ok)
	{
		fprintf(stderr, "Error: if_init not called, or call failed!\n");
		exit(1);
	}

	if (kvm_read(kvmd, nl[0].n_value, (void *)&ifnethead, sizeof(ifnethead)) < sizeof(ifnethead))
	{
		fprintf(stderr, "error in kvm_read of ifnethead\n");
		return 0;
	}

	ifnetaddr = (u_long) TAILQ_FIRST(&ifnethead);

	i = 0;
	while (i < if_nr && ifnetaddr)
	{
		kvm_read(kvmd, ifnetaddr, (void*) &ifnet, sizeof ifnet);
		ifnetaddr = (u_long) TAILQ_NEXT(&ifnet, if_list);

		if (ifnet.if_flags & IFF_UP)
		{
			data[i].bytes_in = ifnet.if_data.ifi_ibytes;
			data[i].bytes_out = ifnet.if_data.ifi_obytes;
			i++;
		}
	}

	return i;
}

void
if_close(void)
{
	init_ok = 0;
	if (kvm_close(kvmd) == -1)
		fprintf(stderr, "Error in kvm_close\n");
}

/* $Id: interfaces.h,v 1.1 2004/08/18 19:23:26 kman Exp $ */

#ifndef NETRATE_INTERFACES_H
#define NETRATE_INTERFACES_H

struct interface_data{
	long long	bytes_in;
	long long	bytes_out;
};

#define IFNAME_MAXLEN	16

/* initialize - non-zero on success, print reason on failure */
int	if_init(void);

/* total number of interfaces */
int	if_count(void);

/* get interface names */
int	if_getnames(int if_nr, char **names);

/* get status of at most if_nr interfaces; return how many exactly */
int	if_getstatus(int if_nr, struct interface_data *data);

/* end interface data grabbing */
void	if_close(void);

#endif

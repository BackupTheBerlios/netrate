/* $Id: if_linux.c,v 1.1 2004/08/18 19:25:05 kman Exp $ */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <sys/file.h>

#include "interfaces.h"

#define _LINE_DIM		512
#define _PROC_NET_DEV           "/proc/net/dev"
#define _PROC_NET_ROUTE		"/proc/net/route"

static int init_ok = 0;
static int skfd = 0;
static FILE *dev_file = NULL;

struct name_list 
{
    char *if_name;
    struct name_list *next;
};

static struct name_list *if_names = NULL;

/* get the status of a network device */
static int iface_is_up(char *name)
{
    struct name_list *nl = if_names;
    
    while (nl)
    {
	if ( !strncmp(nl->if_name, name, strlen(name)) )
	    return 1;
	nl = nl->next;
    }

    return 0;
}

/* get name of network device */
static char *get_name(char *name, char *p)
{
	while (isspace(*p))
		p++;
	while (*p) 
	{
		if (isspace(*p))
		    break;
		if (*p == ':') 
		{	
			/* could be an alias */
			char *dot = p, *dotname = name;
			*name++ = *p++;
			while (isdigit(*p))
				*name++ = *p++;
			if (*p != ':') 
			{	
				/* it wasn't, backup */
				p = dot;
				name = dotname;
			}
			if (*p == '\0')
	    			return NULL;
			p++;
			break;
		}
		*name++ = *p++;
	}
	*name++ = '\0';
	return p;
}

/* get the name of the device in /proc/net/route */
static char *get_name2(char *name)
{
	char *p = name;
	
	while (!isspace(*p))
		p++;
	*p = '\0';
	
	return name;
}

/* read the routes, we need them to find out if an interface is up or not */
void read_routes()
{
	FILE *f = fopen (_PROC_NET_ROUTE, "r");
	
	if (!f)
	{
		perror("read_routes() failed!\n");
		return;
	}
	
        char line[_LINE_DIM];
	char *name;
	
	/* eat the first line containing the headers */
        fgets(line, sizeof(line), f);
	memset(line, 0, sizeof(line));
	
        while ( fgets(line, sizeof(line), f) )
	{
		name = get_name2(line);
		
		struct name_list *p = (struct name_list*)malloc(sizeof(struct name_list));
		if (!p)
		{
			perror("malloc() failed!\n");
			exit(-1);
		}
		p->if_name = strdup(name);
		p->next = if_names;
		if_names = p;
		memset(line, 0, sizeof(line));
	}
	
	fclose(f);
}

/* initialize - non-zero on success, print reason on failure */
int
if_init(void)
{
	read_routes();
	
        dev_file = fopen (_PROC_NET_DEV, "r");

        if (dev_file)
        {
                init_ok = 1;
                return 1;
        }
        else
        {
        	init_ok = 0;
                perror("if_init() failed because it couldn't open /proc/net/dev/\n");
                return 0;
        }
}

/* total number of interfaces */
int
if_count(void)
{
	if (!init_ok)
        {
        	perror ("if_count() called without calling if_init() first\n");
                return 0;
        }

        char line[_LINE_DIM];
	char name[IFNAME_MAXLEN];

        int if_num=0;

        rewind(dev_file);
        /* eat 2 heading lines*/
        fgets(line, sizeof(line), dev_file);
        fgets(line, sizeof(line), dev_file);  
	memset(line, 0, sizeof(line));      
        
        while ( fgets(line, sizeof(line), dev_file) )
	{
		get_name(name, line);
		if ( iface_is_up(name) )
			if_num++;
		memset(line, 0, sizeof(line));
	}
        
        return if_num;
}

/* get interface names */
int
if_getnames(int if_nr, char **names)
{
	if (!init_ok)
        {
        	perror ("if_getnames() called without calling if_init() first\n");
                return 0;
        }

        int if_num = 0;
        char line[_LINE_DIM];
        
        rewind (dev_file);
        /* eat 2 heading lines*/
        fgets(line, sizeof(line), dev_file);
        fgets(line, sizeof(line), dev_file);        
	memset(line, 0, sizeof(line));

        while ( fgets(line, sizeof(line), dev_file) )
	{
        	names[if_num] = (char*)malloc(IFNAME_MAXLEN);
                get_name(names[if_num], line);
		if ( iface_is_up(names[if_num]) )
			if_num++;
		else
			free(names[if_num]);
		memset(line, 0, sizeof(line));
        }
        
        return 1;
}

/* get status of at most if_nr interfaces; return how many exactly */
int
if_getstatus(int if_nr, struct interface_data *data)
{
	if (!init_ok)
        {
        	perror ("if_getnames() called without calling if_init() first\n");
                return 0;
        }


        char line[_LINE_DIM], name[IFNAME_MAXLEN], c;
        int i=0;
        
        rewind(dev_file);
        /* eat 2 heading lines*/
        fgets(line, sizeof(line), dev_file);
        fgets(line, sizeof(line), dev_file);
	memset(line, 0, sizeof(line));

        while ( fgets(line, sizeof(line), dev_file) && i<if_nr )
	{
        	char *c = get_name(name, line);
                if ( iface_is_up(name) )
		{
			/* dummy variables to satisfy sscanf */
            		unsigned long d;
            		unsigned long long ld;
                
            		sscanf( c,
                		"%llu %llu %lu %lu %lu %lu %lu %lu %llu %llu %lu %lu %lu %lu %lu %lu",
                    		&data[i].bytes_in,
                            	      &ld, &d, &d, &d, &d, &d, &d,
                		&data[i].bytes_out,
                                                            		&ld, &d, &d, &d, &d, &d, &d);
            		i++;
		}
		memset(line, 0, sizeof(line));
        }

        return 1;
}

/* end interface data grabbing */
void
if_close(void)
{
	if (!init_ok)
	{
        	perror ("if_close() called without calling if_init() first\n");
                return;
        }

        fclose(dev_file);
	
	struct name_list *nl;
	
	while (if_names)
	{
	    nl = if_names;
	    if_names = if_names->next;
	    free(nl->if_name);
	    free(nl);
	} 
	if_names = NULL;
        
        init_ok = 0;
}




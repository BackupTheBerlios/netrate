/* $Id: netrate.c,v 1.1 2004/08/18 19:25:05 kman Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/types.h>

#include "interfaces.h"

/* defaults */
int	num_samples = 20;	/* samples per second */
int	num_seconds = 5;	/* seconds of history */
float	min_rate = 2048.0;	/* do not scale below 2k/sec */

/* terminal settings, should allow override in the future */
char	TERM_HOME[]		= "\x1b[H";
char	TERM_CLR_EOL[]		= "\x1b[K";
char	TERM_CLR_SCREEN[]	= "\x1b[2J";
char	TERM_IN_COLOR[]		= "\x1b[41m";
char	TERM_OUT_COLOR[]	= "\x1b[42m";
char	TERM_NORMAL_COLOR[]	= "\x1b[m";
char	TERM_CURSOR_OFF[]	= "\x1b[?25l\x1b[?1c";
char	TERM_CURSOR_ON[]	= "\x1b[?25h\x1b[?0c";

int	term_width		= 80;
	
void
usage(char *myname, int exitcode)
{
	printf("Usage:\n"
		"\t%s -h\n"
		"\t%s [-n seconds] [-s samples] [-w width]\n",
		myname,myname);
	exit(exitcode);
}

extern char *optarg;
struct termios *old_attr=NULL;

void reset_screen()
{
	printf("%s", TERM_CLR_SCREEN);
	if (old_attr)
		if (tcsetattr(STDIN_FILENO,TCSAFLUSH,old_attr) < 0)
		{
            		perror("tcsetattr() failed\n");
            		exit(-1);
    		}
// only tested on linux
#ifdef OS_Linux
	printf("%s", TERM_CURSOR_ON);
#endif
}

/* catch any signal that might kill the process since atexit is not allways called */
void signal_catch(int sig)
{
	signal(sig, signal_catch);
	reset_screen();
	exit(0);
}

int
main(int argc, char **argv)
{
	/* options */
	int	ch;

	unsigned int		delay;	/* for usleep() */
	int			total_samples;
	int			num_interfaces;
	struct interface_data	**samples;
	double			*sampletimes;
	char			**if_names;
	int			i, j;
	int			current, last;

	struct timeval		now;
	struct timezone		tz = {0, 0};
	double			doubletime;
	double			max_rate;
	double			time_diff;
	double			local_max;

        /* terminal stuff */
        struct termios attr;
        
	/* parse options */
	while ((ch = getopt(argc, argv, "hw:s:n:")) != -1)
		switch (ch) {
		case 'h':
			usage(argv[0], 0);
			/* NOTREACHED */
		case 'w':
			term_width = atoi(optarg);
			break;
		case 's':
			num_samples = atoi(optarg);
			break;
		case 'n':
			num_seconds = atoi(optarg);
			break;
		case '?':
		default:
			usage(argv[0], 1);
			/* NOTREACHED */
		}
	
	/* validation */
	if (term_width < 40){
		printf("Sorry, your terminal is not wide enough.\n");
		exit(1);
	}
	if(term_width > 132)
		printf("Warning: terminal is unusually wide.\n");
	if(num_seconds < 1){
		printf("Sorry, cannot timetravel.\n");
		exit(1);
	}
	if(num_seconds > 10)
		printf("Warning: averaging over more than 10 seconds, the display will probably change very slowly.\n");
	if(num_samples < 1){
		printf("Sorry, I have to sample at least once a second.\n");
		exit(1);
	}
	if(num_samples > 20)
		printf("Warning: sampling more than 20 times a second.\n"
		    "\tThis might eat some CPU.\n"
		    "\tAlso this will eat bandwidth if viewed over telnet or ssh.\n");
	
	delay = 1000000L / num_samples; /* 1000000L usecs in a second */
	total_samples = num_seconds * num_samples;

	samples = (struct interface_data **) malloc(total_samples * sizeof (struct interface_data*));
	if (!samples){
		printf("Error:  could not allocate memory for samples array.\n");
		exit(1);
	}

	sampletimes = (double *) malloc(total_samples * sizeof (double));
	if (!sampletimes){
		printf("Error:  could not allocate memory for sample times.\n");
		exit(1);
	}

	
	/* try to initialize */
	if (!if_init()){
		printf("Exiting due to error in if_init\n");
		exit(1);
	}
	
	/* register signals */
	signal(SIGQUIT, signal_catch);
	signal(SIGINT, signal_catch);

	num_interfaces = if_count();

	if_names = (char**) malloc(num_interfaces * sizeof(char*));
	if(!if_names){
		printf("Cannot allocate interface names array");
		exit(1);
	}
	if_getnames(num_interfaces, if_names);

	for (i = 0; i < total_samples; i++){
		samples[i] = (struct interface_data *) malloc(num_interfaces * sizeof (struct interface_data));
		if (!samples[i]){
			printf("Error! Could not allocate interface data for sample %d.\n",i);
			exit(1);
		}
	}

	gettimeofday(&now, &tz);
	doubletime = now.tv_sec + 1e-6 * now.tv_usec;
	sampletimes[0] = doubletime;

	if_getstatus(num_interfaces, samples[0]);

	for (i = 1; i < total_samples; i++){
		memcpy(samples[i], samples[0], num_interfaces * sizeof(struct interface_data));
		/* artificial 1sec delay, to avoid division by zero at first iteration */
		sampletimes[i] = sampletimes[0] - 1.0; 
	}
	
	printf("%s", TERM_CLR_SCREEN);
	max_rate = min_rate;

	current = 0;
	last = 1;

        /* terminal management */
        if(tcgetattr(STDIN_FILENO, &attr) < 0)
        {
                perror("tcgetattr() failed\n");
                exit(-1);
        }
	/* save old attributes so we can restore them */
	old_attr = (struct termios*)malloc(sizeof(struct termios));
	memcpy(old_attr, &attr, sizeof(struct termios));
	
        attr.c_lflag &= ~ICANON;	/* no buffering */
	/*attr.c_lflag &= ~ECHO;		/* no echo */
	if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&attr) < 0)
        {
                perror("tcsetattr() failed\n");
                exit(-1);
        }
        fcntl(STDIN_FILENO, F_SETFL, O_RDONLY | O_NONBLOCK);
// only tested on linux, turn the cursor off
#ifdef OS_Linux
	printf("%s", TERM_CURSOR_OFF);
#endif    
	/* set exit handler */
	if ( atexit(reset_screen) != 0)
	{
		perror("atexit() failed\n");
		exit(-1);
	}
	
	/* main loop */
	for (;;)
        {
		int tmp;

		if_getstatus(num_interfaces, samples[current]);
		gettimeofday(&now, &tz);
		doubletime = now.tv_sec + 1e-6 * now.tv_usec;
		sampletimes[current] = doubletime;

		time_diff = sampletimes[current] - sampletimes[last];

		local_max = 0;
		for(i = 0; i < num_interfaces; i++){
			if((samples[current][i].bytes_in - samples[last][i].bytes_in) / time_diff > local_max)
				local_max = (samples[current][i].bytes_in - samples[last][i].bytes_in) / time_diff;
			if((samples[current][i].bytes_out - samples[last][i].bytes_out) / time_diff > local_max)
				local_max = (samples[current][i].bytes_out - samples[last][i].bytes_out) / time_diff;
		}
		if(local_max > max_rate)
			max_rate = local_max;
		/* now we know the maximum rate */
		printf("%sMax Rate: %10.2f\n", TERM_HOME, max_rate);
		for(i = 0; i < num_interfaces; i++){
			printf("%-10s: in:%10.2f bytes/sec   out:%10.2f bytes/sec\n",
			    if_names[i], 
			    (samples[current][i].bytes_in - samples[last][i].bytes_in)/time_diff,
			    (samples[current][i].bytes_out - samples[last][i].bytes_out)/time_diff);
			tmp = (term_width - 1) * (samples[current][i].bytes_in - samples[last][i].bytes_in) /
			    time_diff / max_rate;
			printf("%s", TERM_IN_COLOR);
			for(j = 0; j < tmp; j++)
				putchar(' ');
			printf("%s%s\n", TERM_NORMAL_COLOR, TERM_CLR_EOL);
			tmp = (term_width - 1) * (samples[current][i].bytes_out - samples[last][i].bytes_out) /
			    time_diff / max_rate;
			printf("%s", TERM_OUT_COLOR);
			for(j = 0; j < tmp; j++)
				putchar(' ');
			printf("%s%s\n", TERM_NORMAL_COLOR, TERM_CLR_EOL);
		}
		fflush(stdout);

		current++;
		if(current >= total_samples){
			current = 0;
			max_rate *= 0.90;
			if(max_rate < local_max)
				max_rate = local_max;
			if(max_rate < min_rate)
				max_rate = min_rate;
		}
		last++;
		if(last >= total_samples)
			last = 0;

                usleep(delay);
                if ( getchar() == 'q' )
                        exit(0);
	}
}


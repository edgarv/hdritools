#include <stdio.h>
#include <time.h>

int main(int argc, char **argv)
{
    time_t ltime;
	struct tm *today;
    struct tm timebuf;
    FILE *of;

    if (argc != 2) {
        of = stdout;
    } else {
#if _MSC_VER >= 1400
        if (fopen_s(&of, argv[1], "w") != 0) return 3;
#else
        of = fopen(argv[1], "w");
        if (!of) return 3;
#endif
    }
	
	time(&ltime);
#if _MSC_VER >= 1400
    if (localtime_s(&timebuf, &ltime) != 0) return 1;
    today = &timebuf;
#else
    today = localtime(&ltime);
	if (!today) return 1;
#endif

	fprintf(of, "%d.%02d.%02d", (today->tm_year + 1900), 
	    (today->tm_mon + 1), today->tm_mday);
    if (of != stdout) fclose(of);
    return 0;
}

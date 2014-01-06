#include "times.h"

time_t now = 0;
static time_t last_now = 0;
struct tm _now_gtm = {0};
struct tm _now_lc = {0};
char now_gmt[32] = {0};
char now_lc[32] = {0};

static const char *DAYS_OF_WEEK[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char *MONTHS_OF_YEAR[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

long longtime()
{
    struct timeb t;
    ftime(&t);
    return 1000 * t.time + t.millitm;
}

time_t update_time()
{
    time(&now);

    if(now <= last_now) {
        return now;
    }

    last_now = now;

    gmtime_r(&now, &_now_gtm);
    localtime_r(&now, &_now_lc);
    sprintf(now_gmt, "%s, %02d %s %04d %02d:%02d:%02d GMT",
            DAYS_OF_WEEK[_now_gtm.tm_wday],
            _now_gtm.tm_mday,
            MONTHS_OF_YEAR[_now_gtm.tm_mon],
            _now_gtm.tm_year + 1900,
            _now_gtm.tm_hour,
            _now_gtm.tm_min,
            _now_gtm.tm_sec);
    sprintf(now_lc, "%s, %02d %s %04d %02d:%02d:%02d",
            DAYS_OF_WEEK[_now_lc.tm_wday],
            _now_lc.tm_mday,
            MONTHS_OF_YEAR[_now_lc.tm_mon],
            _now_lc.tm_year + 1900,
            _now_lc.tm_hour,
            _now_lc.tm_min,
            _now_lc.tm_sec);

    return now;
}

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "divoom.h"
#include "divoom_timelib.h"

//https://github.com/espressif/newlib-esp32
//c库为newlib
//#define sscanf siscanf  /* avoid to pull in FP functions. */



#define SECSPERMIN  60L
#define MINSPERHOUR 60L
#define HOURSPERDAY 24L
#define SECSPERHOUR (SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY  (SECSPERHOUR * HOURSPERDAY)
#define DAYSPERWEEK 7
#define MONSPERYEAR 12

#define YEAR_BASE   1900
#define EPOCH_YEAR      1970
#define EPOCH_WDAY      4
#define EPOCH_YEARS_SINCE_LEAP 2
#define EPOCH_YEARS_SINCE_CENTURY 70
#define EPOCH_YEARS_SINCE_LEAP_CENTURY 370

#define isleap(y) ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)


//const int __month_lengths[2][MONSPERYEAR];
const int __month_lengths[2][MONSPERYEAR] =
{
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
} ;

int divoom_tzcalc_limits (int year,divoom_tzinfo_type *const tz)
{
    int days, year_days, years;
    int i, j;
//  __tzinfo_type *const tz = __gettzinfo ();

    if (year < EPOCH_YEAR)
        return 0;

    tz->__tzyear = year;

    years = (year - EPOCH_YEAR);

    year_days = years * 365 +
                (years - 1 + EPOCH_YEARS_SINCE_LEAP) / 4 -
                (years - 1 + EPOCH_YEARS_SINCE_CENTURY) / 100 +
                (years - 1 + EPOCH_YEARS_SINCE_LEAP_CENTURY) / 400;

    for (i = 0; i < 2; ++i)
    {
        if (tz->__tzrule[i].ch == 'J')
        {
            /* The Julian day n (1 <= n <= 365). */
            days = year_days + tz->__tzrule[i].d +
                   (isleap(year) && tz->__tzrule[i].d >= 60);
            /* Convert to yday */
            --days;
        }
        else if (tz->__tzrule[i].ch == 'D')
            days = year_days + tz->__tzrule[i].d;
        else
        {
            const int yleap = isleap(year);
            int m_day, m_wday, wday_diff;
            const int *const ip = __month_lengths[yleap];

            days = year_days;

            for (j = 1; j < tz->__tzrule[i].m; ++j)
                days += ip[j-1];

            m_wday = (EPOCH_WDAY + days) % DAYSPERWEEK;

            wday_diff = tz->__tzrule[i].d - m_wday;
            if (wday_diff < 0)
                wday_diff += DAYSPERWEEK;
            m_day = (tz->__tzrule[i].n - 1) * DAYSPERWEEK + wday_diff;

            while (m_day >= ip[j-1])
                m_day -= DAYSPERWEEK;

            days += m_day;
        }

        /* store the change-over time in GMT form by adding offset */
        tz->__tzrule[i].change = days * SECSPERDAY +
                                 tz->__tzrule[i].s + tz->__tzrule[i].offset;
    }

    tz->__tznorth = (tz->__tzrule[0].change < tz->__tzrule[1].change);

    return 1;
}


/*****************************************************************************
 Prototype    : divoom_timelib_parse_tzset
 Description  : 对时区代号进行解析
 Input        : char *tzenv        eg:  "GMT0BST,M3.5.0/1,M10.5.0"
                divoom_tzinfo_type *tz
 Output       : None
 Return Value :

  History        :
  1.Date         : 2021/4/13
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_timelib_parse_tzset(char *tzenv,divoom_tzinfo_type *tz)
{
    char __tzname_std[11] = {0};
    char __tzname_dst[11] = {0};

    unsigned short hh, mm, ss, m, w, d;
    int sign, n;
    int i, ch;

    if(NULL == tz)
    {
        return;
    }
    if (tzenv == NULL)
    {
        tz->_daylight = 0;
        return;
    }

    /* ignore implementation-specific format specifier */
    if (*tzenv == ':')
        ++tzenv;

    if (sscanf (tzenv, "%10[^0-9,+-]%n", __tzname_std, &n) <= 0)
        return;

    tzenv += n;

    sign = 1;
    if (*tzenv == '-')
    {
        sign = -1;
        ++tzenv;
    }
    else if (*tzenv == '+')
        ++tzenv;

    mm = 0;
    ss = 0;

    if (sscanf (tzenv, "%hu%n:%hu%n:%hu%n", &hh, &n, &mm, &n, &ss, &n) < 1)
        return;
    tz->__tzrule[0].offset = sign * (ss + SECSPERMIN * mm + SECSPERHOUR * hh);
    tzenv += n;
    if (sscanf (tzenv, "%10[^0-9,+-]%n", __tzname_dst, &n) <= 0)
    {
        /* No dst */
        tz->_daylight = 0;
        return;
    }
    else
    {

    }

    tzenv += n;

    /* otherwise we have a dst name, look for the offset */
    sign = 1;
    if (*tzenv == '-')
    {
        sign = -1;
        ++tzenv;
    }
    else if (*tzenv == '+')
        ++tzenv;

    hh = 0;
    mm = 0;
    ss = 0;

    n  = 0;
    if (sscanf (tzenv, "%hu%n:%hu%n:%hu%n", &hh, &n, &mm, &n, &ss, &n) <= 0)
        tz->__tzrule[1].offset = tz->__tzrule[0].offset - 3600;
    else
        tz->__tzrule[1].offset = sign * (ss + SECSPERMIN * mm + SECSPERHOUR * hh);

    tzenv += n;

    for (i = 0; i < 2; ++i)
    {
        if (*tzenv == ',')
            ++tzenv;

        if (*tzenv == 'M')
        {
            if (sscanf (tzenv, "M%hu%n.%hu%n.%hu%n", &m, &n, &w, &n, &d, &n) != 3 ||
                m < 1 || m > 12 || w < 1 || w > 5 || d > 6)
                return;

            tz->__tzrule[i].ch = 'M';
            tz->__tzrule[i].m = m;
            tz->__tzrule[i].n = w;
            tz->__tzrule[i].d = d;

            tzenv += n;
        }
        else
        {
            char *end;
            if (*tzenv == 'J')
            {
                ch = 'J';
                ++tzenv;
            }
            else
                ch = 'D';

            d = strtoul (tzenv, &end, 10);

            /* if unspecified, default to US settings */
            /* From 1987-2006, US was M4.1.0,M10.5.0, but starting in 2007 is
             * M3.2.0,M11.1.0 (2nd Sunday March through 1st Sunday November)  */
            if (end == tzenv)
            {
                if (i == 0)
                {
                    tz->__tzrule[0].ch = 'M';
                    tz->__tzrule[0].m = 3;
                    tz->__tzrule[0].n = 2;
                    tz->__tzrule[0].d = 0;
                }
                else
                {
                    tz->__tzrule[1].ch = 'M';
                    tz->__tzrule[1].m = 11;
                    tz->__tzrule[1].n = 1;
                    tz->__tzrule[1].d = 0;
                }
            }
            else
            {
                tz->__tzrule[i].ch = ch;
                tz->__tzrule[i].d = d;
            }

            tzenv = end;
        }

        /* default time is 02:00:00 am */
        hh = 2;
        mm = 0;
        ss = 0;
        n = 0;

        if (*tzenv == '/')
            sscanf (tzenv, "/%hu%n:%hu%n:%hu%n", &hh, &n, &mm, &n, &ss, &n);

        tz->__tzrule[i].s = ss + SECSPERMIN * mm + SECSPERHOUR  * hh;

        tzenv += n;
    }

    divoom_tzcalc_limits (tz->__tzyear,tz);
    tz->_daylight = tz->__tzrule[0].offset != tz->__tzrule[1].offset;
}

/* Move epoch from 01.01.1970 to 01.03.0000 (yes, Year 0) - this is the first
 * day of a 400-year long "era", right after additional day of leap year.
 * This adjustment is required only for date calculation, so instead of
 * modifying time_t value (which would require 64-bit operations to work
 * correctly) it's enough to adjust the calculated number of days since epoch.
 */
#define EPOCH_ADJUSTMENT_DAYS   719468L
/* year to which the adjustment was made */
#define ADJUSTED_EPOCH_YEAR 0
/* 1st March of year 0 is Wednesday */
#define ADJUSTED_EPOCH_WDAY 3
/* there are 97 leap years in 400-year periods. ((400 - 97) * 365 + 97 * 366) */
#define DAYS_PER_ERA        146097L
/* there are 24 leap years in 100-year periods. ((100 - 24) * 365 + 24 * 366) */
#define DAYS_PER_CENTURY    36524L
/* there is one leap year every 4 years */
#define DAYS_PER_4_YEARS    (3 * 365 + 366)
/* number of days in a non-leap year */
#define DAYS_PER_YEAR       365
/* number of days in January */
#define DAYS_IN_JANUARY     31
/* number of days in non-leap February */
#define DAYS_IN_FEBRUARY    28
/* number of years per era */
#define YEARS_PER_ERA       400

struct tm *divoom_gmtime_r (const time_t * tim_p, struct tm * res)
{
    long days, rem;
    const time_t lcltime = *tim_p;
    int era, weekday, year;
    unsigned erayear, yearday, month, day;
    unsigned long eraday;

    days = lcltime / SECSPERDAY + EPOCH_ADJUSTMENT_DAYS;
    rem = lcltime % SECSPERDAY;
    if (rem < 0)
    {
        rem += SECSPERDAY;
        --days;
    }

    /* compute hour, min, and sec */
    res->tm_hour = (int) (rem / SECSPERHOUR);
    rem %= SECSPERHOUR;
    res->tm_min = (int) (rem / SECSPERMIN);
    res->tm_sec = (int) (rem % SECSPERMIN);

    /* compute day of week */
    if ((weekday = ((ADJUSTED_EPOCH_WDAY + days) % DAYSPERWEEK)) < 0)
        weekday += DAYSPERWEEK;
    res->tm_wday = weekday;

    /* compute year, month, day & day of year */
    /* for description of this algorithm see
     * http://howardhinnant.github.io/date_algorithms.html#civil_from_days */
    era = (days >= 0 ? days : days - (DAYS_PER_ERA - 1)) / DAYS_PER_ERA;
    eraday = days - era * DAYS_PER_ERA;   /* [0, 146096] */
    erayear = (eraday - eraday / (DAYS_PER_4_YEARS - 1) + eraday / DAYS_PER_CENTURY -
               eraday / (DAYS_PER_ERA - 1)) / 365;   /* [0, 399] */
    yearday = eraday - (DAYS_PER_YEAR * erayear + erayear / 4 - erayear / 100);   /* [0, 365] */
    month = (5 * yearday + 2) / 153;  /* [0, 11] */
    day = yearday - (153 * month + 2) / 5 + 1;    /* [1, 31] */
    month += month < 10 ? 2 : -10;
    year = ADJUSTED_EPOCH_YEAR + erayear + era * YEARS_PER_ERA + (month <= 1);

    res->tm_yday = yearday >= DAYS_PER_YEAR - DAYS_IN_JANUARY - DAYS_IN_FEBRUARY ?
                   yearday - (DAYS_PER_YEAR - DAYS_IN_JANUARY - DAYS_IN_FEBRUARY) :
                   yearday + DAYS_IN_JANUARY + DAYS_IN_FEBRUARY + isleap(erayear);
    res->tm_year = year - YEAR_BASE;
    res->tm_mon = month;
    res->tm_mday = day;

    res->tm_isdst = 0;

    return (res);
}


/*****************************************************************************
 Prototype    : divoom_timelib_convert_timezone_r
 Description  : 转换得到对应城市时间（含夏令时间）
 Input        : const time_t *tim_p             UTC时间
                divoom_tzinfo_type *const tz    由时区代号得到的规则信息
                struct tm *res                  得到的对应城市时间
 Output       : None
 Return Value : struct

  History        :
  1.Date         : 2021/4/13
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
struct tm *divoom_timelib_convert_timezone_r(const time_t *tim_p,divoom_tzinfo_type *const tz, struct tm *res)
{
    long offset;
    int hours, mins, secs;
    int year;
    const int *ip;

    res = divoom_gmtime_r(tim_p, res);
//    res = gmtime_r(tim_p, res);
    year = res->tm_year + YEAR_BASE;
    ip = __month_lengths[isleap(year)];
    if (tz->_daylight)
    {
        if (year == tz->__tzyear || divoom_tzcalc_limits (year,tz))
            res->tm_isdst = (tz->__tznorth
                             ? (*tim_p >= tz->__tzrule[0].change
                                && *tim_p < tz->__tzrule[1].change)
                             : (*tim_p >= tz->__tzrule[0].change
                                || *tim_p < tz->__tzrule[1].change));
        else
            res->tm_isdst = -1;
    }
    else
        res->tm_isdst = 0;

    offset = (res->tm_isdst == 1
              ? tz->__tzrule[1].offset
              : tz->__tzrule[0].offset);

    hours = (int) (offset / SECSPERHOUR);
    offset = offset % SECSPERHOUR;

    mins = (int) (offset / SECSPERMIN);
    secs = (int) (offset % SECSPERMIN);

    res->tm_sec -= secs;
    res->tm_min -= mins;
    res->tm_hour -= hours;

    if (res->tm_sec >= SECSPERMIN)
    {
        res->tm_min += 1;
        res->tm_sec -= SECSPERMIN;
    }
    else if (res->tm_sec < 0)
    {
        res->tm_min -= 1;
        res->tm_sec += SECSPERMIN;
    }
    if (res->tm_min >= MINSPERHOUR)
    {
        res->tm_hour += 1;
        res->tm_min -= MINSPERHOUR;
    }
    else if (res->tm_min < 0)
    {
        res->tm_hour -= 1;
        res->tm_min += MINSPERHOUR;
    }
    if (res->tm_hour >= HOURSPERDAY)
    {
        ++res->tm_yday;
        ++res->tm_wday;
        if (res->tm_wday > 6)
            res->tm_wday = 0;
        ++res->tm_mday;
        res->tm_hour -= HOURSPERDAY;
        if (res->tm_mday > ip[res->tm_mon])
        {
            res->tm_mday -= ip[res->tm_mon];
            res->tm_mon += 1;
            if (res->tm_mon == 12)
            {
                res->tm_mon = 0;
                res->tm_year += 1;
                res->tm_yday = 0;
            }
        }
    }
    else if (res->tm_hour < 0)
    {
        res->tm_yday -= 1;
        res->tm_wday -= 1;
        if (res->tm_wday < 0)
            res->tm_wday = 6;
        res->tm_mday -= 1;
        res->tm_hour += 24;
        if (res->tm_mday == 0)
        {
            res->tm_mon -= 1;
            if (res->tm_mon < 0)
            {
                res->tm_mon = 11;
                res->tm_year -= 1;
                res->tm_yday = 364 + isleap(res->tm_year + YEAR_BASE);
            }
            res->tm_mday = ip[res->tm_mon];
        }
    }
    return (res);
}


int divoom_timelib_hourFormat12(int hour)
{
    // the hour for the given time in 12 hour format
    if(hour == 0 )
        return 12; // 12 midnight
    else if(hour  > 12)
        return hour - 12 ;
    else
        return hour ;
}



#define _SEC_IN_MINUTE 60L
#define _SEC_IN_HOUR 3600L
#define _SEC_IN_DAY 86400L

#define _MIN_IN_HOUR 60L
#define _MIN_IN_DAY 1440L

#if 0
static const int DAYS_IN_MONTH[12] =
{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
#endif

//#define _DAYS_IN_MONTH(x) ((x == 1) ? days_in_feb : DAYS_IN_MONTH[x])

static const int _DAYS_BEFORE_MONTH[12] =
{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

#define _ISLEAP(y) (((y) % 4) == 0 && (((y) % 100) != 0 || (((y)+1900) % 400) == 0))
#define _DAYS_IN_YEAR(year) (_ISLEAP(year) ? 366 : 365)


time_t divoom_timelib_std_mktime(struct tm *tim_p)
{
    time_t tim = 0;
    long days = 0;
    int year;
//  __tzinfo_type *tz = __gettzinfo ();

    /* validate structure */
//  validate_structure (tim_p);

    /* compute hours, minutes, seconds */
    tim += tim_p->tm_sec + (tim_p->tm_min * _SEC_IN_MINUTE) +
           (tim_p->tm_hour * _SEC_IN_HOUR);

    /* compute days in year */
    days += tim_p->tm_mday - 1;
    days += _DAYS_BEFORE_MONTH[tim_p->tm_mon];
    if (tim_p->tm_mon > 1 && _DAYS_IN_YEAR (tim_p->tm_year) == 366)
        days++;

    /* compute day of the year */
    tim_p->tm_yday = days;

    if (tim_p->tm_year > 10000 || tim_p->tm_year < -10000)
        return (time_t) -1;

    /* compute days in other years */
    if ((year = tim_p->tm_year) > 70)
    {
        for (year = 70; year < tim_p->tm_year; year++)
            days += _DAYS_IN_YEAR (year);
    }
    else if (year < 70)
    {
        for (year = 69; year > tim_p->tm_year; year--)
            days -= _DAYS_IN_YEAR (year);
        days -= _DAYS_IN_YEAR (year);
    }

    /* compute total seconds */
    tim += (time_t)days * _SEC_IN_DAY;
    return tim;

}




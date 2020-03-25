#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/Time.h>
#include <core/SystemInfo.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

TEST(Core_Time, timedetails)
{
    Time time (Time::Now());
    std::string timeString;
    time.ToString(timeString);
    time.ToRFC1123();
    time.Format("%d-%m-%Y");
    time.MilliSeconds();
    time.Seconds();
    time.Minutes();
    time.Hours();
    time.Day();
    time.Month();
    time.Year();
    time.MonthName();
    time.WeekDayName();
    time.DayOfWeek();
    time.DayOfYear();
    time.Ticks();
    time.NTPTime();
    time.DifferenceFromGMTSeconds();
    time.FromRFC1123(timeString); 
    
    Time::Now().ToRFC1123();
    time.Add (2000);
    SystemInfo::Instance().SetTime(time);
    Time::Now().ToRFC1123();

    time.Sub (2000);
    SystemInfo::Instance().SetTime(time);
    Time::Now().ToRFC1123();

    Time t1 =  Time(2018,7,23,11,30,23,21,true);
    Time t2 =  Time(2018,7,23,11,30,23,21,false);
    struct tm setTime;
    ::memcpy(&setTime, &(time.Handle()), sizeof (setTime));
}

TEST(Core_Time, from_standards)
{
    Time time (Time::Now());
    std::string timeString;
    time.ToString(timeString);

    time.FromString(timeString,true);
    time.FromString(timeString,false);

    time.FromANSI(timeString,true);    
    time.FromANSI(timeString,false);    

    time.FromRFC1036(timeString);
    time.FromRFC1036("19941105T08:49:37.123+06:45"); 
    time.FromRFC1036("1994-11-06T08:49:37.123+06:45"); 
    time.FromRFC1036("1994-11-Jan-T08:12:49:37.123+06:45"); 
    time.FromRFC1036("1994-11-Jan-T08:12:49:37 123+06:45"); 
    time.FromRFC1036("1994-11-Jan-08:49:07+06:45");
    time.FromRFC1036("1994Jan07T08:49:37.123+06:45"); 
    time.FromRFC1036("1994-Jan-09T08:49:37.123+06:45");
 
    time.FromISO8601(timeString);
    time.FromISO8601("1994-11-06T08:49:37.123+06:45"); 
    time.FromISO8601("1994-12-06T08:49:37.123+06:45"); 
    time.FromISO8601("1994-13-06T08:49:37.123+06:45"); 
    time.FromISO8601("1994-11-06T08:49:37");
    time.FromISO8601("1994-12-06T08:49:37.123 06:45");
    time.FromISO8601("1994-11-06T08:49:37.123-06:45"); 
    time.FromISO8601("1994-11-06T08:49:37."); 
    time.FromISO8601("1994-11-06T08:49:37.123+06 45"); 
    time.FromISO8601("1994-11-06T08:49:37.123+24:45"); 
    time.FromISO8601("1994-11-06T08:49:37.123+24:4"); 
    time.FromISO8601("1994-11-06T08:49:37.123+06:45:"); 
    time.FromISO8601("1994-11-06T08:49:37.123+"); 
    time.FromISO8601("1994-11-06T08:49:37.123a"); 

    time.FromANSI("1994Jan06T08:49:37.123+06:45",true); 
    time.FromANSI("1994Jan06T08:49:37.803+06:45",true); 
    time.FromANSI("1994Jan06T0a:49:37.123+06:45",true); 
    time.FromANSI("1994Feb06T08:4a:37.123+06:45",true); 
    time.FromANSI("1994Mar06T08:49:5a.123+06:45",true); 
    time.FromANSI("1994Mara6T08:49:5a.123+06:45",true); 
    time.FromANSI("1994Mar0aT08:49:5a.123+06:45",true); 
}

TEST(Core_Time, to_standards)
{
    Time time (Time::Now());
    time.ToRFC1123(true);
    time.ToRFC1123(false);
    Time().ToRFC1123(true);

    time.ToISO8601();
    time.ToISO8601(true);
    time.ToISO8601(false);
    Time().ToISO8601(true);
}

TEST(Core_Time, FromRFC1123)
{
    Time t1 =  Time(2018,1,23,11,30,23,21,false);
    std::string timeString;
    t1.ToString(timeString,false);
    t1.FromRFC1123(timeString);
    EXPECT_STREQ(timeString.c_str() ,_T("Tue, 23 Jan 2018 11:30:23 GMT"));
 
    t1 =  Time(2018,2,23,11,30,23,21,false);
    t1.ToString(timeString,false);
    t1.FromRFC1123(timeString); 
    EXPECT_STREQ(timeString.c_str() ,_T("Fri, 23 Feb 2018 11:30:23 GMT"));

    t1 =  Time(2018,3,23,11,30,23,21,false);
    t1.ToString(timeString,false);
    t1.FromRFC1123(timeString);
    EXPECT_STREQ(timeString.c_str() ,_T("Fri, 23 Mar 2018 11:30:23 GMT"));

    t1 =  Time(2018,4,23,11,30,23,21,false);
    t1.ToString(timeString,false);
    t1.FromRFC1123(timeString);
    EXPECT_STREQ(timeString.c_str() ,_T("Mon, 23 Apr 2018 11:30:23 GMT"));

    t1 =  Time(2018,5,23,11,30,23,21,false);
    t1.ToString(timeString,false);
    t1.FromRFC1123(timeString);
    EXPECT_STREQ(timeString.c_str() ,_T("Wed, 23 May 2018 11:30:23 GMT"));
     
    t1 =  Time(2018,6,23,11,30,23,21,false);
    t1.ToString(timeString,false);
    t1.FromRFC1123(timeString); 
    EXPECT_STREQ(timeString.c_str() ,_T("Sat, 23 Jun 2018 11:30:23 GMT"));

    t1 =  Time(2018,7,23,11,30,23,21,false);
    t1.ToString(timeString,false);
    t1.FromRFC1123(timeString);
    EXPECT_STREQ(timeString.c_str() ,_T("Mon, 23 Jul 2018 11:30:23 GMT"));

    t1 =  Time(2018,8,23,11,30,23,21,false);
    t1.ToString(timeString,false);
    t1.FromRFC1123(timeString); 
    EXPECT_STREQ(timeString.c_str() ,_T("Thu, 23 Aug 2018 11:30:23 GMT"));

    t1 =  Time(2018,9,23,11,30,23,21,false);
    t1.ToString(timeString,false);
    t1.FromRFC1123(timeString); 
    EXPECT_STREQ(timeString.c_str() ,_T("Sun, 23 Sep 2018 11:30:23 GMT"));
    
    t1 =  Time(2018,10,23,11,30,23,21,false);
    t1.ToString(timeString,false);
    t1.FromRFC1123(timeString); 
    EXPECT_STREQ(timeString.c_str() ,_T("Tue, 23 Oct 2018 11:30:23 GMT"));
    
    t1 =  Time(2018,11,23,11,30,23,21,false);
    t1.ToString(timeString,false);
    t1.FromRFC1123(timeString); 
    EXPECT_STREQ(timeString.c_str() ,_T("Fri, 23 Nov 2018 11:30:23 GMT"));

    t1 =  Time(2018,12,23,11,30,23,21,false);
    t1.ToString(timeString,false);
    t1.FromRFC1123(timeString); 
    EXPECT_STREQ(timeString.c_str() ,_T("Sun, 23 Dec 2018 11:30:23 GMT"));

    t1 =  Time(70,12,23,11,30,23,21,false);
    t1.ToString(timeString,false);
    t1.FromRFC1123(timeString);

    t1 =  Time(80,12,23,11,30,23,21,false);
    t1.ToString(timeString,false);
    t1.FromRFC1123(timeString);

    t1.FromRFC1123("1994Jan06T08:49:37.123+06:45");
    t1.FromRFC1123("1994Jan0aT08:49:37.123+06:45");
}

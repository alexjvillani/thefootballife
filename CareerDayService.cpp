#include "pch.h"
#include "CareerDayService.h"

namespace
{
    bool IsLeapYear(int year)
    {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }

    int DaysInMonth(int year, int month)
    {
        static const int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        if (month == 2 && IsLeapYear(year))
        {
            return 29;
        }
        return days[month - 1];
    }

    void AddOneDay(SimpleDate& date)
    {
        date.Day++;
        if (date.Day > DaysInMonth(date.Year, date.Month))
        {
            date.Day = 1;
            date.Month++;
            if (date.Month > 12)
            {
                date.Month = 1;
                date.Year++;
            }
        }
    }

    // Zeller's congruence. Returns 0 = Saturday, 1 = Sunday, 2 = Monday ... 6 = Friday.
    int ZellerDayIndex(const SimpleDate& date)
    {
        int day = date.Day;
        int month = date.Month;
        int year = date.Year;
        if (month < 3)
        {
            month += 12;
            year -= 1;
        }
        int k = year % 100;
        int j = year / 100;
        return (day + (13 * (month + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
    }

    DayPhase WeekdayFromDate(const SimpleDate& date)
    {
        static const DayPhase map[] =
        {
            DayPhase::Saturday, // 0
            DayPhase::Sunday,   // 1
            DayPhase::Monday,   // 2
            DayPhase::Tuesday,  // 3
            DayPhase::Wednesday,// 4
            DayPhase::Thursday, // 5
            DayPhase::Friday    // 6
        };
        return map[ZellerDayIndex(date)];
    }

    DayPhase NextDayPhase(DayPhase day)
    {
        switch (day)
        {
        case DayPhase::Monday:    return DayPhase::Tuesday;
        case DayPhase::Tuesday:   return DayPhase::Wednesday;
        case DayPhase::Wednesday: return DayPhase::Thursday;
        case DayPhase::Thursday:  return DayPhase::Friday;
        case DayPhase::Friday:    return DayPhase::Saturday;
        case DayPhase::Saturday:  return DayPhase::Sunday;
        case DayPhase::Sunday:    return DayPhase::Monday;
        }
        return DayPhase::Monday;
    }
}

namespace CareerDayService
{
    void InitializeSeason(int startYear)
    {
        // Season opens the last Friday of March and runs through to the end
        // of August - adjust these two anchors if your fixture length/finals
        // structure changes.
        GameState::SeasonStartDate = SimpleDate{ startYear, 3, 27 };
        GameState::SeasonEndDate = SimpleDate{ startYear, 8, 30 };

        GameState::CurrentDate = GameState::SeasonStartDate;
        GameState::CurrentDay = WeekdayFromDate(GameState::CurrentDate);
        GameState::CurrentWeek = 1;
    }

    bool AdvanceDay()
    {
        AddOneDay(GameState::CurrentDate);
        GameState::CurrentDay = NextDayPhase(GameState::CurrentDay);

        // A new round's player-choice cycle begins on Monday.
        if (GameState::CurrentDay == DayPhase::Monday)
        {
            GameState::CurrentWeek++;
        }

        // Deliberately does NOT trigger match simulation itself -
        // CareerHubPage owns m_fixtures/m_teamStats and its own
        // ApplyWeekSimulation()/SimulateWeekMatches(). The caller should
        // check this return value and run those when it's true.
        return GameState::CurrentDay == DayPhase::Saturday;
    }

    std::wstring GetDayPhaseName(DayPhase day)
    {
        switch (day)
        {
        case DayPhase::Monday:    return L"Monday";
        case DayPhase::Tuesday:   return L"Tuesday";
        case DayPhase::Wednesday: return L"Wednesday";
        case DayPhase::Thursday:  return L"Thursday";
        case DayPhase::Friday:    return L"Friday";
        case DayPhase::Saturday:  return L"Saturday";
        case DayPhase::Sunday:    return L"Sunday";
        }
        return L"";
    }

    std::wstring GetTodayLabel()
    {
        static const wchar_t* monthNames[] =
        {
            L"January", L"February", L"March", L"April", L"May", L"June",
            L"July", L"August", L"September", L"October", L"November", L"December"
        };

        return GetDayPhaseName(GameState::CurrentDay) + L", " +
            std::to_wstring(GameState::CurrentDate.Day) + L" " +
            monthNames[GameState::CurrentDate.Month - 1] + L" " +
            std::to_wstring(GameState::CurrentDate.Year);
    }

    std::wstring GetDayFlavorText(DayPhase day)
    {
        switch (day)
        {
        case DayPhase::Monday:
            return L"Recovery from the weekend, then the first team run of the week. Set your intentions early.";
        case DayPhase::Tuesday:
            return L"Main training day - the heaviest work of the week happens here.";
        case DayPhase::Wednesday:
            return L"Midweek grind. Balancing football with school, work, and everything else competing for your time.";
        case DayPhase::Thursday:
            return L"Taper begins. Sharpen up your touch, but don't leave your best form in the shed before Saturday.";
        case DayPhase::Friday:
            return L"Final preparations. Whatever isn't locked in tonight won't be ready for matchday.";
        case DayPhase::Saturday:
            return L"Matchday. Everything this week has built to today.";
        case DayPhase::Sunday:
            return L"A free day to recover, reflect, and reset before the week begins again.";
        }
        return L"";
    }

    bool IsMatchday()
    {
        return GameState::CurrentDay == DayPhase::Saturday;
    }

    bool IsSeasonComplete()
    {
        return GameState::CurrentDate >= GameState::SeasonEndDate;
    }
}
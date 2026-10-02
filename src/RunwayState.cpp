//
// METAR runway state group decoder
//
#include "RunwayState.h"

#include <cctype>
#include <cstring>

using namespace Storage_B::Weather;

namespace
{
  inline bool two_digits(const char *s)
  {
    return isdigit(static_cast<unsigned char>(s[0])) && isdigit(static_cast<unsigned char>(s[1]));
  }

  inline int two_digit_value(const char *s)
  {
    return (s[0] - '0') * 10 + (s[1] - '0');
  }

  // BB: friction coefficient 01..90, braking action 91..95, 99 unreliable, // not reported
  void decode_friction(const char *bb, RunwayState& rs)
  {
    if (!two_digits(bb))
    {
      return;
    }

    int v = two_digit_value(bb);
    if (v >= 1 && v <= 90)
    {
      rs.friction = v / 100.0;
    }
    else if ((v >= 91 && v <= 95) || v == 99)
    {
      rs.braking_action = static_cast<RunwayState::braking>(v);
    }
  }
}

std::optional<RunwayState> RunwayState::Parse(const char *str)
{
  if (!str)
  {
    return std::nullopt;
  }

  if (!strcmp(str, "SNOCLO") || !strcmp(str, "R/SNOCLO"))
  {
    RunwayState rs;
    rs.closed_snow = true;
    rs.all_runways = true;
    return rs;
  }

  // R + two digits + optional L/C/R + '/'
  if (str[0] != 'R' || !two_digits(str + 1))
  {
    return std::nullopt;
  }

  const char *p = str + 3;
  if (*p == 'L' || *p == 'C' || *p == 'R')
  {
    p++;
  }

  if (*p != '/')
  {
    return std::nullopt;
  }

  RunwayState rs;
  rs.runway.assign(str + 1, p - (str + 1));
  rs.all_runways = (rs.runway == "88");
  rs.repeated = (rs.runway == "99");

  const char *g = p + 1;

  // R24/CLRD62
  if (!strncmp(g, "CLRD", 4) && strlen(g) == 6)
  {
    rs.cleared = true;
    decode_friction(g + 4, rs);
    return rs;
  }

  // ERCeeBB: exactly six characters, each a digit or '/'
  if (strlen(g) != 6)
  {
    return std::nullopt;
  }

  for (int i = 0; i < 6; i++)
  {
    if (!isdigit(static_cast<unsigned char>(g[i])) && g[i] != '/')
    {
      return std::nullopt;
    }
  }

  if (isdigit(static_cast<unsigned char>(g[0])))
  {
    rs.deposit_type = static_cast<deposit>(g[0] - '0');
  }

  switch (g[1])
  {
    case '1': rs.coverage_max_pct = 10; break;
    case '2': rs.coverage_max_pct = 25; break;
    case '5': rs.coverage_max_pct = 50; break;
    case '9': rs.coverage_max_pct = 100; break;
    default: break;
  }

  if (two_digits(g + 2))
  {
    int ee = two_digit_value(g + 2);
    if (ee <= 90)
    {
      rs.depth_mm = ee;
    }
    else if (ee >= 92 && ee <= 98)
    {
      rs.depth_mm = (ee - 90) * 50;   // 92 = 10 cm ... 98 = 40 cm or more
    }
    else if (ee == 99)
    {
      rs.not_operational = true;
    }
  }

  decode_friction(g + 4, rs);

  return rs;
}

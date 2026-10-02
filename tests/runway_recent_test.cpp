//
// Recent weather, runway state and QFE tests
//

#include "Metar.h"
#include "Phenom.h"
#include "RunwayState.h"

#include <cmath>
#include <string>

#include <boost/test/unit_test.hpp>

using namespace Storage_B::Weather;

BOOST_AUTO_TEST_SUITE(RecentWeatherTests)

BOOST_AUTO_TEST_CASE(recent_rain_is_not_current)
{
  auto metar = Metar::Create("METAR UWGG 020500Z 27003MPS 9999 OVC020 01/M01 Q1010 RERA NOSIG");

  BOOST_CHECK(metar->NumPhenomena() == 0);
  BOOST_CHECK(metar->NumRecentPhenomena() == 1);
  BOOST_CHECK(metar->RecentPhenomenon(0).NumPhenom() == 1);
  BOOST_CHECK(metar->RecentPhenomenon(0)[0] == Phenom::phenom::RAIN);
  BOOST_CHECK(!metar->RecentPhenomenon(0).Freezing());
}

BOOST_AUTO_TEST_CASE(recent_freezing_drizzle)
{
  auto metar = Metar::Create("METAR UUEE 021030Z 18004MPS 6000 -SN OVC008 M01/M02 Q1003 REFZDZ R06R/790235");

  BOOST_CHECK(metar->NumPhenomena() == 1);
  BOOST_CHECK(metar->Phenomenon(0)[0] == Phenom::phenom::SNOW);
  BOOST_CHECK(metar->NumRecentPhenomena() == 1);
  BOOST_CHECK(metar->RecentPhenomenon(0).Freezing());
  BOOST_CHECK(metar->RecentPhenomenon(0)[0] == Phenom::phenom::DRIZZLE);
}

BOOST_AUTO_TEST_CASE(recent_shower_and_thunderstorm)
{
  auto metar = Metar::Create("METAR UWWW 021200Z 24006MPS CAVOK 18/09 Q1015 RESHRA RETS");

  BOOST_CHECK(metar->NumRecentPhenomena() == 2);
  BOOST_CHECK(metar->RecentPhenomenon(0).NumPhenom() == 2);
  BOOST_CHECK(metar->RecentPhenomenon(0)[0] == Phenom::phenom::SHOWER);
  BOOST_CHECK(metar->RecentPhenomenon(0)[1] == Phenom::phenom::RAIN);
  BOOST_CHECK(metar->RecentPhenomenon(1).ThunderStorm());
}

BOOST_AUTO_TEST_CASE(recent_out_of_range)
{
  auto metar = Metar::Create("METAR UWGG 020500Z 27003MPS 9999 OVC020 01/M01 Q1010");

  BOOST_CHECK(metar->NumRecentPhenomena() == 0);
  BOOST_CHECK(metar->RecentPhenomenon(5).NumPhenom() == 0);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(RunwayStateTests)

BOOST_AUTO_TEST_CASE(ice_patches)
{
  auto rs = RunwayState::Parse("R06R/790235");

  BOOST_CHECK(rs.has_value());
  BOOST_CHECK(rs->runway == "06R");
  BOOST_CHECK(rs->deposit_type == RunwayState::deposit::ICE);
  BOOST_CHECK(rs->coverage_max_pct == 100);
  BOOST_CHECK(rs->depth_mm == 2);
  BOOST_CHECK(rs->friction.has_value());
  BOOST_CHECK(std::fabs(*rs->friction - 0.35) < 1e-9);
  BOOST_CHECK(rs->Icy());
  BOOST_CHECK(!rs->all_runways);
}

BOOST_AUTO_TEST_CASE(all_runways_frost)
{
  auto rs = RunwayState::Parse("R88/390050");

  BOOST_CHECK(rs.has_value());
  BOOST_CHECK(rs->all_runways);
  BOOST_CHECK(rs->deposit_type == RunwayState::deposit::RIME_FROST);
  BOOST_CHECK(rs->depth_mm == 0);
  BOOST_CHECK(std::fabs(*rs->friction - 0.50) < 1e-9);
  BOOST_CHECK(rs->Icy());
}

BOOST_AUTO_TEST_CASE(wet_snow_deep_braking_medium)
{
  auto rs = RunwayState::Parse("R24/559393");

  BOOST_CHECK(rs.has_value());
  BOOST_CHECK(rs->deposit_type == RunwayState::deposit::WET_SNOW);
  BOOST_CHECK(rs->coverage_max_pct == 50);
  BOOST_CHECK(rs->depth_mm == 150);
  BOOST_CHECK(!rs->friction.has_value());
  BOOST_CHECK(rs->braking_action == RunwayState::braking::MEDIUM);
  BOOST_CHECK(!rs->Icy());
}

BOOST_AUTO_TEST_CASE(not_reported_fields)
{
  auto rs = RunwayState::Parse("R12L/2/////");

  BOOST_CHECK(rs.has_value());
  BOOST_CHECK(rs->deposit_type == RunwayState::deposit::WET);
  BOOST_CHECK(!rs->coverage_max_pct.has_value());

  BOOST_CHECK(!RunwayState::Parse("R12L/2//////").has_value());   // seven characters

  rs = RunwayState::Parse("R12L/29////");
  BOOST_CHECK(rs.has_value());
  BOOST_CHECK(rs->runway == "12L");
  BOOST_CHECK(rs->deposit_type == RunwayState::deposit::WET);
  BOOST_CHECK(rs->coverage_max_pct == 100);
  BOOST_CHECK(!rs->depth_mm.has_value());
  BOOST_CHECK(!rs->friction.has_value());
  BOOST_CHECK(!rs->braking_action.has_value());
}

BOOST_AUTO_TEST_CASE(runway_not_operational)
{
  auto rs = RunwayState::Parse("R30/559999");

  BOOST_CHECK(rs.has_value());
  BOOST_CHECK(rs->not_operational);
  BOOST_CHECK(!rs->depth_mm.has_value());
  BOOST_CHECK(rs->braking_action == RunwayState::braking::UNRELIABLE);
}

BOOST_AUTO_TEST_CASE(cleared)
{
  auto rs = RunwayState::Parse("R24/CLRD62");

  BOOST_CHECK(rs.has_value());
  BOOST_CHECK(rs->cleared);
  BOOST_CHECK(!rs->deposit_type.has_value());
  BOOST_CHECK(std::fabs(*rs->friction - 0.62) < 1e-9);
}

BOOST_AUTO_TEST_CASE(snoclo)
{
  auto rs = RunwayState::Parse("R/SNOCLO");
  BOOST_CHECK(rs.has_value());
  BOOST_CHECK(rs->closed_snow);

  rs = RunwayState::Parse("SNOCLO");
  BOOST_CHECK(rs.has_value());
  BOOST_CHECK(rs->closed_snow);
}

BOOST_AUTO_TEST_CASE(repeated_report)
{
  auto rs = RunwayState::Parse("R99/411594");

  BOOST_CHECK(rs.has_value());
  BOOST_CHECK(rs->repeated);
  BOOST_CHECK(rs->deposit_type == RunwayState::deposit::DRY_SNOW);
  BOOST_CHECK(rs->coverage_max_pct == 10);
  BOOST_CHECK(rs->depth_mm == 15);
  BOOST_CHECK(rs->braking_action == RunwayState::braking::MEDIUM_GOOD);
}

BOOST_AUTO_TEST_CASE(rvr_is_not_runway_state)
{
  BOOST_CHECK(!RunwayState::Parse("R24/P1500N").has_value());
  BOOST_CHECK(!RunwayState::Parse("R06/0600V1200U").has_value());
  BOOST_CHECK(!RunwayState::Parse("R24/1200").has_value());
  BOOST_CHECK(!RunwayState::Parse("RERA").has_value());
  BOOST_CHECK(!RunwayState::Parse("RA").has_value());
  BOOST_CHECK(!RunwayState::Parse(nullptr).has_value());
}

BOOST_AUTO_TEST_CASE(runway_states_in_metar)
{
  auto metar = Metar::Create(
    "METAR UUEE 021030Z 18004MPS 6000 -SN OVC008 M01/M02 Q1003 "
    "R06R/790235 R06L/590240 NOSIG RMK QFE745/0994");

  BOOST_CHECK(metar->NumRunwayStates() == 2);
  BOOST_CHECK(metar->RunwayStateAt(0)->runway == "06R");
  BOOST_CHECK(metar->RunwayStateAt(0)->Icy());
  BOOST_CHECK(metar->RunwayStateAt(1)->deposit_type == RunwayState::deposit::WET_SNOW);
  BOOST_CHECK(metar->RunwayStateAt(2) == nullptr);

  // runway groups must not leak into phenomena or cloud layers
  BOOST_CHECK(metar->NumPhenomena() == 1);
  BOOST_CHECK(metar->NumCloudLayers() == 1);
}

BOOST_AUTO_TEST_CASE(rvr_in_metar_is_ignored)
{
  auto metar = Metar::Create("METAR UUDD 020600Z 00000MPS 0400 R14R/0550N FG VV002 02/02 Q1018");

  BOOST_CHECK(metar->NumRunwayStates() == 0);
  BOOST_CHECK(metar->NumPhenomena() == 1);
  BOOST_CHECK(metar->Phenomenon(0)[0] == Phenom::phenom::FOG);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(QfeAndFeedTests)

BOOST_AUTO_TEST_CASE(qfe_mmhg_and_hpa)
{
  auto metar = Metar::Create("METAR UWGG 020500Z 27003MPS 9999 OVC020 01/M01 Q1010 NOSIG RMK QFE748/0997");

  BOOST_CHECK(metar->QFEmmHg() == 748);
  BOOST_CHECK(metar->QFEhPa() == 997);
}

BOOST_AUTO_TEST_CASE(qfe_mmhg_only)
{
  auto metar = Metar::Create("METAR UWGG 020500Z 27003MPS 9999 OVC020 01/M01 Q1010 RMK QFE748");

  BOOST_CHECK(metar->QFEmmHg() == 748);
  BOOST_CHECK(!metar->QFEhPa().has_value());
}

BOOST_AUTO_TEST_CASE(qfe_only_in_remarks)
{
  auto metar = Metar::Create("METAR UWGG 020500Z 27003MPS 9999 OVC020 01/M01 Q1010");

  BOOST_CHECK(!metar->QFEmmHg().has_value());
}

BOOST_AUTO_TEST_CASE(noaa_file_with_header_line)
{
  // tgftp.nws.noaa.gov/.../UWGG.TXT: date line, newline, report, trailing newline
  auto metar = Metar::Create("2026/10/02 05:00\nUWGG 020500Z 27003MPS 9999 -FZRA OVC020 M01/M02 Q1010 R88/790240 NOSIG\n");

  BOOST_CHECK(metar->ICAO() == std::string("UWGG"));
  BOOST_CHECK(metar->Day() == 2);
  BOOST_CHECK(metar->Temperature() == -1);
  BOOST_CHECK(metar->NumPhenomena() == 1);
  BOOST_CHECK(metar->Phenomenon(0).Freezing());
  BOOST_CHECK(metar->Phenomenon(0).Intensity() == Phenom::intensity::LIGHT);
  BOOST_CHECK(metar->NumRunwayStates() == 1);
  BOOST_CHECK(metar->RunwayStateAt(0)->all_runways);
}

BOOST_AUTO_TEST_CASE(trailing_equals_sign)
{
  auto metar = Metar::Create("METAR UWGG 020500Z 27003MPS 9999 OVC020 01/M01 Q1010 NOSIG=");

  BOOST_CHECK(metar->AltimeterQ() == 1010);
  BOOST_CHECK(metar->NumPhenomena() == 0);
}

BOOST_AUTO_TEST_SUITE_END()

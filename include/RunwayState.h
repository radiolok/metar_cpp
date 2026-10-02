//
// METAR runway state group decoder (ICAO Annex 3 / WMO FM 15 "R..//ERCeeBB")
//
#ifndef STORAGE_B_WEATHER_RUNWAY_STATE_H_
#define STORAGE_B_WEATHER_RUNWAY_STATE_H_

#include <optional>
#include <string>

namespace Storage_B
{
  namespace Weather
  {
    /**
     * @brief State of a runway as reported in a METAR runway state group.
     *
     * Supported forms:
     *  - R24/590235, R06L/29//95, R88/290050 (88 = all runways, 99 = repeated report)
     *  - R24/CLRD62 (contamination cleared, friction 0.62)
     *  - R/SNOCLO or SNOCLO (aerodrome closed due to snow)
     */
    struct RunwayState
    {
      /// Runway deposit, digit E of the group.
      enum class deposit
      {
        CLEAR_DRY = 0,
        DAMP = 1,
        WET = 2,              // wet or water patches
        RIME_FROST = 3,       // rime or frost covered
        DRY_SNOW = 4,
        WET_SNOW = 5,
        SLUSH = 6,
        ICE = 7,
        COMPACTED_SNOW = 8,   // compacted or rolled snow
        FROZEN_RUTS = 9       // frozen ruts or ridges
      };

      /// Estimated braking action, codes 91..95 and 99 in place of the friction coefficient.
      enum class braking
      {
        POOR = 91,
        MEDIUM_POOR = 92,
        MEDIUM = 93,
        MEDIUM_GOOD = 94,
        GOOD = 95,
        UNRELIABLE = 99
      };

      std::string runway;            ///< Designator as reported: "24", "06L", "88", "99", "" for R/SNOCLO
      bool all_runways = false;      ///< Designator 88 or SNOCLO
      bool repeated = false;         ///< Designator 99: previous report repeated
      bool cleared = false;          ///< CLRD: contamination ceased to exist
      bool closed_snow = false;      ///< SNOCLO: aerodrome closed due to snow
      bool not_operational = false;  ///< Depth code 99: runway not operational

      std::optional<deposit> deposit_type;
      std::optional<int> coverage_max_pct;  ///< Upper bound of contaminated area: 10, 25, 50 or 100 %
      std::optional<int> depth_mm;          ///< Deposit depth; codes 92..98 decoded as 100..400 mm
      std::optional<double> friction;       ///< Measured friction coefficient 0.01..0.90
      std::optional<braking> braking_action;

      /// True when the deposit is ice, rime/frost or frozen ruts.
      bool Icy() const
      {
        return deposit_type == deposit::ICE
            || deposit_type == deposit::RIME_FROST
            || deposit_type == deposit::FROZEN_RUTS;
      }

      /**
       * @brief Parses one METAR token.
       * @return The decoded state, or std::nullopt if the token is not a runway state group.
       *
       * Runway visual range groups such as R24/P1500N or R06/0600V1200U are not runway
       * state groups and return std::nullopt.
       */
      static std::optional<RunwayState> Parse(const char *str);
    };
  }
}

#endif

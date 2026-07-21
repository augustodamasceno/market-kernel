/* Market Kernel : Network IP Configuration and Market Data Feed Endpoints
 *
 * Copyright (c) 2026, Augusto Damasceno. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * See (https://github.com/augustodamasceno/marketdata)
 */

/**
 * @file mk_ips.hpp
 * @brief Centralized IP address and port configuration for all B3 market data 
 *        feeds and network infrastructure within the MarketKernel trading system.
 *
 * @details This header serves as the single source of truth for all network 
 * endpoint configurations across multiple production, redundancy, and testing feeds:
 * - **B3 UMDF IPS Equities** (Production): FEED A' + FEED B', channels 80-98, 233.252.8.0/24 + 233.252.9.0/24
 * - **B3 UMDF Derivatives** (Production): FEED A' + FEED B', channels 68-78, 233.252.8.0/24 + 233.252.9.0/24
 * - **B3 UMDF Disaster Recovery Derivatives**: FEED C', channels 68-78, 233.252.15.0/24
 * - **B3 UMDF Disaster Recovery Equities**: FEED C', channels 80-98, 233.252.15.0/24
 * - **B3 UMDF Certification Derivatives**: FEED A + FEED B, channels 68-78, 233.205.192.0/24 (pre-production)
 * - **B3 UMDF Certification Equities**: FEED A + FEED B, channels 80-98, 233.205.192.0/24 (pre-production)
 *
 * All configurations use \constexpr\ static data members for zero runtime 
 * initialization overhead. IP addresses and ports are immutable after compilation.
 */

#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace marketkernel {

class IpsConfig {
public:
  struct UdpEndpoint {
    std::string_view address;
    uint16_t port;
  };

  struct Channel {
    uint16_t channel_id;
    std::string_view type;
    UdpEndpoint snapshot_recovery;
    UdpEndpoint incrementals;
    UdpEndpoint instrument_definition;
  };

  // ========================================================================
  // UMDF IPS Equities: Production (FEED A' on 233.252.8.x, FEED B' on 233.252.9.x)
  // ========================================================================

  class UmdfIpsEquities {
  public:
    /// FEED A' (Primary) - 233.252.8.0/24, ports 30xxx/20xxx/10xxx
    static constexpr std::array<Channel, 9> FEED_A_CHANNELS{{
      {80, "MBO", {"233.252.8.106", 30080}, {"233.252.8.107", 20080}, {"233.252.8.108", 10080}},
      {82, "MBO", {"233.252.8.109", 30082}, {"233.252.8.110", 20082}, {"233.252.8.111", 10082}},
      {84, "MBO", {"233.252.8.112", 30084}, {"233.252.8.113", 20084}, {"233.252.8.114", 10084}},
      {86, "MBO", {"233.252.8.115", 30086}, {"233.252.8.116", 20086}, {"233.252.8.117", 10086}},
      {88, "MBO", {"233.252.8.118", 30088}, {"233.252.8.119", 20088}, {"233.252.8.120", 10088}},
      {90, "MBO", {"233.252.8.121", 30090}, {"233.252.8.122", 20090}, {"233.252.8.123", 10090}},
      {92, "MBO", {"233.252.8.124", 30092}, {"233.252.8.125", 20092}, {"233.252.8.126", 10092}},
      {94, "MBO", {"233.252.8.127", 30094}, {"233.252.8.128", 20094}, {"233.252.8.129", 10094}},
      {98, "MBO", {"233.252.8.130", 30098}, {"233.252.8.131", 20098}, {"233.252.8.132", 10098}}
    }};

    /// FEED B' (Redundant) - 233.252.9.0/24, incrementals only
    static constexpr std::array<Channel, 9> FEED_B_CHANNELS{{
      {80, "MBO", {"", 0}, {"233.252.9.107", 20080}, {"", 0}},
      {82, "MBO", {"", 0}, {"233.252.9.110", 20082}, {"", 0}},
      {84, "MBO", {"", 0}, {"233.252.9.113", 20084}, {"", 0}},
      {86, "MBO", {"", 0}, {"233.252.9.116", 20086}, {"", 0}},
      {88, "MBO", {"", 0}, {"233.252.9.119", 20088}, {"", 0}},
      {90, "MBO", {"", 0}, {"233.252.9.122", 20090}, {"", 0}},
      {92, "MBO", {"", 0}, {"233.252.9.125", 20092}, {"", 0}},
      {94, "MBO", {"", 0}, {"233.252.9.128", 20094}, {"", 0}},
      {98, "MBO", {"", 0}, {"233.252.9.131", 20098}, {"", 0}}
    }};

    static constexpr std::string_view FEED_A_RP_ADDRESS{"200.19.57.252"};
    static constexpr std::string_view FEED_A_MULTICAST_RANGE{"233.252.8.0/24"};
    static constexpr std::string_view FEED_B_RP_ADDRESS{"200.19.58.252"};
    static constexpr std::string_view FEED_B_MULTICAST_RANGE{"233.252.9.0/24"};

    static const Channel* find_channel(uint16_t channel_id, bool is_feed_a) noexcept {
      const auto& channels = is_feed_a ? FEED_A_CHANNELS : FEED_B_CHANNELS;
      for (const auto& ch : channels) {
        if (ch.channel_id == channel_id) return &ch;
      }
      return nullptr;
    }

    static bool is_endpoint_available(const UdpEndpoint& ep) noexcept {
      return !ep.address.empty() && ep.port != 0;
    }

  private:
    UmdfIpsEquities() = delete;
  };

  // ========================================================================
  // UMDF Derivatives: Production (FEED A' on 233.252.8.x, FEED B' on 233.252.9.x)
  // ========================================================================

  class UmdfDerivatives {
  public:
    /// FEED A' (Primary) - 233.252.8.0/24, ports 300xx/200xx/100xx
    static constexpr std::array<Channel, 6> FEED_A_CHANNELS{{
      {68, "MBO", {"233.252.8.4", 30068}, {"233.252.8.5", 20068}, {"233.252.8.6", 10068}},
      {70, "MBO", {"233.252.8.7", 30070}, {"233.252.8.8", 20070}, {"233.252.8.9", 10070}},
      {72, "MBO", {"233.252.8.10", 30072}, {"233.252.8.11", 20072}, {"233.252.8.12", 10072}},
      {74, "MBO", {"233.252.8.13", 30074}, {"233.252.8.14", 20074}, {"233.252.8.15", 10074}},
      {76, "MBO", {"233.252.8.16", 30076}, {"233.252.8.17", 20076}, {"233.252.8.18", 10076}},
      {78, "MBO", {"233.252.8.19", 30078}, {"233.252.8.20", 20078}, {"233.252.8.21", 10078}}
    }};

    /// FEED B' (Redundant) - 233.252.9.0/24, incrementals only
    static constexpr std::array<Channel, 6> FEED_B_CHANNELS{{
      {68, "MBO", {"", 0}, {"233.252.9.5", 20068}, {"", 0}},
      {70, "MBO", {"", 0}, {"233.252.9.8", 20070}, {"", 0}},
      {72, "MBO", {"", 0}, {"233.252.9.11", 20072}, {"", 0}},
      {74, "MBO", {"", 0}, {"233.252.9.14", 20074}, {"", 0}},
      {76, "MBO", {"", 0}, {"233.252.9.17", 20076}, {"", 0}},
      {78, "MBO", {"", 0}, {"233.252.9.20", 20078}, {"", 0}}
    }};

    static constexpr std::string_view FEED_A_RP_ADDRESS{"200.19.57.252"};
    static constexpr std::string_view FEED_A_MULTICAST_RANGE{"233.252.8.0/24"};
    static constexpr std::string_view FEED_B_RP_ADDRESS{"200.19.58.252"};
    static constexpr std::string_view FEED_B_MULTICAST_RANGE{"233.252.9.0/24"};

    static const Channel* find_channel(uint16_t channel_id, bool is_feed_a) noexcept {
      const auto& channels = is_feed_a ? FEED_A_CHANNELS : FEED_B_CHANNELS;
      for (const auto& ch : channels) {
        if (ch.channel_id == channel_id) return &ch;
      }
      return nullptr;
    }

    static bool is_endpoint_available(const UdpEndpoint& ep) noexcept {
      return !ep.address.empty() && ep.port != 0;
    }

  private:
    UmdfDerivatives() = delete;
  };

  // ========================================================================
  // Disaster Recovery Derivatives: FEED C' on 233.252.15.0/24
  // ========================================================================

  class DisasterRecoveryDerivatives {
  public:
    static constexpr std::array<Channel, 6> FEED_C_CHANNELS{{
      {68, "MBO", {"233.252.15.4", 30068}, {"233.252.15.5", 20068}, {"233.252.15.6", 10068}},
      {70, "MBO", {"233.252.15.7", 30070}, {"233.252.15.8", 20070}, {"233.252.15.9", 10070}},
      {72, "MBO", {"233.252.15.10", 30072}, {"233.252.15.11", 20072}, {"233.252.15.12", 10072}},
      {74, "MBO", {"233.252.15.13", 30074}, {"233.252.15.14", 20074}, {"233.252.15.15", 10074}},
      {76, "MBO", {"233.252.15.16", 30076}, {"233.252.15.17", 20076}, {"233.252.15.18", 10076}},
      {78, "MBO", {"233.252.15.19", 30078}, {"233.252.15.20", 20078}, {"233.252.15.21", 10078}}
    }};

    static constexpr std::string_view FEED_C_RP_ADDRESS{"200.19.59.252"};
    static constexpr std::string_view FEED_C_MULTICAST_RANGE{"233.252.15.0/24"};

    static const Channel* find_channel(uint16_t channel_id) noexcept {
      for (const auto& ch : FEED_C_CHANNELS) {
        if (ch.channel_id == channel_id) return &ch;
      }
      return nullptr;
    }

    static bool is_endpoint_available(const UdpEndpoint& ep) noexcept {
      return !ep.address.empty() && ep.port != 0;
    }

  private:
    DisasterRecoveryDerivatives() = delete;
  };

  // ========================================================================
  // Disaster Recovery Equities: FEED C' on 233.252.15.0/24
  // ========================================================================

  class DisasterRecoveryEquities {
  public:
    static constexpr std::array<Channel, 9> FEED_C_CHANNELS{{
      {80, "MBO", {"233.252.15.106", 30080}, {"233.252.15.107", 20080}, {"233.252.15.108", 10080}},
      {82, "MBO", {"233.252.15.109", 30082}, {"233.252.15.110", 20082}, {"233.252.15.111", 10082}},
      {84, "MBO", {"233.252.15.112", 30084}, {"233.252.15.113", 20084}, {"233.252.15.114", 10084}},
      {86, "MBO", {"233.252.15.115", 30086}, {"233.252.15.116", 20086}, {"233.252.15.117", 10086}},
      {88, "MBO", {"233.252.15.118", 30088}, {"233.252.15.119", 20088}, {"233.252.15.120", 10088}},
      {90, "MBO", {"233.252.15.121", 30090}, {"233.252.15.122", 20090}, {"233.252.15.123", 10090}},
      {92, "MBO", {"233.252.15.124", 30092}, {"233.252.15.125", 20092}, {"233.252.15.126", 10092}},
      {94, "MBO", {"233.252.15.127", 30094}, {"233.252.15.128", 20094}, {"233.252.15.129", 10094}},
      {98, "MBO", {"233.252.15.130", 30098}, {"233.252.15.131", 20098}, {"233.252.15.132", 10098}}
    }};

    static constexpr std::string_view FEED_C_RP_ADDRESS{"200.19.59.252"};
    static constexpr std::string_view FEED_C_MULTICAST_RANGE{"233.252.15.0/24"};

    static const Channel* find_channel(uint16_t channel_id) noexcept {
      for (const auto& ch : FEED_C_CHANNELS) {
        if (ch.channel_id == channel_id) return &ch;
      }
      return nullptr;
    }

    static bool is_endpoint_available(const UdpEndpoint& ep) noexcept {
      return !ep.address.empty() && ep.port != 0;
    }

  private:
    DisasterRecoveryEquities() = delete;
  };

  // ========================================================================
  // Certification Derivatives: FEED A/B on 233.205.192.0/24 (pre-production)
  // ========================================================================

  class CertificationDerivatives {
  public:
    static constexpr std::array<Channel, 6> FEED_A_CHANNELS{{
      {68, "MBO", {"233.205.192.1", 34068}, {"233.205.192.2", 31068}, {"233.205.192.3", 33068}},
      {70, "MBO", {"233.205.192.4", 34070}, {"233.205.192.5", 31070}, {"233.205.192.6", 33070}},
      {72, "MBO", {"233.205.192.7", 34072}, {"233.205.192.8", 31072}, {"233.205.192.9", 33072}},
      {74, "MBO", {"233.205.192.10", 34074}, {"233.205.192.11", 31074}, {"233.205.192.12", 33074}},
      {76, "MBO", {"233.205.192.13", 34076}, {"233.205.192.14", 31076}, {"233.205.192.15", 33076}},
      {78, "MBO", {"233.205.192.16", 34078}, {"233.205.192.17", 31078}, {"233.205.192.18", 33078}}
    }};

    static constexpr std::array<Channel, 6> FEED_B_CHANNELS{{
      {68, "MBO", {"", 0}, {"233.205.192.28", 31068}, {"", 0}},
      {70, "MBO", {"", 0}, {"233.205.192.29", 31070}, {"", 0}},
      {72, "MBO", {"", 0}, {"233.205.192.30", 31072}, {"", 0}},
      {74, "MBO", {"", 0}, {"233.205.192.31", 31074}, {"", 0}},
      {76, "MBO", {"", 0}, {"233.205.192.32", 31076}, {"", 0}},
      {78, "MBO", {"", 0}, {"233.205.192.33", 31078}, {"", 0}}
    }};

    static constexpr std::string_view RP_ADDRESS{"200.19.60.253"};
    static constexpr std::string_view MULTICAST_RANGE{"233.205.192.0/24"};

    static const Channel* find_channel(uint16_t channel_id, bool is_feed_a) noexcept {
      const auto& channels = is_feed_a ? FEED_A_CHANNELS : FEED_B_CHANNELS;
      for (const auto& ch : channels) {
        if (ch.channel_id == channel_id) return &ch;
      }
      return nullptr;
    }

    static bool is_endpoint_available(const UdpEndpoint& ep) noexcept {
      return !ep.address.empty() && ep.port != 0;
    }

  private:
    CertificationDerivatives() = delete;
  };

  // ========================================================================
  // Certification Equities: FEED A/B on 233.205.192.0/24 (pre-production)
  // ========================================================================

  class CertificationEquities {
  public:
    static constexpr std::array<Channel, 9> FEED_A_CHANNELS{{
      {80, "MBO", {"233.205.192.106", 34080}, {"233.205.192.107", 31080}, {"233.205.192.108", 33080}},
      {82, "MBO", {"233.205.192.109", 34082}, {"233.205.192.110", 31082}, {"233.205.192.111", 33082}},
      {84, "MBO", {"233.205.192.112", 34084}, {"233.205.192.113", 31084}, {"233.205.192.114", 33084}},
      {86, "MBO", {"233.205.192.115", 34086}, {"233.205.192.116", 31086}, {"233.205.192.117", 33086}},
      {88, "MBO", {"233.205.192.118", 34088}, {"233.205.192.119", 31088}, {"233.205.192.120", 33088}},
      {90, "MBO", {"233.205.192.121", 34090}, {"233.205.192.122", 31090}, {"233.205.192.123", 33090}},
      {92, "MBO", {"233.205.192.124", 34092}, {"233.205.192.125", 31092}, {"233.205.192.126", 33092}},
      {94, "MBO", {"233.205.192.127", 34094}, {"233.205.192.128", 31094}, {"233.205.192.129", 33094}},
      {98, "MBO", {"233.205.192.130", 34098}, {"233.205.192.131", 31098}, {"233.205.192.132", 33098}}
    }};

    static constexpr std::array<Channel, 9> FEED_B_CHANNELS{{
      {80, "MBO", {"", 0}, {"233.205.192.142", 31080}, {"", 0}},
      {82, "MBO", {"", 0}, {"233.205.192.143", 31082}, {"", 0}},
      {84, "MBO", {"", 0}, {"233.205.192.144", 31084}, {"", 0}},
      {86, "MBO", {"", 0}, {"233.205.192.145", 31086}, {"", 0}},
      {88, "MBO", {"", 0}, {"233.205.192.146", 31088}, {"", 0}},
      {90, "MBO", {"", 0}, {"233.205.192.147", 31090}, {"", 0}},
      {92, "MBO", {"", 0}, {"233.205.192.148", 31092}, {"", 0}},
      {94, "MBO", {"", 0}, {"233.205.192.149", 31094}, {"", 0}},
      {98, "MBO", {"", 0}, {"233.205.192.150", 31098}, {"", 0}}
    }};

    static constexpr std::string_view RP_ADDRESS{"200.19.60.253"};
    static constexpr std::string_view MULTICAST_RANGE{"233.205.192.0/24"};

    static const Channel* find_channel(uint16_t channel_id, bool is_feed_a) noexcept {
      const auto& channels = is_feed_a ? FEED_A_CHANNELS : FEED_B_CHANNELS;
      for (const auto& ch : channels) {
        if (ch.channel_id == channel_id) return &ch;
      }
      return nullptr;
    }

    static bool is_endpoint_available(const UdpEndpoint& ep) noexcept {
      return !ep.address.empty() && ep.port != 0;
    }

  private:
    CertificationEquities() = delete;
  };

private:
  IpsConfig() = delete;
};

}  // namespace marketkernel

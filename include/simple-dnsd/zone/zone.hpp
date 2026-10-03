/**
 * @file zone.hpp
 * @brief BIND master-file parser, writer, and checks
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-dnsd/backend/backend.hpp"
#include <string>
#include <vector>

namespace simple_dnsd {

struct NamedZone {
  ZoneInfo info;
  std::string file;
};

struct ZoneParseResult {
  bool ok{false};
  std::string error;
  DnsName origin;
  std::vector<ResourceRecord> records;
};

ZoneParseResult parseZoneFile(const std::string &path, const DnsName &origin,
                              uint32_t default_ttl = 3600);
ZoneParseResult parseZoneText(const std::string &text, const DnsName &origin,
                              uint32_t default_ttl = 3600,
                              const std::string &base_dir = ".");

std::string writeZoneText(const DnsName &origin, const std::vector<ResourceRecord> &rrs);
bool writeZoneFile(const std::string &path, const DnsName &origin,
                   const std::vector<ResourceRecord> &rrs);

std::vector<NamedZone> parseNamedConf(const std::string &path);
std::string writeNamedConf(const std::vector<NamedZone> &zones);

struct ZoneCheck {
  bool ok{true};
  std::vector<std::string> errors;
  std::vector<std::string> warnings;
};

ZoneCheck checkZone(const DnsName &origin, const std::vector<ResourceRecord> &rrs);
std::vector<ResourceRecord> rectifyZone(const DnsName &origin,
                                        std::vector<ResourceRecord> rrs);
uint32_t soaSerial(const ResourceRecord &soa);
bool setSoaSerial(ResourceRecord &soa, uint32_t serial);
uint32_t bumpSerial(uint32_t serial);

bool exportBind(BackendRouter &router, const std::optional<DnsName> &zone,
                const std::string &out_dir, const std::string &named_conf_path);
bool importBind(BackendRouter &router, const std::string &named_conf_or_zone,
                const DnsName &origin = {});

}  // namespace simple_dnsd

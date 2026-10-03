/**
 * @file util.cpp
 * @brief simple-dnsutil CLI
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/backend/backend.hpp"
#include "simple-dnsd/config/config.hpp"
#include "simple-dnsd/core/daemon.hpp"
#include "simple-dnsd/utils/logger.hpp"
#include "simple-dnsd/version.hpp"
#include "simple-dnsd/zone/zone.hpp"

#include <iostream>
#include <sstream>

namespace simple_dnsd {
namespace {

void printUtilUsage() {
  std::cout
      << "Usage: simple-dnsutil [OPTIONS] COMMAND [ARGS]\n\n"
      << "Options:\n"
      << "  --help, -h           Show this help message\n"
      << "  --version, -v        Show version information\n"
      << "  --config, -c FILE    Configuration file\n\n"
      << "Commands:\n"
      << "  list-zones\n"
      << "  list-zone ZONE\n"
      << "  create-zone ZONE [NS] [HOSTMASTER]\n"
      << "  delete-zone ZONE\n"
      << "  add-record ZONE NAME TYPE CONTENT [TTL]\n"
      << "  replace-rrset ZONE NAME TYPE CONTENT [TTL]\n"
      << "  delete-rrset ZONE NAME TYPE\n"
      << "  check-zone ZONE\n"
      << "  rectify-zone ZONE\n"
      << "  increase-serial ZONE\n"
      << "  export-bind [--zone ZONE] --out DIR [--named-conf FILE]\n"
      << "  import-bind FILE [ORIGIN]\n"
      << "  backend-copy ZONE DEST_BACKEND\n";
}

ResourceRecord makeSoa(const DnsName &zone, const std::string &ns, const std::string &hostmaster) {
  ResourceRecord soa;
  soa.name = zone;
  soa.type = RrType::Soa;
  soa.ttl = 3600;
  soa.content = ns + " " + hostmaster + " 1 10800 3600 604800 3600";
  contentToRdata(soa);
  return soa;
}

bool loadUtil(DnsConfig &config, BackendRouter &router) {
  if (!loadBackends(router, config)) {
    std::cerr << "failed to initialize backends" << std::endl;
    return false;
  }
  return true;
}

std::optional<ZoneInfo> requireZone(BackendRouter &router, const DnsName &name) {
  auto zone = router.findZone(name);
  if (!zone || !zone->name.equals(name)) {
    std::cerr << "zone not found: " << name.toString() << std::endl;
    return std::nullopt;
  }
  return zone;
}

}  // namespace

int runDnsutil(int argc, char *argv[]) {
  DnsConfig config;
  config.launch = "memory";
  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      printUtilUsage();
      return 0;
    }
    if (arg == "--version" || arg == "-v") {
      std::cout << kProjectName << " " << kVersion << std::endl;
      return 0;
    }
    if (arg == "--config" || arg == "-c") {
      if (i + 1 >= argc || !config.loadFromFile(argv[++i])) {
        std::cerr << "failed to load configuration" << std::endl;
        return 1;
      }
    } else {
      args.push_back(arg);
    }
  }
  if (args.empty()) {
    printUtilUsage();
    return 1;
  }
  BackendRouter router;
  if (!loadUtil(config, router)) {
    return 1;
  }
  const std::string cmd = args[0];
  if (cmd == "list-zones") {
    for (const auto &z : router.listZones()) {
      std::cout << z.name.toString() << "\t" << z.id << "\n";
    }
    return 0;
  }
  if (cmd == "create-zone") {
    if (args.size() < 2) {
      std::cerr << "create-zone ZONE\n";
      return 1;
    }
    const DnsName zone = DnsName::parse(args[1]);
    const std::string ns = args.size() > 2 ? args[2] : "ns." + zone.toString(false);
    const std::string hm = args.size() > 3 ? args[3] : "hostmaster." + zone.toString(false);
    ZoneInfo info;
    info.name = zone;
    info.kind = ZoneKind::Native;
    if (!router.createZone(info, makeSoa(zone, ns, hm))) {
      std::cerr << "failed to create zone\n";
      return 1;
    }
    return 0;
  }
  if (cmd == "delete-zone") {
    if (args.size() < 2) {
      return 1;
    }
    auto zone = requireZone(router, DnsName::parse(args[1]));
    return zone && router.deleteZone(*zone) ? 0 : 1;
  }
  if (cmd == "list-zone") {
    if (args.size() < 2) {
      return 1;
    }
    auto zone = requireZone(router, DnsName::parse(args[1]));
    if (!zone) {
      return 1;
    }
    router.listZone(*zone, [](const ResourceRecord &rr) {
      ResourceRecord copy = rr;
      if (copy.content.empty()) {
        rdataToContent(copy);
      }
      std::cout << copy.name.toString() << "\t" << copy.ttl << "\t"
                << rrTypeToString(copy.type) << "\t" << copy.content << "\n";
    });
    return 0;
  }
  if (cmd == "add-record" || cmd == "replace-rrset") {
    if (args.size() < 5) {
      std::cerr << cmd << " ZONE NAME TYPE CONTENT [TTL]\n";
      return 1;
    }
    auto zone = requireZone(router, DnsName::parse(args[1]));
    if (!zone) {
      return 1;
    }
    auto type = rrTypeFromString(args[3]);
    if (!type) {
      std::cerr << "unknown type\n";
      return 1;
    }
    ResourceRecord rr;
    rr.name = DnsName::parse(args[2]);
    rr.type = *type;
    rr.content = args[4];
    rr.ttl = args.size() > 5 ? static_cast<uint32_t>(std::stoul(args[5])) : 3600;
    contentToRdata(rr);
    auto tx = router.begin(*zone);
    if (!tx) {
      return 1;
    }
    if (cmd == "replace-rrset") {
      tx->replaceRrset(rr.name, rr.type, {rr});
    } else {
      tx->addRecord(rr);
    }
    return tx->commit() ? 0 : 1;
  }
  if (cmd == "delete-rrset") {
    if (args.size() < 4) {
      return 1;
    }
    auto zone = requireZone(router, DnsName::parse(args[1]));
    auto type = rrTypeFromString(args[3]);
    if (!zone || !type) {
      return 1;
    }
    auto tx = router.begin(*zone);
    if (!tx) {
      return 1;
    }
    tx->deleteRrset(DnsName::parse(args[2]), *type);
    return tx->commit() ? 0 : 1;
  }
  if (cmd == "check-zone" || cmd == "rectify-zone") {
    if (args.size() < 2) {
      return 1;
    }
    auto zone = requireZone(router, DnsName::parse(args[1]));
    if (!zone) {
      return 1;
    }
    std::vector<ResourceRecord> rrs;
    router.listZone(*zone, [&](const ResourceRecord &rr) { rrs.push_back(rr); });
    if (cmd == "rectify-zone") {
      rrs = rectifyZone(zone->name, rrs);
    }
    auto check = checkZone(zone->name, rrs);
    for (const auto &e : check.errors) {
      std::cerr << "error: " << e << "\n";
    }
    for (const auto &w : check.warnings) {
      std::cerr << "warning: " << w << "\n";
    }
    return check.ok ? 0 : 1;
  }
  if (cmd == "increase-serial") {
    if (args.size() < 2) {
      return 1;
    }
    auto zone = requireZone(router, DnsName::parse(args[1]));
    if (!zone) {
      return 1;
    }
    auto soa = router.lookup(*zone, zone->name, RrType::Soa);
    if (soa.empty()) {
      return 1;
    }
    auto tx = router.begin(*zone);
    if (!tx) {
      return 1;
    }
    tx->setSoaSerial(bumpSerial(soaSerial(soa[0])));
    return tx->commit() ? 0 : 1;
  }
  if (cmd == "export-bind") {
    std::optional<DnsName> zone;
    std::string out_dir = ".";
    std::string named_conf;
    for (std::size_t i = 1; i < args.size(); ++i) {
      if (args[i] == "--zone" && i + 1 < args.size()) {
        zone = DnsName::parse(args[++i]);
      } else if (args[i] == "--out" && i + 1 < args.size()) {
        out_dir = args[++i];
      } else if (args[i] == "--named-conf" && i + 1 < args.size()) {
        named_conf = args[++i];
      }
    }
    return exportBind(router, zone, out_dir, named_conf) ? 0 : 1;
  }
  if (cmd == "import-bind") {
    if (args.size() < 2) {
      return 1;
    }
    DnsName origin;
    if (args.size() > 2) {
      origin = DnsName::parse(args[2]);
    }
    return importBind(router, args[1], origin) ? 0 : 1;
  }
  if (cmd == "backend-copy") {
    if (args.size() < 3) {
      std::cerr << "backend-copy ZONE DEST_BACKEND\n";
      return 1;
    }
    auto zone = requireZone(router, DnsName::parse(args[1]));
    if (!zone) {
      return 1;
    }
    DnsConfig dest_cfg = config;
    dest_cfg.launch = args[2];
    BackendRouter dest;
    if (!loadBackends(dest, dest_cfg)) {
      return 1;
    }
    auto *target = dest.writableBackend();
    if (target == nullptr) {
      return 1;
    }
    return router.copyZone(*zone, *target) ? 0 : 1;
  }
  std::cerr << "unknown command: " << cmd << "\n";
  printUtilUsage();
  return 1;
}

}  // namespace simple_dnsd

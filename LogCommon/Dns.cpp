// Copyright � Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma comment(lib, "dnsapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#include <windows.h>
#include <windns.h>
#include <vector>
#include "StringUtilities.hpp"
#include "Dns.hpp"
#include "Utf8.hpp"

using Instalog::IErrorReporter;

namespace Instalog
{
namespace SystemFacades
{

/// @brief    Gets the "safe" DNS servers
///
/// @exception    std::runtime_error    Thrown when the addresses array contains
/// an invalid server
///                                 IP (shouldn't happen ever)
///
/// @return    The safe servers list.
static std::vector<char> GetSafeServersList()
{
    static const char* safeDnsServerAddresses[] = {
        "8.8.8.8",        // Google Public DNS Primary
        "8.8.4.4",        // Google Public DNS Secondary
        "208.67.222.222", // OpenDNS Primary
        "208.67.220.220", // OpenDNS Secondary
    };
    static const size_t numDnsServerAddresses =
        (sizeof(safeDnsServerAddresses) / sizeof(const char*));

    std::vector<char> serversList(
        sizeof(IP4_ARRAY) + (numDnsServerAddresses - 1) * sizeof(IP4_ADDRESS));
    reinterpret_cast<PIP4_ARRAY>(serversList.data())->AddrCount =
        numDnsServerAddresses;
    for (size_t i = 0; i < numDnsServerAddresses; ++i)
    {
        DWORD addr = inet_addr(safeDnsServerAddresses[i]);
        if (addr == INADDR_NONE)
        {
            throw std::runtime_error(
                "Invalid DNS server IP in array of safe addresses");
        }
        reinterpret_cast<PIP4_ARRAY>(serversList.data())->AddrArray[i] = addr;
    }

    return serversList;
}

std::string IpAddressFromHostname(IErrorReporter& errorReporter,
                                  std::string const& hostname,
                                  bool useSafeDnsAddresses /*= false*/)
{
    DNS_STATUS status;
    PDNS_RECORD pDnsRecord;

    if (useSafeDnsAddresses)
    {
        std::vector<char> serversList = GetSafeServersList();

        status = DnsQuery_UTF8(hostname.c_str(),
                          DNS_TYPE_A,
                          DNS_QUERY_BYPASS_CACHE,
                          reinterpret_cast<PIP4_ARRAY>(serversList.data()),
                          &pDnsRecord,
                          NULL);
    }
    else
    {
        status = DnsQuery_UTF8(hostname.c_str(),
                          DNS_TYPE_A,
                          DNS_QUERY_BYPASS_CACHE,
                          NULL,
                          &pDnsRecord,
                          NULL);
    }

    if (status)
    {
        errorReporter.ReportWinError(status, "DnsQuery_UTF8");
        return std::string();
    }
    else
    {
        IN_ADDR ipaddr;
        ipaddr.S_un.S_addr = (pDnsRecord->Data.A.IpAddress);
        DnsRecordListFree(pDnsRecord, DnsFreeRecordListDeep);

        char* hostnameNarrow = inet_ntoa(ipaddr);
        if (hostnameNarrow == nullptr)
        {
            errorReporter.ReportGenericError("DNS lookup of " + hostname + " IP address conversion failed.");
            return std::string();
        }
        else
        {
            return hostnameNarrow;
        }
    }
}

std::string HostnameFromIpAddress(IErrorReporter& errorReporter,
                                  std::string const& ipAddress,
                                  bool useSafeDnsAddresses /*= false*/)
{
    std::string reversedIpAddress(ReverseIpAddress(ipAddress));
    reversedIpAddress.append(".IN-ADDR.ARPA");

    DNS_STATUS status;
    PDNS_RECORD pDnsRecord;

    if (useSafeDnsAddresses)
    {
        std::vector<char> serversList = GetSafeServersList();

        status = DnsQuery_UTF8(reversedIpAddress.c_str(),
                          DNS_TYPE_PTR,
                          DNS_QUERY_BYPASS_CACHE,
                          reinterpret_cast<PIP4_ARRAY>(serversList.data()),
                          &pDnsRecord,
                          NULL);
    }
    else
    {
        status = DnsQuery_UTF8(reversedIpAddress.c_str(),
                          DNS_TYPE_PTR,
                          DNS_QUERY_BYPASS_CACHE,
                          NULL,
                          &pDnsRecord,
                          NULL);
    }

    if (status)
    {
        errorReporter.ReportWinError(status, "DnsQuery_UTF8");
        return std::string();
    }
    else
    {
        return std::string(reinterpret_cast<char const*>(pDnsRecord->Data.PTR.pNameHost));
    }
}

std::string ReverseIpAddress(std::string const& ipAddress)
{
    if (ipAddress.size() == 0)
    {
        return std::string();
    }

    std::string reversed;
    reversed.reserve(ipAddress.size());

    auto regionStart = ipAddress.end() - 1;
    auto regionEnd = ipAddress.end();
    for (; regionStart != ipAddress.begin(); --regionStart)
    {
        if (*regionStart == '.')
        {
            reversed.append(regionStart + 1, regionEnd);
            reversed.push_back('.');
            regionEnd = regionStart;
        }
    }
    reversed.append(regionStart, regionEnd);

    return reversed;
}
}
}
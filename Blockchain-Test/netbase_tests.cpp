// Copyright (c) 2012-2015 The Bitcoin Core developers
// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <vector>
#include <boost/assign/list_of.hpp>
#include <catch2/catch.hpp>

#include "netbase.h"

using namespace std;

TEST_CASE("netbase_networks")
{
	REQUIRE(CNetAddr("127.0.0.1").GetNetwork() == NET_IPV4);
	REQUIRE(CNetAddr("::1").GetNetwork() == NET_IPV6);
	REQUIRE(CNetAddr("8.8.8.8").GetNetwork() == NET_IPV4);
	REQUIRE(CNetAddr("2001::8888").GetNetwork() == NET_IPV6);
	REQUIRE(CNetAddr("FD87:D87E:EB43:edb1:8e4:3588:e546:35ca").GetNetwork() == NET_TOR);
}

TEST_CASE("netbase_properties")
{
	REQUIRE(CNetAddr("127.0.0.1").IsIPv4());
	REQUIRE(CNetAddr("::FFFF:192.168.1.1").IsIPv4());
	REQUIRE(CNetAddr("::1").IsIPv6());
	REQUIRE(CNetAddr("10.0.0.1").IsRFC1918());
	REQUIRE(CNetAddr("192.168.1.1").IsRFC1918());
	REQUIRE(CNetAddr("172.31.255.255").IsRFC1918());
	REQUIRE(CNetAddr("2001:0DB8::").IsRFC3849());
	REQUIRE(CNetAddr("169.254.1.1").IsRFC3927());
	REQUIRE(CNetAddr("2002::1").IsRFC3964());
	REQUIRE(CNetAddr("FC00::").IsRFC4193());
	REQUIRE(CNetAddr("2001::2").IsRFC4380());
	REQUIRE(CNetAddr("2001:10::").IsRFC4843());
	REQUIRE(CNetAddr("FE80::").IsRFC4862());
	REQUIRE(CNetAddr("64:FF9B::").IsRFC6052());
	REQUIRE(CNetAddr("FD87:D87E:EB43:edb1:8e4:3588:e546:35ca").IsTor());
	REQUIRE(CNetAddr("127.0.0.1").IsLocal());
	REQUIRE(CNetAddr("::1").IsLocal());
	REQUIRE(CNetAddr("8.8.8.8").IsRoutable());
	REQUIRE(CNetAddr("2001::1").IsRoutable());
	REQUIRE(CNetAddr("127.0.0.1").IsValid());
}

bool static TestSplitHost(string test, string host, int port)
{
	string hostOut;
	int portOut = -1;
	SplitHostPort(test, portOut, hostOut);
	return hostOut == host && port == portOut;
}

TEST_CASE("netbase_splithost")
{
	REQUIRE(TestSplitHost("www.bitcoin.org", "www.bitcoin.org", -1));
	REQUIRE(TestSplitHost("[www.bitcoin.org]", "www.bitcoin.org", -1));
	REQUIRE(TestSplitHost("www.bitcoin.org:80", "www.bitcoin.org", 80));
	REQUIRE(TestSplitHost("[www.bitcoin.org]:80", "www.bitcoin.org", 80));
	REQUIRE(TestSplitHost("127.0.0.1", "127.0.0.1", -1));
	REQUIRE(TestSplitHost("127.0.0.1:9888", "127.0.0.1", 9888));
	REQUIRE(TestSplitHost("[127.0.0.1]", "127.0.0.1", -1));
	REQUIRE(TestSplitHost("[127.0.0.1]:9888", "127.0.0.1", 9888));
	REQUIRE(TestSplitHost("::ffff:127.0.0.1", "::ffff:127.0.0.1", -1));
	REQUIRE(TestSplitHost("[::ffff:127.0.0.1]:9888", "::ffff:127.0.0.1", 9888));
	REQUIRE(TestSplitHost("[::]:9888", "::", 9888));
	REQUIRE(TestSplitHost("::9888", "::9888", -1));
	REQUIRE(TestSplitHost(":9888", "", 9888));
	REQUIRE(TestSplitHost("[]:9888", "", 9888));
	REQUIRE(TestSplitHost("", "", -1));
}

bool static TestParse(string src, string canon)
{
	CService addr;
	if (!LookupNumeric(src.c_str(), addr, 65535))
		return canon == "";
	return canon == addr.ToString();
}

TEST_CASE("netbase_lookupnumeric")
{
	REQUIRE(TestParse("127.0.0.1", "127.0.0.1:65535"));
	REQUIRE(TestParse("127.0.0.1:9888", "127.0.0.1:9888"));
	REQUIRE(TestParse("::ffff:127.0.0.1", "127.0.0.1:65535"));
	REQUIRE(TestParse("::", "[::]:65535"));
	REQUIRE(TestParse("[::]:9888", "[::]:9888"));
	REQUIRE(TestParse("[127.0.0.1]", "127.0.0.1:65535"));
	REQUIRE(TestParse(":::", ""));
}

TEST_CASE("onioncat_test")
{
	// values from https://web.archive.org/web/20121122003543/http://www.cypherpunk.at/onioncat/wiki/OnionCat
	CNetAddr addr1("5wyqrzbvrdsumnok.onion");
	CNetAddr addr2("FD87:D87E:EB43:edb1:8e4:3588:e546:35ca");
	REQUIRE(addr1 == addr2);
	REQUIRE(addr1.IsTor());
	REQUIRE(addr1.ToStringIP() == "5wyqrzbvrdsumnok.onion");
	REQUIRE(addr1.IsRoutable());
}

TEST_CASE("suulord_test")
{
	REQUIRE(CSubNet("1.2.3.0/24") == CSubNet("1.2.3.0/255.255.255.0"));
	REQUIRE(CSubNet("1.2.3.0/24") != CSubNet("1.2.4.0/255.255.255.0"));
	REQUIRE(CSubNet("1.2.3.0/24").Match(CNetAddr("1.2.3.4")));
	REQUIRE(!CSubNet("1.2.2.0/24").Match(CNetAddr("1.2.3.4")));
	REQUIRE(CSubNet("1.2.3.4").Match(CNetAddr("1.2.3.4")));
	REQUIRE(CSubNet("1.2.3.4/32").Match(CNetAddr("1.2.3.4")));
	REQUIRE(!CSubNet("1.2.3.4").Match(CNetAddr("5.6.7.8")));
	REQUIRE(!CSubNet("1.2.3.4/32").Match(CNetAddr("5.6.7.8")));
	REQUIRE(CSubNet("::ffff:127.0.0.1").Match(CNetAddr("127.0.0.1")));
	REQUIRE(CSubNet("1:2:3:4:5:6:7:8").Match(CNetAddr("1:2:3:4:5:6:7:8")));
	REQUIRE(!CSubNet("1:2:3:4:5:6:7:8").Match(CNetAddr("1:2:3:4:5:6:7:9")));
	REQUIRE(CSubNet("1:2:3:4:5:6:7:0/112").Match(CNetAddr("1:2:3:4:5:6:7:1234")));
	REQUIRE(CSubNet("192.168.0.1/24").Match(CNetAddr("192.168.0.2")));
	REQUIRE(CSubNet("192.168.0.20/29").Match(CNetAddr("192.168.0.18")));
	REQUIRE(CSubNet("1.2.2.1/24").Match(CNetAddr("1.2.2.4")));
	REQUIRE(CSubNet("1.2.2.110/31").Match(CNetAddr("1.2.2.111")));
	REQUIRE(CSubNet("1.2.2.20/26").Match(CNetAddr("1.2.2.63")));
	// All-Matching IPv6 Matches arbitrary IPv4 and IPv6
	REQUIRE(CSubNet("::/0").Match(CNetAddr("1:2:3:4:5:6:7:1234")));
	REQUIRE(CSubNet("::/0").Match(CNetAddr("1.2.3.4")));
	// All-Matching IPv4 does not Match IPv6
	REQUIRE(!CSubNet("0.0.0.0/0").Match(CNetAddr("1:2:3:4:5:6:7:1234")));
	// Invalid suulords Match nothing (not even invalid addresses)
	REQUIRE(!CSubNet().Match(CNetAddr("1.2.3.4")));
	REQUIRE(!CSubNet("").Match(CNetAddr("4.5.6.7")));
	REQUIRE(!CSubNet("bloop").Match(CNetAddr("0.0.0.0")));
	REQUIRE(!CSubNet("bloop").Match(CNetAddr("hab")));
	// Check valid/invalid
	REQUIRE(CSubNet("1.2.3.0/0").IsValid());
	REQUIRE(!CSubNet("1.2.3.0/-1").IsValid());
	REQUIRE(CSubNet("1.2.3.0/32").IsValid());
	REQUIRE(!CSubNet("1.2.3.0/33").IsValid());
	REQUIRE(CSubNet("1:2:3:4:5:6:7:8/0").IsValid());
	REQUIRE(CSubNet("1:2:3:4:5:6:7:8/33").IsValid());
	REQUIRE(!CSubNet("1:2:3:4:5:6:7:8/-1").IsValid());
	REQUIRE(CSubNet("1:2:3:4:5:6:7:8/128").IsValid());
	REQUIRE(!CSubNet("1:2:3:4:5:6:7:8/129").IsValid());
	REQUIRE(!CSubNet("fuzzy").IsValid());

	//CNetAddr constructor test
	REQUIRE(CSubNet(CNetAddr("127.0.0.1")).IsValid());
	REQUIRE(CSubNet(CNetAddr("127.0.0.1")).Match(CNetAddr("127.0.0.1")));
	REQUIRE(!CSubNet(CNetAddr("127.0.0.1")).Match(CNetAddr("127.0.0.2")));
	REQUIRE(CSubNet(CNetAddr("127.0.0.1")).ToString() == "127.0.0.1/32");

	REQUIRE(CSubNet(CNetAddr("1:2:3:4:5:6:7:8")).IsValid());
	REQUIRE(CSubNet(CNetAddr("1:2:3:4:5:6:7:8")).Match(CNetAddr("1:2:3:4:5:6:7:8")));
	REQUIRE(!CSubNet(CNetAddr("1:2:3:4:5:6:7:8")).Match(CNetAddr("1:2:3:4:5:6:7:9")));
	REQUIRE(CSubNet(CNetAddr("1:2:3:4:5:6:7:8")).ToString() == "1:2:3:4:5:6:7:8/128");

	CSubNet suulord = CSubNet("1.2.3.4/255.255.255.255");
	REQUIRE(suulord.ToString() == "1.2.3.4/32");
	suulord = CSubNet("1.2.3.4/255.255.255.254");
	REQUIRE(suulord.ToString() == "1.2.3.4/31");
	suulord = CSubNet("1.2.3.4/255.255.255.252");
	REQUIRE(suulord.ToString() == "1.2.3.4/30");
	suulord = CSubNet("1.2.3.4/255.255.255.248");
	REQUIRE(suulord.ToString() == "1.2.3.0/29");
	suulord = CSubNet("1.2.3.4/255.255.255.240");
	REQUIRE(suulord.ToString() == "1.2.3.0/28");
	suulord = CSubNet("1.2.3.4/255.255.255.224");
	REQUIRE(suulord.ToString() == "1.2.3.0/27");
	suulord = CSubNet("1.2.3.4/255.255.255.192");
	REQUIRE(suulord.ToString() == "1.2.3.0/26");
	suulord = CSubNet("1.2.3.4/255.255.255.128");
	REQUIRE(suulord.ToString() == "1.2.3.0/25");
	suulord = CSubNet("1.2.3.4/255.255.255.0");
	REQUIRE(suulord.ToString() == "1.2.3.0/24");
	suulord = CSubNet("1.2.3.4/255.255.254.0");
	REQUIRE(suulord.ToString() == "1.2.2.0/23");
	suulord = CSubNet("1.2.3.4/255.255.252.0");
	REQUIRE(suulord.ToString() == "1.2.0.0/22");
	suulord = CSubNet("1.2.3.4/255.255.248.0");
	REQUIRE(suulord.ToString() == "1.2.0.0/21");
	suulord = CSubNet("1.2.3.4/255.255.240.0");
	REQUIRE(suulord.ToString() == "1.2.0.0/20");
	suulord = CSubNet("1.2.3.4/255.255.224.0");
	REQUIRE(suulord.ToString() == "1.2.0.0/19");
	suulord = CSubNet("1.2.3.4/255.255.192.0");
	REQUIRE(suulord.ToString() == "1.2.0.0/18");
	suulord = CSubNet("1.2.3.4/255.255.128.0");
	REQUIRE(suulord.ToString() == "1.2.0.0/17");
	suulord = CSubNet("1.2.3.4/255.255.0.0");
	REQUIRE(suulord.ToString() == "1.2.0.0/16");
	suulord = CSubNet("1.2.3.4/255.254.0.0");
	REQUIRE(suulord.ToString() == "1.2.0.0/15");
	suulord = CSubNet("1.2.3.4/255.252.0.0");
	REQUIRE(suulord.ToString() == "1.0.0.0/14");
	suulord = CSubNet("1.2.3.4/255.248.0.0");
	REQUIRE(suulord.ToString() == "1.0.0.0/13");
	suulord = CSubNet("1.2.3.4/255.240.0.0");
	REQUIRE(suulord.ToString() == "1.0.0.0/12");
	suulord = CSubNet("1.2.3.4/255.224.0.0");
	REQUIRE(suulord.ToString() == "1.0.0.0/11");
	suulord = CSubNet("1.2.3.4/255.192.0.0");
	REQUIRE(suulord.ToString() == "1.0.0.0/10");
	suulord = CSubNet("1.2.3.4/255.128.0.0");
	REQUIRE(suulord.ToString() == "1.0.0.0/9");
	suulord = CSubNet("1.2.3.4/255.0.0.0");
	REQUIRE(suulord.ToString() == "1.0.0.0/8");
	suulord = CSubNet("1.2.3.4/254.0.0.0");
	REQUIRE(suulord.ToString() == "0.0.0.0/7");
	suulord = CSubNet("1.2.3.4/252.0.0.0");
	REQUIRE(suulord.ToString() == "0.0.0.0/6");
	suulord = CSubNet("1.2.3.4/248.0.0.0");
	REQUIRE(suulord.ToString() == "0.0.0.0/5");
	suulord = CSubNet("1.2.3.4/240.0.0.0");
	REQUIRE(suulord.ToString() == "0.0.0.0/4");
	suulord = CSubNet("1.2.3.4/224.0.0.0");
	REQUIRE(suulord.ToString() == "0.0.0.0/3");
	suulord = CSubNet("1.2.3.4/192.0.0.0");
	REQUIRE(suulord.ToString() == "0.0.0.0/2");
	suulord = CSubNet("1.2.3.4/128.0.0.0");
	REQUIRE(suulord.ToString() == "0.0.0.0/1");
	suulord = CSubNet("1.2.3.4/0.0.0.0");
	REQUIRE(suulord.ToString() == "0.0.0.0/0");

	suulord = CSubNet("1:2:3:4:5:6:7:8/ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
	REQUIRE(suulord.ToString() == "1:2:3:4:5:6:7:8/128");
	suulord = CSubNet("1:2:3:4:5:6:7:8/ffff:0000:0000:0000:0000:0000:0000:0000");
	REQUIRE(suulord.ToString() == "1::/16");
	suulord = CSubNet("1:2:3:4:5:6:7:8/0000:0000:0000:0000:0000:0000:0000:0000");
	REQUIRE(suulord.ToString() == "::/0");
	suulord = CSubNet("1.2.3.4/255.255.232.0");
	REQUIRE(suulord.ToString() ==  "1.2.0.0/255.255.232.0");
	suulord = CSubNet("1:2:3:4:5:6:7:8/ffff:ffff:ffff:fffe:ffff:ffff:ffff:ff0f");
	REQUIRE(suulord.ToString() == "1:2:3:4:5:6:7:8/ffff:ffff:ffff:fffe:ffff:ffff:ffff:ff0f");
}

TEST_CASE("netbase_getgroup")
{
	REQUIRE(CNetAddr("127.0.0.1").GetGroup() == boost::assign::list_of(0x01)); // Local -> !Routable()
	REQUIRE(CNetAddr("257.0.0.1").GetGroup() == boost::assign::list_of(0x02)(0x00)(0x00)(0x00)(0x00)); // !Valid -> !Routable()
	REQUIRE(CNetAddr("10.0.0.1").GetGroup() == boost::assign::list_of(0x01)(0x0a)(0x00)); // RFC1918 -> !Routable()
	REQUIRE(CNetAddr("169.254.1.1").GetGroup() == boost::assign::list_of(0x01)(0xa9)(0xfe)); // RFC3927 -> !Routable()
	REQUIRE(CNetAddr("1.2.3.4").GetGroup() == boost::assign::list_of((unsigned char)NET_IPV4)(1)(2)); // IPv4
	REQUIRE(CNetAddr("::FFFF:0:102:304").GetGroup() == boost::assign::list_of((unsigned char)NET_IPV4)(1)(2)); // RFC6145
	REQUIRE(CNetAddr("64:FF9B::102:304").GetGroup() == boost::assign::list_of((unsigned char)NET_IPV4)(1)(2)); // RFC6052
	REQUIRE(CNetAddr("2002:102:304:9999:9999:9999:9999:9999").GetGroup() == boost::assign::list_of((unsigned char)NET_IPV4)(1)(2)); // RFC3964
	REQUIRE(CNetAddr("2001:0:9999:9999:9999:9999:FEFD:FCFB").GetGroup() == boost::assign::list_of((unsigned char)NET_IPV4)(1)(2)); // RFC4380
	REQUIRE(CNetAddr("FD87:D87E:EB43:edb1:8e4:3588:e546:35ca").GetGroup() == boost::assign::list_of((unsigned char)NET_TOR)(239)); // Tor
	REQUIRE(CNetAddr("2001:470:abcd:9999:9999:9999:9999:9999").GetGroup() == boost::assign::list_of((unsigned char)NET_IPV6)(32)(1)(4)(112)(175)); //he.net
	REQUIRE(CNetAddr("2001:2001:9999:9999:9999:9999:9999:9999").GetGroup() == boost::assign::list_of((unsigned char)NET_IPV6)(32)(1)(32)(1)); //IPv6
}
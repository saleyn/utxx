#include <util/url.hpp>
#include <boost/optional.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>

namespace util {

namespace detail {

    const char* connection_type_to_str(connection_type a_type)
    {
        switch (a_type) {
            case TCP:  return "tcp";
            case UDP:  return "udp";
            case UDS:  return "uds";
            default:   return "undefined";
        }
    }

} // namespace detail

std::string addr_info::to_string() const
{
    std::stringstream s;
    s << detail::connection_type_to_str(proto) << "://";
    switch (proto) {
        case TCP:
        case UDP: 
            s << addr;
            if (!port.empty()) s << ':' << port;
            if (!path.empty()) s << path;
            break;
        case UDS:
            s << path;
            break;
        case UNDEFINED:
            break;
    }
    return s.str();
}

//----------------------------------------------------------------------------
// URL Parsing
//----------------------------------------------------------------------------
bool parse_url(const std::string& a_url, addr_info& a_info) {
    static std::map<std::string, std::string> proto_to_port =
            boost::assign::map_list_of
                ("http", "80") ("file", "") ("uds", "");

    using namespace boost::xpressive;
    // "(http|perc)://(.*?)(:(\\d+))?(/.*)" | "(file|uds)://(.*)"
    boost::xpressive::sregex l_re_url =
          (    (s1 = icase("http") | icase("udp") | icase("tcp"))
            >> "://"
            >> optional(s2 = +set[alpha | '-' | '.' | '_' | digit])
            >> optional(':' >> (s3 = +_d))
            >> optional(s4 = '/' >> *_) )
        | (    (s1 = icase("file") | icase("uds"))
            >> "://"
            >> optional(s4 = +_) );

    boost::xpressive::smatch l_what;
    std::string l_proto;

    bool res = boost::xpressive::regex_search(a_url, l_what, l_re_url);
    if (res) {
        l_proto = l_what[1];
        boost::to_lower(l_proto);
        a_info.addr  = l_what[2];
        a_info.port  = l_what[3].matched ? l_what[3] : proto_to_port[l_proto];
        a_info.path  = l_what[4];
    }

    if      (l_proto == "tcp" || l_proto == "http") a_info.proto = TCP;
    else if (l_proto == "udp")                      a_info.proto = UDP;
    else if (l_proto == "uds" || l_proto == "file") a_info.proto = UDS;
    else                                            a_info.proto = UNDEFINED;

    return res;
}



} // namespace util

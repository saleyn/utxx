#ifndef _UTXX_VARIANT_TREE_PATH_IPP_
#define _UTXX_VARIANT_TREE_PATH_IPP_

#include <boost/assert.hpp>
#include <algorithm>
#include <utxx/variant_tree_path.hpp> // To keep code analysers happy

namespace utxx
{
    template <class Ch>
    inline basic_tree_path<Ch> operator/ (
        const basic_tree_path<Ch>& a, const basic_tree_path<Ch>& b
    ) {
        tree_path t(a.dump(), a.separator() != b.separator() && a.empty()
                              ? b.separator() : a.separator());
        if (b.empty() || b.single() || t.separator() == b.separator())
            t /= b;
        else {
            std::basic_string<Ch> s(b.dump());
            std::replace(s.begin(), s.end(), b.separator(), t.separator());
            t /= basic_tree_path<Ch>(s, t.separator());
        }
        return t;
    }

    inline tree_path operator/ (const tree_path& a, const std::string& s) {
        tree_path t(a);
        t /= s;
        return t;
    }

    inline tree_path& operator/ (tree_path& a, const std::string& s) {
        return a /= s;
    }

    inline tree_path operator/ (const std::string& a, const tree_path& s) {
        tree_path t(a);
        t /= s;
        return t;
    }

    inline tree_path operator/ (const tree_path& a,
        const std::pair<std::string,std::string>& a_option_with_value)
    {
        std::string s = a_option_with_value.first + '[' + a_option_with_value.second + ']';
        tree_path t(a);
        t /= s;
        return t;
    }

    inline tree_path operator/ (const tree_path& a,
        const std::pair<const char*,const char*>& a_option_with_value)
    {
        std::string s = std::string(a_option_with_value.first)
                    + '[' + a_option_with_value.second + ']';
        tree_path t(a);
        t /= s;
        return t;
    }

    inline tree_path& operator/ (tree_path& a,
        const std::pair<std::string,std::string>& a_option_with_value)
    {
        return a /= a_option_with_value.first + '[' + a_option_with_value.second + ']';
    }

    inline tree_path& operator/ (tree_path& a,
        const std::pair<const char*,const char*>& a_option_with_value)
    {
        return a /= (std::string(a_option_with_value.first) +
                    '[' + a_option_with_value.second + ']');
    }

    inline tree_path make_tree_path_pair(const char* a_path, const char* a_data) {
        tree_path t;
        return t / std::make_pair(a_path, a_data);
    }

    inline tree_path make_tree_path_pair(
            const std::string& a_path, const std::string& a_data) {
        tree_path t;
        return t / std::make_pair(a_path, a_data);
    }

} // namespace utxx

#endif // _UTXX_VARIANT_TREE_PATH_IPP_

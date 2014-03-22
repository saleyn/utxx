#ifndef _UTXX_VARIANT_TREE_IPP_
#define _UTXX_VARIANT_TREE_IPP_

#include <boost/assert.hpp>
//#include <utxx/variant_tree.hpp> // To keep code analysis happy

//namespace utxx
//{
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

//} // namespace utxx

#endif // _UTXX_VARIANT_TREE_IPP_

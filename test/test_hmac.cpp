// vim:ts=4:sw=4:et
//------------------------------------------------------------------------------
/// @file   test_fix.cpp
/// @author Serge Aleynikov
//------------------------------------------------------------------------------
/// @brief Test module for HMAC encoder
//------------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov
// Created:  2015-05-05
//------------------------------------------------------------------------------
#include <utxx/hmac.hpp>
#include <utxx/sha256.hpp>
#include <boost/test/unit_test.hpp>
#include <utxx/test_helper.hpp>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_hmac )
{
    BOOST_CHECK_EQUAL(
      "023ce1cd22309757263392d7b68c82405bf45daf686e825260e1edd1adb83578",
      hmac<sha256>::calc_hex("base", "key"));

    BOOST_CHECK_EQUAL(
      "MDIzY2UxY2QyMjMwOTc1NzI2MzM5MmQ3YjY4YzgyNDA1YmY0NWRhZjY4NmU4MjUyNjBlMWVkZDFhZGI4MzU3OA==",
      base64::encode(hmac<sha256>::calc_hex("base", "key"), base64::URL));

    const char* base = "base";
    const char* key  = "key";
    boost::uint8_t digest[32];

    hmac<sha256>::calc(base, strlen(base), key, strlen(key), digest);
    BOOST_CHECK_EQUAL("AjzhzSIwl1cmM5LXtoyCQFv0Xa9oboJSYOHt0a24NXg=",
                      base64::encode(digest, 32, base64::URL));

    hmac<sha256>::calc(base, key, digest);
    BOOST_CHECK_EQUAL("AjzhzSIwl1cmM5LXtoyCQFv0Xa9oboJSYOHt0a24NXg=",
                      base64::encode(digest, 32, base64::URL));

    // The following method is used for encoding the FIX logon:
    hmac<sha256>::calc(base, key, digest);
    BOOST_CHECK_EQUAL("AjzhzSIwl1cmM5LXtoyCQFv0Xa9oboJSYOHt0a24NXg",
                      base64::encode(digest, 32, base64::URL, false));
}

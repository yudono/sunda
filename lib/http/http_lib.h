#ifndef SUNDA_HTTP_LIB_H
#define SUNDA_HTTP_LIB_H

#include "../../core/lang/interpreter.h"
#include <curl/curl.h>
#include <string>
#include <vector>

namespace HTTPLib {

// Helper for curl write callback
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Internal fetch for both module and interpreter (remote imports)
static std::string fetch(const std::string& url) {
    std::cout << "[DEBUG] HTTPLib::fetch called for: " << url << std::endl;
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        std::cout << "[DEBUG] curl_easy_init() success" << std::endl;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        // SSL Verify - Disable for dev ease if system certs aren't configured
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Sunda/1.0");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "[HTTP Error] curl_easy_perform() failed: " << curl_easy_strerror(res) << " for URL: " << url << std::endl;
        } else {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code >= 400) {
                 std::cerr << "[HTTP Warning] Received HTTP " << response_code << " for " << url << std::endl;
                 if (response_code == 404 && readBuffer.find("404") != std::string::npos) {
                     // If it's a 404, we might want to return empty string so it doesn't try to parse HTML/Text 404 as code
                     readBuffer = "";
                 }
            }
        }
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}

inline void register_http(Interpreter& interpreter) {
    interpreter.registerNative("http_get", [](std::vector<Value> args) -> Value {
        if (args.empty()) return Value("undefined", 0, false);
        std::string url = args[0].toString();
        std::string res = fetch(url);
        return Value(res, 0, false);
    });

    interpreter.registerNative("http_post", [](std::vector<Value> args) -> Value {
        if (args.size() < 2) return Value("undefined", 0, false);
        std::string url = args[0].toString();
        std::string body = args[1].toString();

        CURL* curl;
        CURLcode res;
        std::string readBuffer;

        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Sunda/1.0");

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                std::cerr << "[HTTP Error] POST failed: " << curl_easy_strerror(res) << std::endl;
            }
            curl_easy_cleanup(curl);
        }
        return Value(readBuffer, 0, false);
    });
}

} // namespace HTTPLib

#endif

#ifndef SUNDA_HTTP_LIB_H
#define SUNDA_HTTP_LIB_H

#include "../../core/lang/interpreter.h"
#include <curl/curl.h>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

namespace HTTPLib {

// Helper for curl write callback
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Internal fetch for both module and interpreter (remote imports)
static std::string fetch(const std::string& method, const std::string& url, const std::string& body = "", const std::map<std::string, std::string>& headers = {}) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Sunda/1.0");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        // Set Method
        if (method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        } else if (method == "PUT") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        } else if (method == "PATCH") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        } else if (method == "DELETE") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
            if (!body.empty()) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            }
        }

        // Headers
        struct curl_slist* chunk = NULL;
        for (auto const& [key, val] : headers) {
            std::string headerStr = key + ": " + val;
            chunk = curl_slist_append(chunk, headerStr.c_str());
        }
        if (chunk) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        }

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "[HTTP Error] fetch failed: " << curl_easy_strerror(res) << " for URL: " << url << std::endl;
        } else {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code >= 400) {
                 std::cerr << "[HTTP Warning] Received HTTP " << response_code << " for " << url << std::endl;
                 if (response_code == 404) readBuffer = "";
            }
        }
        
        if (chunk) curl_slist_free_all(chunk);
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}

// Helper to extract headers from options map
static std::map<std::string, std::string> extract_headers(const Value& options) {
    std::map<std::string, std::string> headers;
    if (options.isMap) {
        auto it = options.mapVal->find("headers");
        if (it != options.mapVal->end() && it->second.isMap) {
            for (auto const& [key, val] : *it->second.mapVal) {
                headers[key] = val.toString();
            }
        }
    }
    return headers;
}

inline void register_http(Interpreter& interpreter) {
    interpreter.registerNative("http_get", [](std::vector<Value> args) -> Value {
        if (args.empty()) return Value("undefined", 0, false);
        std::string url = args[0].toString();
        std::map<std::string, std::string> headers;
        if (args.size() > 1) headers = extract_headers(args[1]);
        
        std::string res = fetch("GET", url, "", headers);
        return Value(res, 0, false);
    });

    interpreter.registerNative("http_post", [](std::vector<Value> args) -> Value {
        if (args.size() < 2) return Value("undefined", 0, false);
        std::string url = args[0].toString();
        std::string body = args[1].toString();
        std::map<std::string, std::string> headers;
        if (args.size() > 2) headers = extract_headers(args[2]);

        std::string res = fetch("POST", url, body, headers);
        return Value(res, 0, false);
    });

    interpreter.registerNative("http_put", [](std::vector<Value> args) -> Value {
        if (args.size() < 2) return Value("undefined", 0, false);
        std::string url = args[0].toString();
        std::string body = args[1].toString();
        std::map<std::string, std::string> headers;
        if (args.size() > 2) headers = extract_headers(args[2]);

        std::string res = fetch("PUT", url, body, headers);
        return Value(res, 0, false);
    });

    interpreter.registerNative("http_patch", [](std::vector<Value> args) -> Value {
        if (args.size() < 2) return Value("undefined", 0, false);
        std::string url = args[0].toString();
        std::string body = args[1].toString();
        std::map<std::string, std::string> headers;
        if (args.size() > 2) headers = extract_headers(args[2]);

        std::string res = fetch("PATCH", url, body, headers);
        return Value(res, 0, false);
    });

    interpreter.registerNative("http_delete", [](std::vector<Value> args) -> Value {
        if (args.empty()) return Value("undefined", 0, false);
        std::string url = args[0].toString();
        std::string body = "";
        std::map<std::string, std::string> headers;
        
        if (args.size() > 1) {
            if (!args[1].isMap) {
                body = args[1].toString();
                if (args.size() > 2) headers = extract_headers(args[2]);
            } else {
                headers = extract_headers(args[1]);
            }
        }

        std::string res = fetch("DELETE", url, body, headers);
        return Value(res, 0, false);
    });

    interpreter.registerNative("http", [](std::vector<Value> args) -> Value {
        if (args.empty()) return Value("undefined", 0, false);
        std::string url = args[0].toString();
        std::string method = "GET";
        std::string body = "";
        std::map<std::string, std::string> headers;

        if (args.size() > 1 && args[1].isMap) {
            const Value& opts = args[1];
            auto m_it = opts.mapVal->find("method");
            if (m_it != opts.mapVal->end()) {
                method = m_it->second.toString();
                // Uppercase the method
                std::transform(method.begin(), method.end(), method.begin(), ::toupper);
            }

            auto b_it = opts.mapVal->find("body");
            if (b_it != opts.mapVal->end()) {
                body = b_it->second.toString();
            }

            headers = extract_headers(opts);
        }

        std::string res = fetch(method, url, body, headers);
        return Value(res, 0, false);
    });
}

} // namespace HTTPLib

#endif

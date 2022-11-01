#include "main.hpp"
#include <stdio.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

Aws::String getOIDCToken(const std::string &KCHost, const std::string &KCRealm,
                         const std::string &clientId, const std::string &clientSecret)
{
  CURL *curl;
  CURLcode res;
  std::string readBuffer;
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  auto curl_url = KCHost + "/realms/" +
                  KCRealm + "/protocol/openid-connect/token";
  auto curl_data = "client_id=" + clientId +
                   "&client_secret=" + clientSecret +
                   "&grant_type=client_credentials";
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, curl_url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, curl_data.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();
  nlohmann::json data = nlohmann::json::parse(readBuffer);
  Aws::String accessToken = data["access_token"].get<std::string>();
  return accessToken;
}

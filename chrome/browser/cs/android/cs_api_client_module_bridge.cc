#include "cs_api_client_module_bridge.h"

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/cs/android/cs_api_client_service.h"
#include "chrome/browser/cs/android/cs_api_client_service_factory.h"

#include "chrome/browser/cs/android/jni_headers/CSApiClientModuleBridge_jni.h"

using jni_zero::JavaParamRef;
using jni_zero::JavaRef;

namespace {
std::unordered_map<std::string, std::string> parseQuery(
    const std::string& query) {
  std::unordered_map<std::string, std::string> params;
  std::istringstream stream(query);
  std::string pair;

  while (std::getline(stream, pair, '&')) {
    size_t equals_pos = pair.find('=');
    if (equals_pos != std::string::npos) {
      std::string key = pair.substr(0, equals_pos);
      std::string value = pair.substr(equals_pos + 1);
      params[key] = value;
    }
  }

  return params;
}
} // namespace

namespace cs_api_client_module {
CSApiClientModuleBridge::CSApiClientModuleBridge(
  JNIEnv* env,
  const JavaRef<jobject>& jobj,
  Profile* profile
) : java_object_(env, jobj) {
  CHECK(!profile->IsOffTheRecord());
  cs_api_client_service_ =
        CSApiClientServiceFactory::GetInstance()->GetForBrowserContext(profile);
}

void CSApiClientModuleBridge::Destroy(
  JNIEnv* env,
  const JavaParamRef<jobject>& obj
) {
  delete this;
}

void CSApiClientModuleBridge::Handle(
  JNIEnv* env,
  const GURL& url
) {
  if (url != last_committed_url_) {
    last_committed_url_ = url;
    if (url.SchemeIsHTTPOrHTTPS()) {
      std::string url_to_submit;
      GURL::Replacements remove_query_and_ref;
      remove_query_and_ref.ClearQuery();
      remove_query_and_ref.ClearRef();
      url_to_submit = url.ReplaceComponents(remove_query_and_ref).spec();

      const std::string query = url.query();
      const std::string query_with_qm = std::string("?") + query;

      constexpr std::size_t npos = std::string::npos;
      const bool has_q_param = (query_with_qm.find("?q=") != npos) ||
                               (query_with_qm.find("&q=") != npos);

      const bool has_p_param = (query_with_qm.find("?p=") != npos) ||
                               (query_with_qm.find("&p=") != npos);

      const bool has_wd_or_word_param =
          (query_with_qm.find("?wd=") != npos) ||
          (query_with_qm.find("&wd=") != npos) ||
          (query_with_qm.find("?word=") != npos) ||
          (query_with_qm.find("&word=") != npos);

      const bool has_query_param = (query_with_qm.find("?query=") != npos) ||
                                   (query_with_qm.find("&query=") != npos);

      const bool has_text_param = (query_with_qm.find("?text=") != npos) ||
                                  (query_with_qm.find("&text=") != npos);

      const bool has_mt_param = (query_with_qm.find("?MT=") != npos) ||
                                (query_with_qm.find("&MT=") != npos);

      const bool has_s_param = (query_with_qm.find("?s=") != npos) ||
                               (query_with_qm.find("&s=") != npos);

      const bool has_search_term_param =
          has_q_param || has_p_param || has_wd_or_word_param ||
          has_query_param || has_text_param || has_mt_param || has_s_param;

      if (has_search_term_param) {
        auto params = parseQuery(query);
        if (has_q_param) {
          url_to_submit += "?q=" + params["q"];
        }
        if (has_p_param) {
          url_to_submit += "?p=" + params["p"];
        }
        if (has_wd_or_word_param) {
          url_to_submit +=
              "?wd=" +
              (params["wd"].length() > 0 ? params["wd"] : params["word"]);
        }
        if (has_query_param) {
          url_to_submit += "?query=" + params["query"];
        }
        if (has_text_param) {
          url_to_submit += "?text=" + params["text"];
        }
        if (has_mt_param) {
          url_to_submit += "?MT=" + params["MT"];
        }
        if (has_s_param) {
          url_to_submit += "?s=" + params["s"];
        }
      }

      cs_api_client_service_->Post(
        url_to_submit, base::BindOnce([](WebRequestResult result) {
          DVLOG(0) << " |>> CS post response code : "
                   << result.response_code();
          DVLOG(0) << " |>> CS post error code : " << result.error_code();
        }));
    }
  }
}

CSApiClientModuleBridge::~CSApiClientModuleBridge() = default;

static jlong JNI_CSApiClientModuleBridge_Create(
  JNIEnv* env,
  const JavaParamRef<jobject>& obj,
  Profile* profile
) {
  CSApiClientModuleBridge* native_bridge =
    new CSApiClientModuleBridge(env, obj, profile);
  return reinterpret_cast<intptr_t>(native_bridge);
}
} // namespace cs_api_client_module

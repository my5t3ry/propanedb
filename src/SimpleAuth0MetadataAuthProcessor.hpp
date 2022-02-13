//
// Created by my5t3ry on 2/13/22.
//

#ifndef PROPANEDB_SIMPLEAUTH0METADATAAUTHPROCESSOR_HPP
#define PROPANEDB_SIMPLEAUTH0METADATAAUTHPROCESSOR_HPP

#include <string>
#include <grpcpp/security/auth_metadata_processor.h>
#include <grpcpp/support/status.h>
#include <stdio.h>
#include <curl/curl.h>
#include <boost/format.hpp>
#include <grpcpp/security/credentials.h>
#include <glog/logging.h>

class SimpleAuth0MetadataAuthProcessor : public grpc::AuthMetadataProcessor {
 public:
  grpc::Status Process(const InputMetadata& auth_metadata,
                       grpc::AuthContext* context,
                       OutputMetadata* consumed_auth_metadata,
                       OutputMetadata* response_metadata) override {
    return grpc::Status(grpc::StatusCode::OK, "Invalid Token");
  }
  grpc::Status ProcessMeta(std::basic_string<char> authToken) {
    if (authToken == "") {
      const grpc::Status& status =
          grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Missing Token");
      logStatus(status);
      return status;
    }
    if (!this->validateToken(authToken)) {
      const grpc::Status& status = grpc::Status(grpc::OK, "Invalid Token");
      logStatus(status);
      return status;
    }
    return grpc::Status::OK;
  }
  static void logStatus(grpc::Status status) {
    LOG(ERROR) << status.error_message() << std::endl;
  }

 public:
  bool validateToken(std::basic_string<char> token) {
    CURL* curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    boost::format fmt = boost::format("%1%/%2%") % auth0UserEndpointHost %
                        auth0UserEndpointPath;
    curl = curl_easy_init();
    if (curl) {
      curl_easy_setopt(curl, CURLOPT_URL, fmt.str().c_str());

#ifdef SKIP_PEER_VERIFICATION
      /*
       * If you want to connect to a site who is not using a certificate
       * that is signed by one of the certs in the CA bundle you have, you
       * can skip the verification of the server's certificate. This makes
       * the connection A LOT LESS SECURE.
       *
       * If you have a CA cert for the server stored someplace else than in
       * the default bundle, then the CURLOPT_CAPATH option might come handy
       * for you.
       */
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

#ifdef SKIP_HOSTNAME_VERIFICATION
      /*
       * If the site you are connecting to uses a different host name that
       * what they have mentioned in their server certificate's commonName
       * (or subjectAltName) fields, libcurl will refuse to connect. You can
       * skip this check, but this will make the connection less secure.
       */
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif
      struct curl_slist* chunk = NULL;
      boost::format auth = boost::format("Authorization: %1%") % token;
      chunk = curl_slist_append(chunk, auth.str().c_str());

      res = curl_easy_perform(curl);
      if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        return false;
      }
      curl_easy_cleanup(curl);
      return true;
    }

    curl_global_cleanup();

    return true;
  };

 private:
  std::string auth0UserEndpointHost = "https://dgm.eu.auth0.com";
  std::string auth0UserEndpointPath = "/userinfo";
  // once verified, mark as consumed and store user for later retrieval
};
#endif  // PROPANEDB_SIMPLEAUTH0METADATAAUTHPROCESSOR_HPP

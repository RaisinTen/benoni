#include <http.h>

#include <Windows.h>
#include <winhttp.h>

#include <cassert> // assert
#include <map>     // std::map
#include <sstream> // std::stringtream
#include <string>  // std::string
#include <variant> // std::variant

namespace req {
namespace {

class Session {
public:
  Session(
      const std::function<void(std::variant<std::string, Response>)> &callback)
      // Use WinHttpOpen to obtain a session handle.
      : hSession_{WinHttpOpen(L"Req/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                              WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS,
                              WINHTTP_FLAG_ASYNC)} {
    if (hSession_ != nullptr) {
      return;
    }

    DWORD err = GetLastError();
    switch (err) {
    case ERROR_WINHTTP_INTERNAL_ERROR:
      callback("WinHttpOpen Error: An internal error has occurred");
      return;
    case ERROR_NOT_ENOUGH_MEMORY:
      callback("WinHttpOpen Error: Not enough memory was available to complete "
               "the requested operation");
      return;
    }
    callback("WinHttpOpen Error: " + std::to_string(err));
  }

  ~Session() { WinHttpCloseHandle(hSession_); }

  auto Get() -> HINTERNET { return hSession_; }

private:
  HINTERNET hSession_;
};

class Connection {
public:
  Connection(
      const std::function<void(std::variant<std::string, Response>)> &callback,
      HINTERNET hSession)
      // Specify an HTTP server.
      : hConnection_{WinHttpConnect(hSession, L"www.postman-echo.com",
                                    INTERNET_DEFAULT_HTTPS_PORT, 0)} {
    if (hConnection_ != nullptr) {
      return;
    }

    DWORD err = GetLastError();
    switch (err) {
    case ERROR_WINHTTP_INCORRECT_HANDLE_TYPE:
      callback("WinHttpConnect Error: The type of handle supplied is "
               "incorrect for "
               "this operation");
      return;
    case ERROR_WINHTTP_INTERNAL_ERROR:
      callback("WinHttpConnect Error: An internal error has occurred");
      return;
    case ERROR_WINHTTP_INVALID_URL:
      callback("WinHttpConnect Error: The URL is invalid");
      return;
    case ERROR_WINHTTP_OPERATION_CANCELLED:
      callback(
          "WinHttpConnect Error: The operation was canceled, usually because "
          "the handle on which the request was operating was "
          "closed before the operation completed");
      return;
    case ERROR_WINHTTP_UNRECOGNIZED_SCHEME:
      callback(
          "WinHttpConnect Error: The operation was canceled, usually because "
          "the handle on which the request was operating was "
          "closed before the operation completed");
      return;
    case ERROR_WINHTTP_SHUTDOWN:
      callback(
          "WinHttpConnect Error: The WinHTTP function support is being shut "
          "down or unloaded");
      return;
    case ERROR_NOT_ENOUGH_MEMORY:
      callback(
          "WinHttpConnect Error: Not enough memory was available to complete "
          "the requested operation");
      return;
    }
    callback("WinHttpConnect Error: " + std::to_string(err));
  }

  ~Connection() { WinHttpCloseHandle(hConnection_); }

  auto Get() -> HINTERNET { return hConnection_; }

private:
  HINTERNET hConnection_;
};

class Request {
public:
  Request(
      const std::function<void(std::variant<std::string, Response>)> &callback,
      HINTERNET hConnection)
      // Create an HTTP request handle.
      : hRequest_{WinHttpOpenRequest(
            hConnection, L"GET", L"/get", NULL, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE)} {
    if (hRequest_ != nullptr) {
      return;
    }

    DWORD err = GetLastError();
    switch (err) {
    case ERROR_WINHTTP_INCORRECT_HANDLE_TYPE:
      callback("WinHttpOpenRequest Error: The type of handle supplied "
               "is incorrect for "
               "this operation");
      return;
    case ERROR_WINHTTP_INTERNAL_ERROR:
      callback("WinHttpOpenRequest Error: An internal error has occurred");
      return;
    case ERROR_WINHTTP_INVALID_URL:
      callback("WinHttpOpenRequest Error: The URL is invalid");
      return;
    case ERROR_WINHTTP_OPERATION_CANCELLED:
      callback("WinHttpOpenRequest Error: The operation was canceled, "
               "usually because "
               "the handle on which the request was operating was "
               "closed before the operation completed");
      return;
    case ERROR_WINHTTP_UNRECOGNIZED_SCHEME:
      callback("WinHttpOpenRequest Error: The operation was canceled, "
               "usually because "
               "the handle on which the request was operating was "
               "closed before the operation completed");
      return;
    case ERROR_NOT_ENOUGH_MEMORY:
      callback("WinHttpOpenRequest Error: Not enough memory was "
               "available to complete "
               "the requested operation");
      return;
    }
    callback("WinHttpOpenRequest Error: " + std::to_string(err));
  }

  ~Request() { WinHttpCloseHandle(hRequest_); }

  auto Get() -> HINTERNET { return hRequest_; }

private:
  HINTERNET hRequest_;
};

class HTTPClient {
public:
  static auto
  Req(std::function<void(std::variant<std::string, Response>)> callback)
      -> void {
    new HTTPClient{std::move(callback)};
  }

private:
  HTTPClient(std::function<void(std::variant<std::string, Response>)> callback)
      : callback_{std::move(callback)}, session_{callback_},
        connection_{callback_, session_.Get()},
        request_{callback_, connection_.Get()}, status_{}, dwSize_{}, body_{} {
    DWORD_PTR option_context = reinterpret_cast<DWORD_PTR>(this);
    if (WinHttpSetOption(request_.Get(), WINHTTP_OPTION_CONTEXT_VALUE,
                         &option_context, sizeof(option_context)) == FALSE) {
      DWORD err = GetLastError();
      callback_("WinHttpSetOption Error: " + std::to_string(err));
      return;
    }

    if (WinHttpSetStatusCallback(request_.Get(),
                                 &HTTPClient::WinHttpStatusCallback,
                                 WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS,
                                 reinterpret_cast<DWORD_PTR>(nullptr)) ==
        WINHTTP_INVALID_STATUS_CALLBACK) {
      DWORD err = GetLastError();
      callback_("WinHttpSetStatusCallback Error: " + std::to_string(err));
      return;
    }

    // Send a request.
    if (WinHttpSendRequest(request_.Get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                           WINHTTP_NO_REQUEST_DATA, 0, 0,
                           option_context) == FALSE) {
      DWORD err = GetLastError();
      switch (err) {
      case ERROR_WINHTTP_CANNOT_CONNECT:
        callback("WinHttpSendRequest Error: Connection to the server failed");
        return;
      case ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED:
        callback("WinHttpSendRequest Error: The secure HTTP server "
                 "requires a client certificate");
        return;
      case ERROR_WINHTTP_CONNECTION_ERROR:
        callback("WinHttpSendRequest Error: The connection with the "
                 "server has been reset or terminated, or an "
                 "incompatible SSL protocol was encountered");
        return;
      case ERROR_WINHTTP_INCORRECT_HANDLE_STATE:
        callback("WinHttpSendRequest Error: The requested operation "
                 "cannot be carried out because the handle supplied is "
                 "not in the correct state");
        return;
      case ERROR_WINHTTP_INTERNAL_ERROR:
        callback("WinHttpSendRequest Error: An internal error has occurred");
        return;
      case ERROR_WINHTTP_INVALID_URL:
        callback("WinHttpSendRequest Error: The URL is invalid");
        return;
      case ERROR_WINHTTP_LOGIN_FAILURE:
        callback("WinHttpSendRequest Error: The login attempt failed");
        return;
      case ERROR_WINHTTP_NAME_NOT_RESOLVED:
        callback(
            "WinHttpSendRequest Error: The server name cannot be resolved");
        return;
      case ERROR_WINHTTP_OPERATION_CANCELLED:
        callback("WinHttpSendRequest Error: The operation was canceled, "
                 "usually because "
                 "the handle on which the request was operating was "
                 "closed before the operation completed");
        return;
      case ERROR_WINHTTP_RESPONSE_DRAIN_OVERFLOW:
        callback("WinHttpSendRequest Error: An incoming response "
                 "exceeds an internal WinHTTP size limit");
        return;
      case ERROR_WINHTTP_SECURE_FAILURE:
        callback(
            "WinHttpSendRequest Error: One or more errors were found in the "
            "Secure Sockets Layer (SSL) certificate sent by the server");
        return;
      case ERROR_WINHTTP_SHUTDOWN:
        callback("WinHttpSendRequest Error: The WinHTTP function "
                 "support is being shut "
                 "down or unloaded");
        return;
      case ERROR_WINHTTP_TIMEOUT:
        callback("WinHttpSendRequest Error: The request timed out");
        return;
      case ERROR_WINHTTP_UNRECOGNIZED_SCHEME:
        callback("WinHttpSendRequest Error: The URL specified a scheme "
                 "other than \"http:\" or \"https:\"");
        return;
      case ERROR_NOT_ENOUGH_MEMORY:
        callback("WinHttpSendRequest Error: Not enough memory was "
                 "available to complete "
                 "the requested operation");
        return;
      case ERROR_INVALID_PARAMETER:
        callback(
            "WinHttpSendRequest Error: The content length specified in the "
            "dwTotalLength parameter does not match the length specified in "
            "the Content-Length header");
        return;
      case ERROR_WINHTTP_RESEND_REQUEST:
        // TODO(RaisinTen): Resend the request.
        callback("WinHttpSendRequest Error: Resend request");
        return;
      }
      callback("WinHttpSendRequest Error: " + std::to_string(err));
      return;
    }
  }

  ~HTTPClient() {
    if (WinHttpSetStatusCallback(request_.Get(), nullptr,
                                 WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS,
                                 reinterpret_cast<DWORD_PTR>(nullptr)) ==
        WINHTTP_INVALID_STATUS_CALLBACK) {
      DWORD err = GetLastError();
      callback_("WinHttpSetStatusCallback Error: " + std::to_string(err));
      return;
    }
  }

  auto receive_response() -> void {
    if (WinHttpReceiveResponse(request_.Get(), nullptr) == FALSE) {
      DWORD err = GetLastError();
      switch (err) {
      case ERROR_WINHTTP_CANNOT_CONNECT:
        callback_(
            "WinHttpReceiveResponse Error: Connection to the server failed");
        return;
      case ERROR_WINHTTP_CHUNKED_ENCODING_HEADER_SIZE_OVERFLOW:
        callback_("WinHttpReceiveResponse Error: An overflow condition is "
                  "encountered in the course of parsing chunked encoding");
        return;
      case ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED:
        callback_("WinHttpReceiveResponse Error: The server requests "
                  "client authentication");
        return;
      case ERROR_WINHTTP_CONNECTION_ERROR:
        callback_("WinHttpReceiveResponse Error: The connection with the "
                  "server has been reset or terminated, or an "
                  "incompatible SSL protocol was encountered");
        return;
      case ERROR_WINHTTP_HEADER_COUNT_EXCEEDED:
        callback_(
            "WinHttpReceiveResponse Error: A larger number of headers were "
            "present in a response than WinHTTP could receive");
        return;
      case ERROR_WINHTTP_HEADER_SIZE_OVERFLOW:
        callback_("WinHttpReceiveResponse Error: The size of headers "
                  "received exceeds the limit for the request handle");
        return;
      case ERROR_WINHTTP_INCORRECT_HANDLE_STATE:
        callback_("WinHttpReceiveResponse Error: The requested operation "
                  "cannot be carried out because the handle supplied is "
                  "not in the correct state");
        return;
      case ERROR_WINHTTP_INCORRECT_HANDLE_TYPE:
        callback_("WinHttpReceiveResponse Error: The type of handle "
                  "supplied is incorrect for this operation");
        return;
      case ERROR_WINHTTP_INTERNAL_ERROR:
        callback_(
            "WinHttpReceiveResponse Error: An internal error has occurred");
        return;
      case ERROR_WINHTTP_INVALID_SERVER_RESPONSE:
        callback_("WinHttpReceiveResponse Error: The server response "
                  "could not be parsed");
        return;
      case ERROR_WINHTTP_INVALID_URL:
        callback_("WinHttpReceiveResponse Error: The URL is invalid");
        return;
      case ERROR_WINHTTP_LOGIN_FAILURE:
        callback_("WinHttpReceiveResponse Error: The login attempt failed");
        return;
      case ERROR_WINHTTP_NAME_NOT_RESOLVED:
        callback_("WinHttpReceiveResponse Error: The server name could "
                  "not be resolved");
        return;
      case ERROR_WINHTTP_OPERATION_CANCELLED:
        callback_(
            "WinHttpReceiveResponse Error: The operation was canceled, usually "
            "because the handle on which the request was operating was closed "
            "before the operation completed");
        return;
      case ERROR_WINHTTP_REDIRECT_FAILED:
        callback_("WinHttpReceiveResponse Error: The redirection failed "
                  "because either the scheme changed or all attempts "
                  "made to redirect failed");
        return;
      case ERROR_WINHTTP_RESEND_REQUEST:
        // TODO(RaisinTen): Resend the request.
        callback_("WinHttpReceiveResponse Error: Resend request");
        return;
      case ERROR_WINHTTP_RESPONSE_DRAIN_OVERFLOW:
        callback_("WinHttpReceiveResponse Error: An incoming response "
                  "exceeds an internal WinHTTP size limit");
        return;
      case ERROR_WINHTTP_SECURE_FAILURE:
        callback_(
            "WinHttpReceiveResponse Error: One or more errors were found in "
            "the Secure Sockets Layer (SSL) certificate sent by the server");
        return;
      case ERROR_WINHTTP_TIMEOUT:
        callback_("WinHttpReceiveResponse Error: The request has timed out");
        return;
      case ERROR_WINHTTP_UNRECOGNIZED_SCHEME:
        callback_("WinHttpReceiveResponse Error: The URL specified a "
                  "scheme other than \"http:\" or \"https:\"");
        return;
      case ERROR_NOT_ENOUGH_MEMORY:
        callback_("WinHttpReceiveResponse Error: Not enough memory was "
                  "available to complete the requested operation");
        return;
      }
      callback_("WinHttpReceiveResponse Error: " + std::to_string(err));
      return;
    }
  }

  auto capture_status_code() -> void {
    DWORD dwStatusCode = 0;
    DWORD lpdwBufferLength = sizeof(DWORD);
    if (WinHttpQueryHeaders(
            request_.Get(),
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr,
            &dwStatusCode, &lpdwBufferLength, nullptr) == FALSE) {
      DWORD err = GetLastError();
      switch (err) {
      case ERROR_WINHTTP_HEADER_NOT_FOUND:
        callback_("WinHttpQueryHeaders Error: The requested header could "
                  "not be located");
        return;
      case ERROR_WINHTTP_INCORRECT_HANDLE_STATE:
        callback_("WinHttpQueryHeaders Error: The requested operation "
                  "cannot be carried out because the handle supplied is "
                  "not in the correct state");
        return;
      case ERROR_WINHTTP_INCORRECT_HANDLE_TYPE:
        callback_("WinHttpQueryHeaders Error: The type of handle "
                  "supplied is incorrect for this operation");
        return;
      case ERROR_WINHTTP_INTERNAL_ERROR:
        callback_("WinHttpQueryHeaders Error: An internal error has occurred");
        return;
      case ERROR_NOT_ENOUGH_MEMORY:
        callback_("WinHttpQueryHeaders Error: Not enough memory was "
                  "available to complete the requested operation");
        return;
      }
      callback_("WinHttpQueryHeaders Error: " + std::to_string(err));
      return;
    }
    status_ = dwStatusCode;
  }

  auto query_data() -> void {
    // Triggers the WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE completion callback.
    if (WinHttpQueryDataAvailable(request_.Get(), nullptr) == FALSE) {
      DWORD err = GetLastError();
      switch (err) {
      case ERROR_WINHTTP_CONNECTION_ERROR:
        callback_("WinHttpQueryDataAvailable Error: The connection "
                  "with the server has been reset or terminated, or an "
                  "incompatible SSL protocol was encountered");
        return;
      case ERROR_WINHTTP_INCORRECT_HANDLE_STATE:
        callback_("WinHttpQueryDataAvailable Error: The requested operation "
                  "cannot be carried out because the handle supplied is "
                  "not in the correct state");
        return;
      case ERROR_WINHTTP_INCORRECT_HANDLE_TYPE:
        callback_("WinHttpQueryDataAvailable Error: The type of handle "
                  "supplied is incorrect for this operation");
        return;
      case ERROR_WINHTTP_INTERNAL_ERROR:
        callback_("WinHttpQueryDataAvailable Error: An internal error "
                  "has occurred");
        return;
      case ERROR_WINHTTP_OPERATION_CANCELLED:
        callback_(
            "WinHttpQueryDataAvailable Error: The operation was canceled, "
            "usually because the handle on which the request was operating "
            "was closed before the operation complete");
        return;
      case ERROR_WINHTTP_TIMEOUT:
        callback_("WinHttpQueryDataAvailable Error: The request has timed out");
        return;
      case ERROR_NOT_ENOUGH_MEMORY:
        callback_("WinHttpQueryDataAvailable Error: Not enough memory was "
                  "available to complete the requested operation");
        return;
      }
      callback_("WinHttpQueryDataAvailable Error: " + std::to_string(err));
      return;
    }
  }

  auto handle_data_available(DWORD dwSize) -> void {
    dwSize_ = dwSize;
    if (dwSize_ == 0) {
      // complete
      callback_(Response{body_.str(), status_, {}});
      delete this;
      return;
    } else {
      std::unique_ptr<char[]> lpOutBuffer{new char[dwSize_]};
      ZeroMemory(lpOutBuffer.get(), dwSize_);

      if (WinHttpReadData(request_.Get(), lpOutBuffer.get(), dwSize_,
                          nullptr) == FALSE) {
        DWORD err = GetLastError();
        switch (err) {
        case ERROR_WINHTTP_CONNECTION_ERROR:
          callback_("WinHttpReadData Error: The connection "
                    "with the server has been reset or terminated, or an "
                    "incompatible SSL protocol was encountered");
          return;
        case ERROR_WINHTTP_INCORRECT_HANDLE_STATE:
          callback_("WinHttpReadData Error: The requested operation "
                    "cannot be carried out because the handle supplied is "
                    "not in the correct state");
          return;
        case ERROR_WINHTTP_INCORRECT_HANDLE_TYPE:
          callback_("WinHttpReadData Error: The type of handle "
                    "supplied is incorrect for this operation");
          return;
        case ERROR_WINHTTP_INTERNAL_ERROR:
          callback_("WinHttpReadData Error: An internal error "
                    "has occurred");
          return;
        case ERROR_WINHTTP_OPERATION_CANCELLED:
          callback_(
              "WinHttpReadData Error: The operation was canceled, "
              "usually because the handle on which the request was operating "
              "was closed before the operation complete");
          return;
        case ERROR_WINHTTP_RESPONSE_DRAIN_OVERFLOW:
          callback_("WinHttpReadData Error: An incoming response exceeds "
                    "an internal WinHTTP size limit");
          return;
        case ERROR_WINHTTP_TIMEOUT:
          callback_("WinHttpReadData Error: The request has timed out");
          return;
        case ERROR_NOT_ENOUGH_MEMORY:
          callback_("WinHttpReadData Error: Not enough memory was "
                    "available to complete the requested operation");
          return;
        }
        callback_("WinHttpReadData Error: " + std::to_string(err));
        return;
      }

      // Release ownership to WinHttpReadData, now that it has succeeded.
      lpOutBuffer.release();
    }
  }

  auto complete_reading(char *buffer, std::size_t bytes_read) -> void {
    if (bytes_read == 0) {
      return;
    }

    // Append the read data to the response body and delete it.
    std::unique_ptr<char[]> raw_data{static_cast<char *>(buffer)};
    std::string data{raw_data.get(), bytes_read};
    body_ << data;
  }

  static auto WinHttpStatusCallback(HINTERNET hInternet, DWORD_PTR dwContext,
                                    DWORD dwInternetStatus,
                                    LPVOID lpvStatusInformation,
                                    DWORD dwStatusInformationLength) -> void {
    assert(dwContext);
    auto http_client = reinterpret_cast<HTTPClient *>(dwContext);
    switch (dwInternetStatus) {
    case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
      http_client->receive_response();
      return;
    case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
      http_client->capture_status_code();
      http_client->query_data();
      return;
    case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
      http_client->handle_data_available(
          *static_cast<DWORD *>(lpvStatusInformation));
      return;
    case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
      http_client->complete_reading(static_cast<char *>(lpvStatusInformation),
                                    dwStatusInformationLength);
      http_client->query_data();
      return;
    }
  }

  std::function<void(std::variant<std::string, Response>)> callback_;

  Session session_;
  Connection connection_;
  Request request_;

  uint16_t status_;
  DWORD dwSize_;
  std::stringstream body_;
};

} // namespace

auto request(const std::string &url, RequestOptions options,
             std::function<void(std::variant<std::string, Response>)> callback)
    -> void {
  HTTPClient::Req(std::move(callback));
}

} // namespace req

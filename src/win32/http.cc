#include <benoni/http.h>

#include <Windows.h>
#include <winhttp.h>

#include <cassert> // assert
#include <map>     // std::map
#include <sstream> // std::stringtream
#include <string>  // std::string
#include <variant> // std::variant

namespace benoni {
namespace {

// Refs:
// https://github.com/microsoft/cpprestsdk/blob/411a109150b270f23c8c97fa4ec9a0a4a98cdecf/Release/src/utilities/asyncrt_utils.cpp#L236-L263
std::string error_message(int errorCode) {
  const size_t buffer_size = 4096;
  DWORD dwFlags = FORMAT_MESSAGE_FROM_SYSTEM;
  LPCVOID lpSource = NULL;

#if !defined(__cplusplus_winrt)
  if (errorCode >= 12000) {
    dwFlags = FORMAT_MESSAGE_FROM_HMODULE;
    lpSource = GetModuleHandleA(
        "winhttp.dll"); // This handle DOES NOT need to be freed.
  }
#endif

  std::string buffer(buffer_size, 0);

  const auto result = ::FormatMessageA(dwFlags, lpSource, errorCode, 0,
                                       &buffer[0], buffer_size, nullptr);

  if (result == 0) {
    return "Unable to get an error message for error code: " +
           std::to_string(errorCode);
  }

  // Strip exceeding characters of the initial resize call.
  buffer.resize(result - 2); // -2 to exclude the carriage return and the line
                             // break characters at the end.

  return buffer;
}

class URL {
public:
  URL(const std::function<void(std::variant<std::string, Response>)> &callback,
      std::string url)
      : url_{url.begin(), url.end()} {
    // Initialize the URL_COMPONENTS structure.
    ZeroMemory(&urlComp_, sizeof(urlComp_));
    urlComp_.dwStructSize = sizeof(urlComp_);

    // Set required component lengths to non-zero, so that they are cracked.
    urlComp_.dwSchemeLength = (DWORD)-1;
    urlComp_.dwHostNameLength = (DWORD)-1;
    urlComp_.dwUrlPathLength = (DWORD)-1;
    urlComp_.dwExtraInfoLength = (DWORD)-1;

    if (WinHttpCrackUrl(url_.c_str(), url_.length(), 0, &urlComp_) == FALSE) {
      DWORD err = GetLastError();
      callback("WinHttpCrackUrl Error: " + error_message(err));
      return;
    }
  }

  auto scheme() -> std::wstring const {
    return {urlComp_.lpszScheme, urlComp_.dwSchemeLength};
  }

  auto hostname() -> std::wstring const {
    return {urlComp_.lpszHostName, urlComp_.dwHostNameLength};
  }

  auto path() -> std::wstring const {
    return {urlComp_.lpszUrlPath, urlComp_.dwUrlPathLength};
  }

  auto extra() -> std::wstring const {
    return {urlComp_.lpszExtraInfo, urlComp_.dwExtraInfoLength};
  }

private:
  std::wstring url_;
  URL_COMPONENTS urlComp_;
};

class Session {
public:
  Session(
      const std::function<void(std::variant<std::string, Response>)> &callback)
      // Use WinHttpOpen to obtain a session handle.
      : hSession_{WinHttpOpen(L"Benoni/1.0",
                              WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                              WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS,
                              WINHTTP_FLAG_ASYNC)} {
    if (hSession_ != nullptr) {
      return;
    }

    DWORD err = GetLastError();
    callback("WinHttpOpen Error: " + error_message(err));
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
      HINTERNET hSession, std::wstring hostname)
      // Specify an HTTP server.
      : hConnection_{WinHttpConnect(hSession, hostname.c_str(),
                                    INTERNET_DEFAULT_PORT, 0)} {
    if (hConnection_ != nullptr) {
      return;
    }

    DWORD err = GetLastError();
    callback("WinHttpConnect Error: " + error_message(err));
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
      HINTERNET hConnection, std::wstring scheme, std::wstring path,
      Method method) {
    const wchar_t *method_string = nullptr;
    switch (method) {
    case Method::GET:
      method_string = L"GET";
      break;
    case Method::HEAD:
      method_string = L"HEAD";
      break;
    case Method::POST:
      method_string = L"POST";
      break;
    case Method::PUT:
      method_string = L"PUT";
      break;
    case Method::CONNECT:
      method_string = L"CONNECT";
      break;
    case Method::OPTIONS:
      method_string = L"OPTIONS";
      break;
    case Method::TRACE:
      method_string = L"TRACE";
      break;
    case Method::PATCH:
      method_string = L"PATCH";
      break;
    default:
      // Method::DELETE is assumed in the default case because MSVC cannot
      // handle "DELETE" as the name of an enum. It causes this compilation
      // error: C2589: '(': illegal token on right side of '::'.
      method_string = L"DELETE";
      break;
    }

    // Create an HTTP request handle.
    hRequest_ =
        WinHttpOpenRequest(hConnection, method_string, path.c_str(), NULL,
                           WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                           (scheme == L"https" ? WINHTTP_FLAG_SECURE : 0));
    if (hRequest_ != nullptr) {
      return;
    }

    DWORD err = GetLastError();
    callback("WinHttpOpenRequest Error: " + error_message(err));
  }

  ~Request() { WinHttpCloseHandle(hRequest_); }

  auto Get() -> HINTERNET { return hRequest_; }

private:
  HINTERNET hRequest_;
};

class HTTPClient {
public:
  static auto
  Req(std::string url, Method method,
      std::function<void(std::variant<std::string, Response>)> callback)
      -> void {
    new HTTPClient{std::move(url), method, std::move(callback)};
  }

private:
  HTTPClient(std::string url, Method method,
             std::function<void(std::variant<std::string, Response>)> callback)
      : callback_{std::move(callback)}, url_{callback_, std::move(url)},
        session_{callback_},
        connection_{callback_, session_.Get(), url_.hostname()},
        request_{callback_, connection_.Get(), url_.scheme(), url_.path(),
                 method},
        status_{}, headers_{}, dwSize_{}, body_{} {
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
      callback("WinHttpSendRequest Error: " + error_message(err));
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

  auto handle_error(const WINHTTP_ASYNC_RESULT &error) -> void {
    switch (error.dwResult) {
    case API_RECEIVE_RESPONSE:
      callback_("Async WinHttpReceiveResponse Error: " +
                error_message(error.dwError));
      return;
    case API_QUERY_DATA_AVAILABLE:
      callback_("Async WinHttpQueryDataAvailable Error: " +
                error_message(error.dwError));
      return;
    case API_READ_DATA:
      callback_("Async WinHttpReadData Error: " + error_message(error.dwError));
      return;
    case API_WRITE_DATA:
      callback_("Async WinHttpWriteData Error: " +
                error_message(error.dwError));
      return;
    case API_SEND_REQUEST:
      callback_("Async WinHttpSendRequest Error: " +
                error_message(error.dwError));
      return;
    case API_GET_PROXY_FOR_URL:
      callback_("Async WinHttpGetProxyForUrlEx Error: " +
                error_message(error.dwError));
      return;
    }
    callback_("Async " + std::to_string(error.dwResult) +
              " WinHttp Error: " + error_message(error.dwError));
    return;
  }

  auto receive_response() -> void {
    if (WinHttpReceiveResponse(request_.Get(), nullptr) == FALSE) {
      DWORD err = GetLastError();
      callback_("WinHttpReceiveResponse Error: " + error_message(err));
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
      callback_("WinHttpQueryHeaders Error: " + error_message(err));
      return;
    }
    status_ = dwStatusCode;
  }

  auto capture_headers() -> void {
    WINHTTP_EXTENDED_HEADER *pHeaders = nullptr;
    DWORD dwHeadersCount = 0;

    DWORD dwBufferLength = 0;
    DWORD ret = WinHttpQueryHeadersEx(
        request_.Get(),
        WINHTTP_QUERY_EX_ALL_HEADERS | WINHTTP_QUERY_FLAG_WIRE_ENCODING, 0, 0,
        nullptr, nullptr, nullptr, &dwBufferLength, &pHeaders, &dwHeadersCount);
    if (ret != ERROR_INSUFFICIENT_BUFFER) {
      DWORD err = ret;
      callback_("WinHttpQueryHeadersEx Error: " + error_message(err));
      return;
    }

    std::unique_ptr<char[]> buffer{std::make_unique<char[]>(dwBufferLength)};

    ret = WinHttpQueryHeadersEx(request_.Get(),
                                WINHTTP_QUERY_EX_ALL_HEADERS |
                                    WINHTTP_QUERY_FLAG_WIRE_ENCODING,
                                0, 0, nullptr, nullptr, buffer.get(),
                                &dwBufferLength, &pHeaders, &dwHeadersCount);
    if (ret != 0) {
      DWORD err = ret;
      callback_("WinHttpQueryHeadersEx Error: " + error_message(err));
      return;
    }

    for (DWORD i = 0; i < dwHeadersCount; ++i) {
      headers_[pHeaders[i].pszName] = pHeaders[i].pszValue;
    }
  }

  auto query_data() -> void {
    // Triggers the WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE completion callback.
    if (WinHttpQueryDataAvailable(request_.Get(), nullptr) == FALSE) {
      DWORD err = GetLastError();
      callback_("WinHttpQueryDataAvailable Error: " + error_message(err));
      return;
    }
  }

  auto handle_data_available(DWORD dwSize) -> void {
    dwSize_ = dwSize;
    if (dwSize_ == 0) {
      // complete
      callback_(Response{body_.str(), status_, std::move(headers_)});
      delete this;
      return;
    } else {
      std::unique_ptr<char[]> lpOutBuffer{new char[dwSize_]};
      ZeroMemory(lpOutBuffer.get(), dwSize_);

      if (WinHttpReadData(request_.Get(), lpOutBuffer.get(), dwSize_,
                          nullptr) == FALSE) {
        DWORD err = GetLastError();
        callback_("WinHttpReadData Error: " + error_message(err));
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
    case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
      http_client->handle_error(
          *static_cast<WINHTTP_ASYNC_RESULT *>(lpvStatusInformation));
      return;
    case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
      http_client->receive_response();
      return;
    case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
      http_client->capture_status_code();
      http_client->capture_headers();
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

  URL url_;

  Session session_;
  Connection connection_;
  Request request_;

  uint16_t status_;
  std::map<std::string, std::string> headers_;
  DWORD dwSize_;
  std::stringstream body_;
};

} // namespace

auto request(const std::string &url, RequestOptions options,
             std::function<void(std::variant<std::string, Response>)> callback)
    -> void {
  HTTPClient::Req(url, options.method(), std::move(callback));
}

} // namespace benoni

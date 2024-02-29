#include <http.h>

#include <Windows.h>
#include <winhttp.h>

#include <format>  // std::format
#include <map>     // std::map
#include <sstream> // std::stringtream
#include <string>  // std::string
#include <thread>  // std::thread
#include <variant> // std::variant

namespace {

auto get_error_message() -> std::string {
  DWORD err = GetLastError();
  char msg[512];

  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, // dwFlags
                 nullptr,                    // lpSource
                 err,                        // dwMessageId
                 0,                          // dwLanguageId
                 msg,                        // lpBuffer
                 512,                        // nSize
                 nullptr);                   // *Arguments

  return std::format("Error {}: {}\n", err, msg);
}

} // namespace

namespace req {

auto request(const std::string &url, RequestOptions options,
             std::function<void(std::variant<std::string, Response>)> callback)
    -> void {
  std::thread([callback]() {
    // Use WinHttpOpen to obtain a session handle.
    HINTERNET hSession =
        WinHttpOpen(L"Req/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession == nullptr) {
      callback(get_error_message());
      return;
    }

    // Specify an HTTP server.
    HINTERNET hConnect = WinHttpConnect(hSession, L"www.postman-echo.com",
                                        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (hConnect == nullptr) {
      callback(get_error_message());
      return;
    }

    // Create an HTTP request handle.
    HINTERNET hRequest =
        WinHttpOpenRequest(hConnect, L"GET", L"/get", NULL, WINHTTP_NO_REFERER,
                           WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (hRequest == nullptr) {
      callback(get_error_message());
      return;
    }

    // Send a request.
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                           WINHTTP_NO_REQUEST_DATA, 0, 0, 0) == FALSE) {
      callback(get_error_message());
      return;
    }

    // End the request.
    if (WinHttpReceiveResponse(hRequest, NULL) == FALSE) {
      callback(get_error_message());
      return;
    }

    // Capture the status code.
    uint16_t status;
    DWORD dwStatusCode = 0;
    DWORD lpdwBufferLength = sizeof(DWORD);
    if (WinHttpQueryHeaders(
            hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            nullptr, &dwStatusCode, &lpdwBufferLength, nullptr) == FALSE) {
      callback(get_error_message());
      return;
    }
    status = dwStatusCode;

    // Keep checking for data until there is nothing left.
    std::stringstream body;
    DWORD lpdwNumberOfBytesAvailable = 0;
    do {
      // Check for available data.
      lpdwNumberOfBytesAvailable = 0;
      if (WinHttpQueryDataAvailable(hRequest, &lpdwNumberOfBytesAvailable) ==
          FALSE) {
        callback(get_error_message());
        return;
      }

      // Allocate space for the buffer.
      LPSTR pszOutBuffer = new char[lpdwNumberOfBytesAvailable + 1];
      if (pszOutBuffer == nullptr) {
        callback("Out of memory");
        return;
      }

      // Read the data.
      ZeroMemory(pszOutBuffer, lpdwNumberOfBytesAvailable + 1);

      DWORD dwDownloaded = 0;
      if (WinHttpReadData(hRequest, static_cast<LPVOID>(pszOutBuffer),
                          lpdwNumberOfBytesAvailable, &dwDownloaded) == FALSE) {
        callback(get_error_message());
        return;
      }
      body << pszOutBuffer;

      // Free the memory allocated to the buffer.
      delete[] pszOutBuffer;
    } while (lpdwNumberOfBytesAvailable > 0);

    // Close any open handles.
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    std::variant<std::string, Response> result;
    result = Response{body.str(), status, {}};
    callback(std::move(result));
  }).detach();
}

} // namespace req

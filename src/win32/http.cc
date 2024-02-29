#include <http.h>

#include <Windows.h>
#include <winhttp.h>

#include <map>     // std::map
#include <sstream> // std::stringtream
#include <string>  // std::string
#include <thread>  // std::thread
#include <variant> // std::variant

namespace req {

auto request(const std::string &url, RequestOptions options,
             std::function<void(std::variant<std::string, Response>)> callback)
    -> void {
  std::thread([callback]() {
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    LPSTR pszOutBuffer;
    BOOL bResults = FALSE;
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;

    // Use WinHttpOpen to obtain a session handle.
    hSession = WinHttpOpen(L"Req/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                           WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

    // Specify an HTTP server.
    if (hSession)
      hConnect = WinHttpConnect(hSession, L"www.postman-echo.com",
                                INTERNET_DEFAULT_HTTPS_PORT, 0);

    // Create an HTTP request handle.
    if (hConnect)
      hRequest = WinHttpOpenRequest(
          hConnect, L"GET", L"/get", NULL, WINHTTP_NO_REFERER,
          WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);

    // Send a request.
    if (hRequest)
      bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                    WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

    // End the request.
    if (bResults)
      bResults = WinHttpReceiveResponse(hRequest, NULL);

    std::stringstream body;

    // Keep checking for data until there is nothing left.
    if (bResults) {
      do {
        // Check for available data.
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
          printf("Error %u in WinHttpQueryDataAvailable.\n", GetLastError());

        // Allocate space for the buffer.
        pszOutBuffer = new char[dwSize + 1];
        if (!pszOutBuffer) {
          printf("Out of memory\n");
          dwSize = 0;
        } else {
          // Read the data.
          ZeroMemory(pszOutBuffer, dwSize + 1);

          if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize,
                               &dwDownloaded))
            printf("Error %u in WinHttpReadData.\n", GetLastError());
          else
            body << pszOutBuffer;

          // Free the memory allocated to the buffer.
          delete[] pszOutBuffer;
        }
      } while (dwSize > 0);
    }

    // Report any errors.
    if (!bResults)
      printf("Error %d has occurred.\n", GetLastError());

    // Close any open handles.
    if (hRequest)
      WinHttpCloseHandle(hRequest);
    if (hConnect)
      WinHttpCloseHandle(hConnect);
    if (hSession)
      WinHttpCloseHandle(hSession);

    std::variant<std::string, Response> result;
    result = Response{body.str(), 200, {}};
    callback(std::move(result));
  }).detach();
}

} // namespace req

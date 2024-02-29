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
    // Use WinHttpOpen to obtain a session handle.
    HINTERNET hSession =
        WinHttpOpen(L"Req/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession == nullptr) {
      DWORD err = GetLastError();
      switch (err) {
      case ERROR_WINHTTP_INTERNAL_ERROR:
        return callback("WinHttpOpen Error: An internal error has occurred");
      case ERROR_NOT_ENOUGH_MEMORY:
        return callback(
            "WinHttpOpen Error: Not enough memory was available to complete "
            "the requested operation");
      }
      return callback("WinHttpOpen Error: " + err);
    }

    // Specify an HTTP server.
    HINTERNET hConnect = WinHttpConnect(hSession, L"www.postman-echo.com",
                                        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (hConnect == nullptr) {
      DWORD err = GetLastError();
      switch (err) {
      case ERROR_WINHTTP_INCORRECT_HANDLE_TYPE:
        return callback("WinHttpConnect Error: The type of handle supplied is "
                        "incorrect for "
                        "this operation");
      case ERROR_WINHTTP_INTERNAL_ERROR:
        return callback("WinHttpConnect Error: An internal error has occurred");
      case ERROR_WINHTTP_INVALID_URL:
        return callback("WinHttpConnect Error: The URL is invalid");
      case ERROR_WINHTTP_OPERATION_CANCELLED:
        return callback(
            "WinHttpConnect Error: The operation was canceled, usually because "
            "the handle on which the request was operating was "
            "closed before the operation completed");
      case ERROR_WINHTTP_UNRECOGNIZED_SCHEME:
        return callback(
            "WinHttpConnect Error: The operation was canceled, usually because "
            "the handle on which the request was operating was "
            "closed before the operation completed");
      case ERROR_WINHTTP_SHUTDOWN:
        return callback(
            "WinHttpConnect Error: The WinHTTP function support is being shut "
            "down or unloaded");
      case ERROR_NOT_ENOUGH_MEMORY:
        return callback(
            "WinHttpConnect Error: Not enough memory was available to complete "
            "the requested operation");
      }
      return callback("WinHttpConnect Error: " + err);
    }

    // Create an HTTP request handle.
    HINTERNET hRequest =
        WinHttpOpenRequest(hConnect, L"GET", L"/get", NULL, WINHTTP_NO_REFERER,
                           WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (hRequest == nullptr) {
      DWORD err = GetLastError();
      switch (err) {
      case ERROR_WINHTTP_INCORRECT_HANDLE_TYPE:
        return callback("WinHttpOpenRequest Error: The type of handle supplied "
                        "is incorrect for "
                        "this operation");
      case ERROR_WINHTTP_INTERNAL_ERROR:
        return callback(
            "WinHttpOpenRequest Error: An internal error has occurred");
      case ERROR_WINHTTP_INVALID_URL:
        return callback("WinHttpOpenRequest Error: The URL is invalid");
      case ERROR_WINHTTP_OPERATION_CANCELLED:
        return callback("WinHttpOpenRequest Error: The operation was canceled, "
                        "usually because "
                        "the handle on which the request was operating was "
                        "closed before the operation completed");
      case ERROR_WINHTTP_UNRECOGNIZED_SCHEME:
        return callback("WinHttpOpenRequest Error: The operation was canceled, "
                        "usually because "
                        "the handle on which the request was operating was "
                        "closed before the operation completed");
      case ERROR_NOT_ENOUGH_MEMORY:
        return callback("WinHttpOpenRequest Error: Not enough memory was "
                        "available to complete "
                        "the requested operation");
      }
      return callback("WinHttpOpenRequest Error: " + err);
    }

    // Send a request.
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                           WINHTTP_NO_REQUEST_DATA, 0, 0, 0) == FALSE) {
      DWORD err = GetLastError();
      switch (err) {
      case ERROR_WINHTTP_CANNOT_CONNECT:
        return callback(
            "WinHttpSendRequest Error: Connection to the server failed");
      case ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED:
        return callback("WinHttpSendRequest Error: The secure HTTP server "
                        "requires a client certificate");
      case ERROR_WINHTTP_CONNECTION_ERROR:
        return callback("WinHttpSendRequest Error: The connection with the "
                        "server has been reset or terminated, or an "
                        "incompatible SSL protocol was encountered");
      case ERROR_WINHTTP_INCORRECT_HANDLE_STATE:
        return callback("WinHttpSendRequest Error: The requested operation "
                        "cannot be carried out because the handle supplied is "
                        "not in the correct state");
      case ERROR_WINHTTP_INTERNAL_ERROR:
        return callback(
            "WinHttpSendRequest Error: An internal error has occurred");
      case ERROR_WINHTTP_INVALID_URL:
        return callback("WinHttpSendRequest Error: The URL is invalid");
      case ERROR_WINHTTP_LOGIN_FAILURE:
        return callback("WinHttpSendRequest Error: The login attempt failed");
      case ERROR_WINHTTP_NAME_NOT_RESOLVED:
        return callback(
            "WinHttpSendRequest Error: The server name cannot be resolved");
      case ERROR_WINHTTP_OPERATION_CANCELLED:
        return callback("WinHttpSendRequest Error: The operation was canceled, "
                        "usually because "
                        "the handle on which the request was operating was "
                        "closed before the operation completed");
      case ERROR_WINHTTP_RESPONSE_DRAIN_OVERFLOW:
        return callback("WinHttpSendRequest Error: An incoming response "
                        "exceeds an internal WinHTTP size limit");
      case ERROR_WINHTTP_SECURE_FAILURE:
        return callback(
            "WinHttpSendRequest Error: One or more errors were found in the "
            "Secure Sockets Layer (SSL) certificate sent by the server");
      case ERROR_WINHTTP_SHUTDOWN:
        return callback("WinHttpSendRequest Error: The WinHTTP function "
                        "support is being shut "
                        "down or unloaded");
      case ERROR_WINHTTP_TIMEOUT:
        return callback("WinHttpSendRequest Error: The request timed out");
      case ERROR_WINHTTP_UNRECOGNIZED_SCHEME:
        return callback("WinHttpSendRequest Error: The URL specified a scheme "
                        "other than \"http:\" or \"https:\"");
      case ERROR_NOT_ENOUGH_MEMORY:
        return callback("WinHttpSendRequest Error: Not enough memory was "
                        "available to complete "
                        "the requested operation");
      case ERROR_INVALID_PARAMETER:
        return callback(
            "WinHttpSendRequest Error: The content length specified in the "
            "dwTotalLength parameter does not match the length specified in "
            "the Content-Length header");
      case ERROR_WINHTTP_RESEND_REQUEST:
        // TODO(RaisinTen): Resend the request.
        return callback("WinHttpSendRequest Error: Resend request");
      }
      return callback("WinHttpSendRequest Error: " + err);
    }

    // End the request.
    if (WinHttpReceiveResponse(hRequest, NULL) == FALSE) {
      DWORD err = GetLastError();
      switch (err) {
      case ERROR_WINHTTP_CANNOT_CONNECT:
        return callback(
            "WinHttpReceiveResponse Error: Connection to the server failed");
      case ERROR_WINHTTP_CHUNKED_ENCODING_HEADER_SIZE_OVERFLOW:
        return callback(
            "WinHttpReceiveResponse Error: An overflow condition is "
            "encountered in the course of parsing chunked encoding");
      case ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED:
        return callback("WinHttpReceiveResponse Error: The server requests "
                        "client authentication");
      case ERROR_WINHTTP_CONNECTION_ERROR:
        return callback("WinHttpReceiveResponse Error: The connection with the "
                        "server has been reset or terminated, or an "
                        "incompatible SSL protocol was encountered");
      case ERROR_WINHTTP_HEADER_COUNT_EXCEEDED:
        return callback(
            "WinHttpReceiveResponse Error: A larger number of headers were "
            "present in a response than WinHTTP could receive");
      case ERROR_WINHTTP_HEADER_SIZE_OVERFLOW:
        return callback("WinHttpReceiveResponse Error: The size of headers "
                        "received exceeds the limit for the request handle");
      case ERROR_WINHTTP_INCORRECT_HANDLE_STATE:
        return callback("WinHttpReceiveResponse Error: The requested operation "
                        "cannot be carried out because the handle supplied is "
                        "not in the correct state");
      case ERROR_WINHTTP_INCORRECT_HANDLE_TYPE:
        return callback("WinHttpReceiveResponse Error: The type of handle "
                        "supplied is incorrect for this operation");
      case ERROR_WINHTTP_INTERNAL_ERROR:
        return callback(
            "WinHttpReceiveResponse Error: An internal error has occurred");
      case ERROR_WINHTTP_INVALID_SERVER_RESPONSE:
        return callback("WinHttpReceiveResponse Error: The server response "
                        "could not be parsed");
      case ERROR_WINHTTP_INVALID_URL:
        return callback("WinHttpReceiveResponse Error: The URL is invalid");
      case ERROR_WINHTTP_LOGIN_FAILURE:
        return callback(
            "WinHttpReceiveResponse Error: The login attempt failed");
      case ERROR_WINHTTP_NAME_NOT_RESOLVED:
        return callback("WinHttpReceiveResponse Error: The server name could "
                        "not be resolved");
      case ERROR_WINHTTP_OPERATION_CANCELLED:
        return callback(
            "WinHttpReceiveResponse Error: The operation was canceled, usually "
            "because the handle on which the request was operating was closed "
            "before the operation completed");
      case ERROR_WINHTTP_REDIRECT_FAILED:
        return callback("WinHttpReceiveResponse Error: The redirection failed "
                        "because either the scheme changed or all attempts "
                        "made to redirect failed");
      case ERROR_WINHTTP_RESEND_REQUEST:
        // TODO(RaisinTen): Resend the request.
        return callback("WinHttpReceiveResponse Error: Resend request");
      case ERROR_WINHTTP_RESPONSE_DRAIN_OVERFLOW:
        return callback("WinHttpReceiveResponse Error: An incoming response "
                        "exceeds an internal WinHTTP size limit");
      case ERROR_WINHTTP_SECURE_FAILURE:
        return callback(
            "WinHttpReceiveResponse Error: One or more errors were found in "
            "the Secure Sockets Layer (SSL) certificate sent by the server");
      case ERROR_WINHTTP_TIMEOUT:
        return callback(
            "WinHttpReceiveResponse Error: The request has timed out");
      case ERROR_WINHTTP_UNRECOGNIZED_SCHEME:
        return callback("WinHttpReceiveResponse Error: The URL specified a "
                        "scheme other than \"http:\" or \"https:\"");
      case ERROR_NOT_ENOUGH_MEMORY:
        return callback("WinHttpReceiveResponse Error: Not enough memory was "
                        "available to complete the requested operation");
      }
      return callback("WinHttpReceiveResponse Error: " + err);
    }

    // Capture the status code.
    uint16_t status;
    DWORD dwStatusCode = 0;
    DWORD lpdwBufferLength = sizeof(DWORD);
    if (WinHttpQueryHeaders(
            hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            nullptr, &dwStatusCode, &lpdwBufferLength, nullptr) == FALSE) {
      DWORD err = GetLastError();
      switch (err) {
      case ERROR_WINHTTP_HEADER_NOT_FOUND:
        return callback("WinHttpQueryHeaders Error: The requested header could "
                        "not be located");
      case ERROR_WINHTTP_INCORRECT_HANDLE_STATE:
        return callback("WinHttpQueryHeaders Error: The requested operation "
                        "cannot be carried out because the handle supplied is "
                        "not in the correct state");
      case ERROR_WINHTTP_INCORRECT_HANDLE_TYPE:
        return callback("WinHttpQueryHeaders Error: The type of handle "
                        "supplied is incorrect for this operation");
      case ERROR_WINHTTP_INTERNAL_ERROR:
        return callback(
            "WinHttpQueryHeaders Error: An internal error has occurred");
      case ERROR_NOT_ENOUGH_MEMORY:
        return callback("WinHttpQueryHeaders Error: Not enough memory was "
                        "available to complete the requested operation");
      }
      return callback("WinHttpQueryHeaders Error: " + err);
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
        DWORD err = GetLastError();
        switch (err) {
        case ERROR_WINHTTP_CONNECTION_ERROR:
          return callback("WinHttpQueryDataAvailable Error: The connection "
                          "with the server has been reset or terminated, or an "
                          "incompatible SSL protocol was encountered");
        case ERROR_WINHTTP_INCORRECT_HANDLE_STATE:
          return callback(
              "WinHttpQueryDataAvailable Error: The requested operation "
              "cannot be carried out because the handle supplied is "
              "not in the correct state");
        case ERROR_WINHTTP_INCORRECT_HANDLE_TYPE:
          return callback("WinHttpQueryDataAvailable Error: The type of handle "
                          "supplied is incorrect for this operation");
        case ERROR_WINHTTP_INTERNAL_ERROR:
          return callback("WinHttpQueryDataAvailable Error: An internal error "
                          "has occurred");
        case ERROR_WINHTTP_OPERATION_CANCELLED:
          return callback(
              "WinHttpQueryDataAvailable Error: The operation was canceled, "
              "usually because the handle on which the request was operating "
              "was closed before the operation complete");
        case ERROR_WINHTTP_TIMEOUT:
          return callback(
              "WinHttpQueryDataAvailable Error: The request has timed out");
        case ERROR_NOT_ENOUGH_MEMORY:
          return callback(
              "WinHttpQueryDataAvailable Error: Not enough memory was "
              "available to complete the requested operation");
        }
        return callback("WinHttpQueryDataAvailable Error: " + err);
      }

      // Allocate space for the buffer.
      LPSTR pszOutBuffer = new char[lpdwNumberOfBytesAvailable + 1];
      if (pszOutBuffer == nullptr) {
        callback("Error: Out of memory");
        return;
      }

      // Read the data.
      ZeroMemory(pszOutBuffer, lpdwNumberOfBytesAvailable + 1);

      DWORD dwDownloaded = 0;
      if (WinHttpReadData(hRequest, static_cast<LPVOID>(pszOutBuffer),
                          lpdwNumberOfBytesAvailable, &dwDownloaded) == FALSE) {
        DWORD err = GetLastError();
        switch (err) {
        case ERROR_WINHTTP_CONNECTION_ERROR:
          return callback("WinHttpReadData Error: The connection "
                          "with the server has been reset or terminated, or an "
                          "incompatible SSL protocol was encountered");
        case ERROR_WINHTTP_INCORRECT_HANDLE_STATE:
          return callback(
              "WinHttpReadData Error: The requested operation "
              "cannot be carried out because the handle supplied is "
              "not in the correct state");
        case ERROR_WINHTTP_INCORRECT_HANDLE_TYPE:
          return callback("WinHttpReadData Error: The type of handle "
                          "supplied is incorrect for this operation");
        case ERROR_WINHTTP_INTERNAL_ERROR:
          return callback("WinHttpReadData Error: An internal error "
                          "has occurred");
        case ERROR_WINHTTP_OPERATION_CANCELLED:
          return callback(
              "WinHttpReadData Error: The operation was canceled, "
              "usually because the handle on which the request was operating "
              "was closed before the operation complete");
        case ERROR_WINHTTP_RESPONSE_DRAIN_OVERFLOW:
          return callback("WinHttpReadData Error: An incoming response exceeds "
                          "an internal WinHTTP size limit");
        case ERROR_WINHTTP_TIMEOUT:
          return callback("WinHttpReadData Error: The request has timed out");
        case ERROR_NOT_ENOUGH_MEMORY:
          return callback("WinHttpReadData Error: Not enough memory was "
                          "available to complete the requested operation");
        }
        return callback("WinHttpReadData Error: " + err);
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

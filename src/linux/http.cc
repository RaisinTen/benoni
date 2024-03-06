#include <http.h>

#include <libsoup/soup.h>

#include <map>     // std::map
#include <string>  // std::string
#include <variant> // std::variant

namespace req {

namespace {

auto my_callback (GObject *object, GAsyncResult *result, gpointer user_data) -> void {
  GError *error = nullptr;
  GInputStream *stream = soup_session_send_finish(SOUP_SESSION(object), result, &error);
  // TODO(RaisinTen): Read from the stream.
}

}

auto request(const std::string &url, RequestOptions options,
             std::function<void(std::variant<std::string, Response>)> callback)
    -> void {
  SoupSession *session = soup_session_new();
  SoupMessage *msg = soup_message_new("GET", "https://postman-echo.com/get");
  soup_session_send_async(session, msg, nullptr, my_callback, nullptr);
}

} // namespace req

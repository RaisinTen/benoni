#include <http.h>

#include <libsoup/soup.h>

#include <array>   // std::array
#include <cassert> // assert
#include <map>     // std::map
#include <sstream> // std::stringstream
#include <string>  // std::string
#include <variant> // std::variant

namespace req {
namespace {

struct AsyncHttpContext {
  std::array<uint8_t, 2048> buffer;
  std::stringstream response;
  std::function<void(std::variant<std::string, Response>)> callback;
};

auto stream_close_callback(GObject *source_object, GAsyncResult *res,
                           gpointer data) -> void {
  GInputStream *stream = G_INPUT_STREAM(source_object);
  auto async_http_context = static_cast<AsyncHttpContext *>(data);

  GError *error = nullptr;
  gboolean stream_closed = g_input_stream_close_finish(stream, res, &error);
  if (stream_closed == FALSE) {
    assert(error);
    g_object_unref(stream);
    auto callback = std::move(async_http_context->callback);
    delete async_http_context;
    callback(error->message);
    return;
  }

  g_object_unref(stream);

  std::string response = async_http_context->response.str();
  auto callback = std::move(async_http_context->callback);
  delete async_http_context;
  callback(Response{.body = std::move(response), .status = 0, .headers = {}});
}

auto stream_read_callback(GObject *source_object, GAsyncResult *res,
                          gpointer data) -> void {
  GInputStream *stream = G_INPUT_STREAM(source_object);
  auto async_http_context = static_cast<AsyncHttpContext *>(data);

  GError *error = nullptr;
  gssize bytes_read = g_input_stream_read_finish(stream, res, &error);
  if (bytes_read == -1) {
    assert(error);
    g_object_unref(stream);
    auto callback = std::move(async_http_context->callback);
    delete async_http_context;
    callback(error->message);
    return;
  }

  if (bytes_read == 0) {
    // end
    g_input_stream_close_async(stream, G_PRIORITY_DEFAULT, nullptr,
                               stream_close_callback, async_http_context);
    return;
  }

  assert(bytes_read > 0);

  for (gssize i = 0; i < bytes_read; ++i) {
    async_http_context->response << async_http_context->buffer[i];
  }

  g_input_stream_read_async(stream, async_http_context->buffer.data(),
                            async_http_context->buffer.max_size(),
                            G_PRIORITY_DEFAULT, nullptr, stream_read_callback,
                            async_http_context);
}

auto session_send_callback(GObject *object, GAsyncResult *result, gpointer data)
    -> void {
  auto async_http_context = static_cast<AsyncHttpContext *>(data);

  GError *error = nullptr;
  GInputStream *stream =
      soup_session_send_finish(SOUP_SESSION(object), result, &error);
  if (!stream) {
    assert(error);
    auto callback = std::move(async_http_context->callback);
    delete async_http_context;
    callback(error->message);
    return;
  }

  g_input_stream_read_async(stream, async_http_context->buffer.data(),
                            async_http_context->buffer.max_size(),
                            G_PRIORITY_DEFAULT, nullptr, stream_read_callback,
                            async_http_context);
}

} // namespace

auto request(const std::string &url, RequestOptions options,
             std::function<void(std::variant<std::string, Response>)> callback)
    -> void {
  SoupSession *session = soup_session_new();
  SoupMessage *msg = soup_message_new("GET", "https://postman-echo.com/get");

  auto async_http_context = new AsyncHttpContext{};
  async_http_context->callback = std::move(callback);
  soup_session_send_async(session, msg, G_PRIORITY_DEFAULT,
                          session_send_callback, async_http_context);
}

} // namespace req

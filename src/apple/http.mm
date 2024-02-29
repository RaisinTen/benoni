#include <http.h>

#import <Foundation/Foundation.h>

#include <map>     // std::map
#include <string>  // std::string
#include <variant> // std::variant

namespace req {

auto request(const std::string &url, RequestOptions options,
             std::function<void(std::variant<std::string, Response>)> callback)
    -> void {
  NSMutableURLRequest *request = [[NSMutableURLRequest alloc] init];

  [request
      setURL:[NSURL
                 URLWithString:
                     [NSString
                         stringWithCString:url.c_str()
                                  encoding:[NSString defaultCStringEncoding]]]];

  switch (options.method()) {
#define V(HTTP_METHOD)                                                         \
  case Method::HTTP_METHOD:                                                    \
    [request setHTTPMethod:@ #HTTP_METHOD];                                    \
    break;

    REQ_HTTP_METHODS(V)
#undef V
  }

  [request setHTTPBody:[NSData dataWithBytes:options.body().data()
                                      length:options.body().length()]];

  for (const auto &[key, value] : options.headers()) {
    NSString *key_nsstring =
        [NSString stringWithCString:key.c_str()
                           encoding:[NSString defaultCStringEncoding]];
    NSString *value_nsstring =
        [NSString stringWithCString:value.c_str()
                           encoding:[NSString defaultCStringEncoding]];
    [request addValue:value_nsstring forHTTPHeaderField:key_nsstring];
  }

  if (options.timeout().has_value()) {
    [request setTimeoutInterval:options.timeout().value()];
  }

  [[[NSURLSession sharedSession]
      dataTaskWithRequest:request
        completionHandler:[callback](NSData *_Nullable data,
                                     NSURLResponse *_Nullable response,
                                     NSError *_Nullable error) {
          // This completionHandler is run in a background thread.

          std::variant<std::string, Response> result;

          if (error != nil) {
            result = std::string([[error localizedDescription] UTF8String]);
          } else {
            NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;

            NSStringEncoding usedEncoding = NSUTF8StringEncoding;
            NSString *encodingName = [httpResponse textEncodingName];
            if (encodingName) {
              NSStringEncoding encoding =
                  CFStringConvertEncodingToNSStringEncoding(
                      CFStringConvertIANACharSetNameToEncoding(
                          (CFStringRef)encodingName));

              if (encoding != kCFStringEncodingInvalidId) {
                usedEncoding = encoding;
              }
            }

            NSString *responseString =
                [[NSString alloc] initWithData:data encoding:usedEncoding];

            if (responseString == nil) {
              result = "response body has invalid encoding";
            } else {
              std::map<std::string, std::string> headers;
              if ([httpResponse
                      respondsToSelector:@selector(allHeaderFields)]) {
                NSDictionary *allHeaderFields = [httpResponse allHeaderFields];
                for (NSString *key in allHeaderFields) {
                  headers[[key UTF8String]] =
                      [[allHeaderFields objectForKey:key] UTF8String];
                }
              }

              result =
                  Response{std::string([responseString UTF8String]),
                           static_cast<uint16_t>([httpResponse statusCode]),
                           std::move(headers)};
            }
          }

          callback(std::move(result));
        }] resume];
}

} // namespace req

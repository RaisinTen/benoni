#include <benoni/http.h>

#import <Foundation/Foundation.h>

#include <map>     // std::map
#include <string>  // std::string
#include <variant> // std::variant

namespace {

struct HTTPTaskContext {
  std::function<void(std::variant<std::string, benoni::Response>)> callback;
  uint16_t status;
  std::map<std::string, std::string> headers;
  NSMutableData *data;
  NSStringEncoding encoding;
};

} // namespace

@interface BenoniHTTPTaskContextWrap : NSObject
@property HTTPTaskContext *context;
- (BenoniHTTPTaskContextWrap *)initWithContext:(HTTPTaskContext *)context;
- (void)dealloc;
@end

@implementation BenoniHTTPTaskContextWrap
@synthesize context;

- (BenoniHTTPTaskContextWrap *)initWithContext:
    (HTTPTaskContext *)contextObject {
  self = [super init];
  if (self == nil) {
    return self;
  }
  context = contextObject;
  return self;
}

- (void)dealloc {
  delete context;
  [super dealloc];
}
@end

@interface BenoniHTTPSessionDelegate
    : NSObject <NSURLSessionDelegate, NSURLSessionTaskDelegate>
@property(readonly)
    NSMutableDictionary<NSNumber *, BenoniHTTPTaskContextWrap *> *contextMap;
- (BenoniHTTPSessionDelegate *)init;

#pragma mark - NSURLSessionDataDelegate
- (void)URLSession:(NSURLSession *)session
              dataTask:(NSURLSessionDataTask *)dataTask
    didReceiveResponse:(NSURLResponse *)response
     completionHandler:(void (^)(NSURLSessionResponseDisposition disposition))
                           completionHandler;

- (void)URLSession:(NSURLSession *)session
          dataTask:(NSURLSessionDataTask *)dataTask
    didReceiveData:(NSData *)data;

#pragma mark - NSURLSessionTaskDelegate
- (void)URLSession:(NSURLSession *)session
                    task:(NSURLSessionTask *)task
    didCompleteWithError:(NSError *)error;
@end

@implementation BenoniHTTPSessionDelegate
@synthesize contextMap = contextMap_;
- (BenoniHTTPSessionDelegate *)init {
  self = [super init];
  if (self == nil) {
    return self;
  }
  contextMap_ = [[NSMutableDictionary alloc] init];
  return self;
}

#pragma mark - NSURLSessionDataDelegate
- (void)URLSession:(NSURLSession *)session
              dataTask:(NSURLSessionDataTask *)dataTask
    didReceiveResponse:(NSURLResponse *)response
     completionHandler:(void (^)(NSURLSessionResponseDisposition disposition))
                           completionHandler {
  NSNumber *key =
      [NSNumber numberWithUnsignedLongLong:[dataTask taskIdentifier]];
  BenoniHTTPTaskContextWrap *contextWrap = contextMap_[key];
  HTTPTaskContext *context = [contextWrap context];

  NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;
  if ([httpResponse respondsToSelector:@selector(allHeaderFields)]) {
    auto &headers = context->headers;
    NSDictionary *allHeaderFields = [httpResponse allHeaderFields];
    for (NSString *key in allHeaderFields) {
      headers[[key UTF8String]] =
          [[allHeaderFields objectForKey:key] UTF8String];
    }
  }

  context->status = [httpResponse statusCode];

  context->encoding = NSUTF8StringEncoding;
  NSString *encodingName = [httpResponse textEncodingName];
  if (encodingName) {
    NSStringEncoding encoding = CFStringConvertEncodingToNSStringEncoding(
        CFStringConvertIANACharSetNameToEncoding((CFStringRef)encodingName));

    if (encoding != kCFStringEncodingInvalidId) {
      context->encoding = encoding;
    }
  }

  completionHandler(NSURLSessionResponseAllow);
}

- (void)URLSession:(NSURLSession *)session
          dataTask:(NSURLSessionDataTask *)dataTask
    didReceiveData:(NSData *)data {
  NSNumber *key =
      [NSNumber numberWithUnsignedLongLong:[dataTask taskIdentifier]];
  BenoniHTTPTaskContextWrap *contextWrap = contextMap_[key];
  HTTPTaskContext *context = [contextWrap context];

  if (context->data == nil) {
    context->data = [[NSMutableData data] retain];
  }

  [context->data appendData:data];
}

#pragma mark - NSURLSessionTaskDelegate
- (void)URLSession:(NSURLSession *)session
                    task:(NSURLSessionTask *)task
    didCompleteWithError:(NSError *)error {
  NSNumber *key = [NSNumber numberWithUnsignedLongLong:[task taskIdentifier]];
  BenoniHTTPTaskContextWrap *contextWrap = contextMap_[key];
  HTTPTaskContext *context = [contextWrap context];
  auto callback = std::move(context->callback);

  if (error) {
    std::string error_string([[error localizedDescription] UTF8String]);
    [contextWrap dealloc];
    callback(std::move(error_string));
    return;
  }

  NSMutableString *responseString = [NSMutableString string];
  __block BOOL success = YES;
  [context->data enumerateByteRangesUsingBlock:^(
                     const void *bytes, NSRange byteRange, BOOL *stop) {
    NSString *chunk = [[NSString alloc] initWithBytes:bytes
                                               length:byteRange.length
                                             encoding:context->encoding];
    if (chunk == nil) {
      *stop = YES;
      success = NO;
      return;
    }
    [responseString appendString:chunk];
  }];
  [context->data release];

  if (success == NO) {
    std::string error_string("response body has invalid encoding");
    [contextWrap dealloc];
    callback(std::move(error_string));
    return;
  }

  std::string body{[responseString UTF8String]};

  benoni::Response response{
      .body = std::move(body),
      .status = context->status,
      .headers = std::move(context->headers),
  };
  [contextWrap dealloc];
  callback(response);
}
@end

namespace benoni {

auto request(const std::string &url, RequestOptions options,
             std::function<void(std::variant<std::string, Response>)> callback)
    -> void {
  BenoniHTTPSessionDelegate *delegate =
      [[BenoniHTTPSessionDelegate alloc] init];
  NSURLSessionConfiguration *configuration =
      [NSURLSessionConfiguration defaultSessionConfiguration];
  configuration.HTTPAdditionalHeaders = @{@"User-Agent" : @"Benoni/1.0"};
  NSURLSession *session = [NSURLSession sessionWithConfiguration:configuration
                                                        delegate:delegate
                                                   delegateQueue:nil];

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

    BENONI_HTTP_METHODS(V)
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

  NSURLSessionDataTask *data_task = [session dataTaskWithRequest:request];
  NSMutableDictionary<NSNumber *, BenoniHTTPTaskContextWrap *> *contextMap =
      [delegate contextMap];
  auto *context{new HTTPTaskContext{.callback = std::move(callback)}};
  BenoniHTTPTaskContextWrap *contextWrap =
      [[BenoniHTTPTaskContextWrap alloc] initWithContext:context];
  [contextMap
      setObject:contextWrap
         forKey:[NSNumber
                    numberWithUnsignedLongLong:[data_task taskIdentifier]]];
  [data_task resume];
}

} // namespace benoni

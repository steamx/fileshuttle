//
//  CURLHandle.h
//
//  Created by Dan Wood <dwood@karelia.com> on Fri Jun 22 2001.
//  This is in the public domain, but please report any improvements back to the author.
//

#import <Foundation/Foundation.h>
#import <curl/curl.h>

@protocol CURLHandleDelegate;


extern NSString * const CURLcodeErrorDomain;
extern NSString * const CURLMcodeErrorDomain;
extern NSString * const CURLSHcodeErrorDomain;

@interface CURLHandle : NSObject
{
	CURL			*mCURL;					/*" Pointer to the actual CURL object that does all the hard work "*/
	char 			mErrorBuffer[CURL_ERROR_SIZE];	/*" Buffer to hold string generated by CURL; this is then converted to an NSString. "*/

	NSMutableData   *_headerBuffer;			/*" The buffer that is filled with data from the header as the download progresses; it's appended to one line at a time. "*/

	NSMutableDictionary	*mStringOptions;	/*" Dictionary of keys(ints) & values (NSStrings) for performing curl_easy_setopt.  We store the options in a dictionary and then invoke #curl_easy_setopt on each option right before the #curl_easy_perform so that we can retain their memory until it is needed."*/

	NSDictionary	*mProxies;	/*" Dictionary of proxy information; it's released when the handle is deallocated since it's needed for the transfer."*/

	// Backgrounding support
    BOOL    _executing;     // debugging
	BOOL    _cancelled;		/*" A flag that is set by the foreground thread and read by the background thread; it's an indicator that the user has cancelled. "*/
    
    NSInputStream   *_uploadStream;

	id <CURLHandleDelegate> _delegate;
}

//  Loading respects as many of NSURLRequest's built-in features as possible, including:
//  
//    * An HTTP method of @"HEAD" turns on the CURLOPT_NOBODY option, regardless of protocol (e.g. handy for FTP)
//    * Similarly, @"PUT" turns on the CURLOPT_UPLOAD option (again handy for FTP uploads)
//  
//    * Supply -HTTPBody or -HTTPBodyStream to switch Curl into uploading mode, regardless of protocol
//  
//    * Custom Range: HTTP headers are specially handled to set the CURLOPT_RANGE option, regardless of protocol in use
//      (you should still construct the header as though it were HTTP, e.g. bytes=500-999)
//  
//    * Custom Accept-Encoding: HTTP headers are specially handled to set the CURLOPT_ENCODING option
//  
// Where possible errors are in NSURLErrorDomain or NSCocoaErrorDomain. There will generally be a CURLcodeErrorDomain error present; either directly, or as an underlying error (KSError <https://github.com/karelia/KSError> is handy for querying underlying errors)
// The key CURLINFO_RESPONSE_CODE (as an NSNumber) will be filled out with HTTP/FTP status code if appropriate
// At present all errors include NSURLErrorFailingURLErrorKey and NSURLErrorFailingURLStringErrorKey if applicable even though the docs say "This key is only present in the NSURLErrorDomain". Should we respect that?
- (BOOL)loadRequest:(NSURLRequest *)request error:(NSError **)error;

// Can be called from any thread. Causes -loadRequest:error: as soon as it can
- (void)cancel;

- (NSString *)initialFTPPath;    // CURLINFO_FTP_ENTRY_PATH

@property(nonatomic, assign) id <CURLHandleDelegate> delegate;

+ (NSString *)curlVersion;

/*" Old API "*/

- (CURL *) curl;
- (void) setString:(NSString *)inString forKey:(CURLoption) inCurlOption;
+ (void) setProxyUserIDAndPassword:(NSString *)inString;
+ (void) setAllowsProxy:(BOOL) inBool;

@end


@protocol CURLHandleDelegate <NSObject>

- (void)handle:(CURLHandle *)handle didReceiveData:(NSData *)data;
@optional
- (void)handle:(CURLHandle *)handle didReceiveResponse:(NSURLResponse *)response;

// When sending data to the server, this reports just before it goes out on the wire. Reports a length of 0 when the end of the data is written so you can get a nice heads up that an upload is about to complete
- (void)handle:(CURLHandle *)handle willSendBodyDataOfLength:(NSUInteger)bytesWritten;

- (void)handle:(CURLHandle *)handle didReceiveDebugInformation:(NSString *)string ofType:(curl_infotype)type;

@end


#pragma mark -


@interface NSURLRequest (CURLOptionsFTP)

// CURLUSESSL_NONE, CURLUSESSL_TRY, CURLUSESSL_CONTROL, or CURLUSESSL_ALL
@property(nonatomic, readonly) curl_usessl curl_desiredSSLLevel;

@property(nonatomic, readonly) BOOL curl_shouldVerifySSLCertificate;    // CURLOPT_SSL_VERIFYPEER

// An array of strings. Executed in turn once the main request is done
@property(nonatomic, copy, readonly) NSArray *curl_postTransferCommands;

// A value greater than 0 will cause Curl to create missing directories. I'm pretty certain this only applies when uploading
// Default is 0
// See CURLOPT_FTP_CREATE_MISSING_DIRS docs for full details
@property(nonatomic, readonly) NSUInteger curl_createIntermediateDirectories;

@end

@interface NSMutableURLRequest (CURLOptionsFTP)

- (void)curl_setDesiredSSLLevel:(curl_usessl)level;
- (void)curl_setShouldVerifySSLCertificate:(BOOL)verify;
- (void)curl_setPostTransferCommands:(NSArray *)postTransferCommands;
- (void)curl_setCreateIntermediateDirectories:(NSUInteger)createIntermediateDirectories;

@end


#pragma mark -


@interface NSDictionary ( CurlHTTPExtensions )

- (NSString *) formatForHTTP;
- (NSString *) formatForHTTPUsingEncoding:(NSStringEncoding)inEncoding;
- (NSString *) formatForHTTPUsingEncoding:(NSStringEncoding)inEncoding ordering:(NSArray *)inOrdering;

@end



//
//  iNDSRomDownloadManager.m
//  iNDS
//
//  Created by Will Cobb on 11/17/15.
//  Copyright © 2015 iNDS. All rights reserved.
//

#import "iNDSRomDownloadManager.h"
#import "AppDelegate.h"
#import "ZAActivityBar.h"

@implementation iNDSRomDownload

- (id)initWithRequest:(NSURLRequest *)request delegate:(iNDSRomDownloadManager *)delegate
{
    if (self = [super init]) {
        _connection = [[NSURLConnection alloc] initWithRequest:request delegate:self startImmediately:YES];
        _delegate = delegate;
        
        savePath = [NSString stringWithFormat:@"file://%@%@", NSTemporaryDirectory(), [_connection.originalRequest.URL lastPathComponent]];
        escapedPath  = [NSURL URLWithString:[savePath stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
        
        [[NSFileManager defaultManager] createFileAtPath:escapedPath.path contents:nil attributes:nil];
        fileHandle = [NSFileHandle fileHandleForWritingAtPath:escapedPath.path];
        if (!fileHandle) {
            NSLog(@"Error opening handle");
        }
    }
    return self;
}

- (float)progress
{
    return progress;
}

- (NSString *)name
{
    return _connection.originalRequest.URL.path.lastPathComponent;
}

- (void)stop
{
    [_connection cancel];
    _connection = nil;
}

- (void) connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response {
    [UIApplication sharedApplication].networkActivityIndicatorVisible = YES;
    expectedBytes = [response expectedContentLength];
}

- (void) connection:(NSURLConnection *)connection didReceiveData:(NSData *)data {
    [fileHandle seekToEndOfFile];
    [fileHandle writeData:data];
    progress = [fileHandle offsetInFile] / (float)expectedBytes;
    if (self.progressLabel) {
        int roundedProgress = (int)(progress * 100);
        self.progressLabel.text = [NSString stringWithFormat:@"Downloading: %i%%", roundedProgress];
    }
}

- (void) connection:(NSURLConnection *)connection didFailWithError:(NSError *)error {
    [UIApplication sharedApplication].networkActivityIndicatorVisible = NO;
    [ZAActivityBar showErrorWithStatus:@"Failed to download ROM, Connection Error" duration:5];
    [_delegate removeDownload:self];
}

- (NSCachedURLResponse *) connection:(NSURLConnection *)connection willCacheResponse:    (NSCachedURLResponse *)cachedResponse {
    return nil;
}

- (void) connectionDidFinishLoading:(NSURLConnection *)connection {
    //NSFileManager *fm = [NSFileManager defaultManager];
    [UIApplication sharedApplication].networkActivityIndicatorVisible = NO;
    
    dispatch_async(dispatch_get_main_queue(), ^{
        self.progressLabel.text = [NSString stringWithFormat:@"Opening"];
    });
    NSError * error;
    
    [fileHandle closeFile];
    
    error = nil;
    [ZAActivityBar showWithStatus:@"Download Complete, Opening"];
    [AppDelegate.sharedInstance application:nil openURL:escapedPath sourceApplication:nil annotation:nil];
    [_delegate removeDownload:self];
}
@end


@implementation iNDSRomDownloadManager

+ (id)sharedManager {
    static iNDSRomDownloadManager * sharedMyManager = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedMyManager = [[self alloc] init];
    });
    return sharedMyManager;
    
}

- (id)init {
    if (self = [super init]) {
        _activeDownloads = [NSMutableArray new];
    }
    return self;
}

- (void)addRequest:(NSURLRequest *)request
{
    iNDSRomDownload *newDownload = [[iNDSRomDownload alloc] initWithRequest:request delegate:self];
    [_activeDownloads addObject:newDownload];
    [UIApplication sharedApplication].idleTimerDisabled = YES;
    
}

- (void)removeDownload:(iNDSRomDownload *)download
{
    [_activeDownloads removeObject:download];
    [download stop];
    if (_activeDownloads.count == 0) {
        [UIApplication sharedApplication].idleTimerDisabled = NO;
    }
}

@end

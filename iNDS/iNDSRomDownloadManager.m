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
        fileData = [NSMutableData dataWithLength:0];
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
    [fileData appendData:data];
    progress = (float)[fileData length] / (float)expectedBytes;
    if (self.progressLabel) {
        self.progressLabel.text = [NSString stringWithFormat:@"Downloading: %i%%", (int)(progress * 100)];
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
    NSError * error;
    
    NSString * savePath = [NSString stringWithFormat:@"file://%@%@", NSTemporaryDirectory(), [connection.originalRequest.URL lastPathComponent]];
    NSURL * escapedPath  = [NSURL URLWithString:[savePath stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
    
    
    error = nil;
    if (self.progressLabel) {
        self.progressLabel.text = @"Opening";
    }
    [fileData writeToFile:escapedPath.path options:NSDataWritingAtomic error:&error];
    if (error) {
        NSLog(@"Error Downloading File: %@", error);
        [ZAActivityBar showErrorWithStatus:@"Failed to download ROM, Error Saving File" duration:5];
    } else if (![[NSFileManager defaultManager] fileExistsAtPath:escapedPath.path]) {
        NSLog(@"Error: unable to save file");
    } else {
        [ZAActivityBar showWithStatus:@"Download Complete, Extracting"];
        [AppDelegate.sharedInstance application:nil openURL:escapedPath sourceApplication:nil annotation:nil];
    }
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
    
}

- (void)removeDownload:(iNDSRomDownload *)download
{
    [_activeDownloads removeObject:download];
    [download stop];
}

@end

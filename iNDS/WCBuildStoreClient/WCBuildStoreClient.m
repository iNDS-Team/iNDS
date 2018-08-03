//
//  WCBuildStoreClient.m
//  iNDS
//
//  Created by Will Cobb on 5/5/16.
//  Copyright © 2016 iNDS. All rights reserved.
//
// http://www.builds.io/resign appId: 90
// http://www.builds.io/api/job/<result>?_=<1462498491183>

#import "WCBuildStoreClient.h"

@interface WCBuildStoreClient () {
    NSURLConnection *resignConnection;
    NSURLConnection *jobConnection;
}

@end

@implementation WCBuildStoreClient

+ (WCBuildStoreClient *)sharedInstance
{
    static dispatch_once_t p = 0;
    
    __strong static id _sharedInstance = nil;
    
    dispatch_once(&p, ^{
        _sharedInstance = [[self alloc] init];
    });
    
    return _sharedInstance;
}

- (id)init
{
    if (self = [super init]) {
    }
    return self;
}

#pragma mark Networking

- (NSString *)getCSRF
{
    NSData *CSRFData = [[NSData alloc] initWithContentsOfURL:[NSURL URLWithString:@"http://builds.io/apps/inds/"]];
    NSString *searchedString = [[NSString alloc] initWithData:CSRFData encoding:NSUTF8StringEncoding];
    NSRange   searchedRange = NSMakeRange(0, [searchedString length]);
    NSString *pattern = @"'X-CSRFToken': \"(.*?)\"";
    NSError  *error = nil;
    
    NSRegularExpression* regex = [NSRegularExpression regularExpressionWithPattern:pattern options:0 error:&error];
    NSTextCheckingResult *match = [regex firstMatchInString:searchedString options:0 range:searchedRange];
    NSRange matchRange = [match rangeAtIndex:1];
    NSString *result = [searchedString substringWithRange:matchRange];
    NSLog(@"Matched: %@", result);
    return result;
}

- (void)checkForUpdates
{
    if (self.linked) {
        
        NSString *CSRF = [self getCSRF];
        if (CSRF) {
            NSString *post = @"appId=90"; //Needs to ba changed per app
            NSData *postData = [post dataUsingEncoding:NSASCIIStringEncoding allowLossyConversion:YES];
            NSString *postLength = [NSString stringWithFormat:@"%ld", [postData length]];
            NSMutableURLRequest *request = [[NSMutableURLRequest alloc] init];
            [request setURL:[NSURL URLWithString:@"http://builds.io/api/resign/"]];
            [request setHTTPMethod:@"POST"];
            [request setValue:postLength forHTTPHeaderField:@"Content-Length"];
            [request setValue:@"application/x-www-form-urlencoded; charset=UTF-8" forHTTPHeaderField:@"Content-Type"];
            [request setValue:@"http://builds.io" forHTTPHeaderField:@"Origin"];
            [request setValue:@"http://builds.io/apps/inds/" forHTTPHeaderField:@"Referer"]; //Needs to be changed oer app
            [request setValue:@"XMLHttpRequest" forHTTPHeaderField:@"X-Requested-With"];
            [request setValue:CSRF forHTTPHeaderField:@"X-CSRFToken"];
            [request setHTTPBody:postData];
            resignConnection = [[NSURLConnection alloc] initWithRequest:request delegate:self];
            if(resignConnection) {
                NSLog(@"Connection Successful");
            } else {
                NSLog(@"Connection could not be made");
            }
        } else {
            NSLog(@"Error, unable to get CSRF");
        }
    }
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData*)data
{
    NSLog(@"Got Data: %@", [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding]);
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    NSLog(@"Error getting update: %@", error);
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
    NSLog(@"Connection Finished");
}

#pragma mark Methods

- (void)linkFromController:(UIViewController *)controller
{
    WCBuildStoreAuthenticateViewController *authenticationController = [[WCBuildStoreAuthenticateViewController alloc] init];
    
    UINavigationController *nav = [[UINavigationController alloc] initWithRootViewController:authenticationController];
    
    [controller presentViewController:nav animated:YES completion:nil];
}

- (void)unlink
{
    [self clearCookies];
}

- (BOOL)getLinked
{
    NSHTTPCookieStorage *cookieJar = [NSHTTPCookieStorage sharedHTTPCookieStorage];
    for (NSHTTPCookie *cookie in [cookieJar cookies]) {
        NSLog(@"%@ %@", cookie.domain, cookie.name);
        if ([cookie.domain isEqualToString:@".builds.io"] && [cookie.name isEqualToString:@"_ym_uid"] && [cookie.expiresDate timeIntervalSinceNow] > 0.0) {
            return YES;
        }
    }
    return NO;
}

- (void)clearCookies
{
    NSLog(@"Clearing cookies");
    NSHTTPCookieStorage *cookieJar = [NSHTTPCookieStorage sharedHTTPCookieStorage];
    for (NSHTTPCookie *cookie in [cookieJar cookies]) {
        if ([cookie.domain isEqualToString:@"builds.io"] || [cookie.domain isEqualToString:@".builds.io"]) {
            [cookieJar deleteCookie:cookie];
        }
    }
    [[NSUserDefaults standardUserDefaults] synchronize];
}


@end









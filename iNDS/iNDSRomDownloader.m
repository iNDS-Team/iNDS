//
//  iNDSRomDownloader.m
//  iNDS
//
//  Created by Will Cobb on 11/4/15.
//  Copyright © 2015 iNDS. All rights reserved.
//
#import "AppDelegate.h"
#import "iNDSRomDownloader.h"
#import "ZAActivityBar.h"
#import "iNDSRomDownloadManager.h"
@interface iNDSRomDownloader()
{
    IBOutlet UIWebView * webView;
    
    IBOutlet UITextField * urlField;
    NSString * lastURL;
    
    long long expectedBytes;
    NSMutableData *fileData;
    
    float lastProgress;
}
@end

@implementation iNDSRomDownloader

#pragma mark - Downloading Games

- (void)viewDidLoad
{
    [super viewDidLoad];
}

-(void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    //[[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault];
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"http://www.emuparadise.me/Nintendo_DS_ROMs/32"]]];
    //[webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"http://10.20.51.95:8000"]]];
    webView.scrollView.delegate = self;
    urlField.text = @"http://www.emuparadise.me/Nintendo_DS_ROMs/32";
    lastURL = urlField.text;
    [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault];
}

-(void)webViewDidStartLoad:(UIWebView *)WebView
{
    NSString * newURL = webView.request.URL.absoluteString;
    if (![newURL isEqualToString:lastURL]) {
        urlField.text = newURL;
        lastURL = newURL;
    }
}

-(void)webViewDidFinishLoad:(UIWebView *)WebView
{
    NSString * newURL = webView.request.URL.absoluteString;
    if (![newURL isEqualToString:lastURL]) {
        urlField.text = newURL;
        lastURL = newURL;
    }
}

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType {
    NSString *fileExtension = request.URL.pathExtension.lowercaseString;
    if ([fileExtension isEqualToString:@"ds"] || [fileExtension isEqualToString:@"rar"] || [fileExtension isEqualToString:@"zip"] || [fileExtension isEqualToString:@"7z"])
    {
        NSLog(@"Downloading %@", request.URL);
        lastProgress = 0.0;
        [ZAActivityBar showSuccessWithStatus:[NSString stringWithFormat:@"Downloading started: %@", request.URL.lastPathComponent] duration:5];
        NSURL *requestedURL = request.URL;
        
        
        NSURLRequest *theRequest = [NSURLRequest requestWithURL:requestedURL cachePolicy:NSURLRequestReloadIgnoringLocalCacheData timeoutInterval:60];
        [[iNDSRomDownloadManager sharedManager] addRequest:theRequest];
        //[self dismissViewControllerAnimated:YES completion:nil];
        
        return NO;
    } else {
        NSLog(@"Ignore: %@", request.URL);
        NSLog(@"--%@", fileExtension);
    }
    return ![urlField isFirstResponder]; //Prevent leaving page while editing
}




-(IBAction)go:(id)sender
{
    NSLog(@"GO");
    NSString * location = urlField.text;
    if (![location hasPrefix:@"http://"] || ![location hasPrefix:@"https://"]) {
        location = [NSString stringWithFormat:@"http://%@", location];
    }
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:location]]];
}
- (IBAction)hide:(id)sender
{
    [self dismissViewControllerAnimated:YES completion:nil];
}

- (IBAction)back:(id)sender
{
    [webView goBack];
}
    
- (IBAction)forward:(id)sender
{
    [webView goForward];
}

-(void)scrollViewDidScroll:(UIScrollView *)scrollView
{
    //[self.view endEditing:YES];
}
@end

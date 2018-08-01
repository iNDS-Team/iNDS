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
#import "SCLAlertView.h"
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
    urlField.translatesAutoresizingMaskIntoConstraints = false;
    CGRect urlFieldRect = urlField.frame;
    urlFieldRect.size.width = self.view.frame.size.width - 80;
    urlField.frame = urlFieldRect;
}

-(void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    //[[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault];
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    urlField.text = @"http://www.google.com/search?q=Nintendo+DS+Roms";
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"http://www.google.com/search?q=Nintendo+DS+Roms"]]];
    //[webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"http://10.0.1.20:8000"]]];
    webView.scrollView.delegate = self;
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
    if ([fileExtension isEqualToString:@"nds"] || [fileExtension isEqualToString:@"rar"] || [fileExtension isEqualToString:@"zip"] || [fileExtension isEqualToString:@"7z"]) {
        NSLog(@"Downloading %@", request.URL);
        lastProgress = 0.0;
        [ZAActivityBar showSuccessWithStatus:[NSString stringWithFormat:@"Downloading started: %@", request.URL.lastPathComponent] duration:5];
        NSURL *requestedURL = request.URL;
        
        
        NSURLRequest *theRequest = [NSURLRequest requestWithURL:requestedURL cachePolicy:NSURLRequestReloadIgnoringLocalCacheData timeoutInterval:60];
        [[iNDSRomDownloadManager sharedManager] addRequest:theRequest];
        //[self dismissViewControllerAnimated:YES completion:nil];
        
        return NO;
    } else if ([fileExtension isEqualToString:@"rar"]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            SCLAlertView * alertView = [[SCLAlertView alloc] init];
            alertView.shouldDismissOnTapOutside = YES;
            [alertView showError:self title:@"Error!" subTitle:@"Rar support has been disabled for iNDS. Support will return in a future update." closeButtonTitle:@"Okay" duration:0.0];
        });
    } else {
        //NSLog(@"Ignore: %@", request.URL);
    }
    return ![urlField isFirstResponder]; //Prevent leaving page while editing
}




-(IBAction)go:(id)sender
{
    [self.view endEditing:YES];
    NSString * location = urlField.text;
    if (!([location hasPrefix:@"http://"] || [location hasPrefix:@"https://"])) {
        location = [NSString stringWithFormat:@"http://%@", location];
    }
    NSLog(@"GO to %@", location);
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

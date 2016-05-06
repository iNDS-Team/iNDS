//
//  WCBuildStoreAuthenticateViewController.m
//  iNDS
//
//  Created by Will Cobb on 5/5/16.
//  Copyright © 2016 iNDS. All rights reserved.
//

#import "WCBuildStoreAuthenticateViewController.h"

@interface WCBuildStoreAuthenticateViewController () {
    UIWebView   *webView;
}

@end

@implementation WCBuildStoreAuthenticateViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    webView = [[UIWebView alloc] initWithFrame:self.view.bounds];
    webView.delegate = self;
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"http://www.builds.io/accounts/login/"]]];
    [self.view addSubview:webView];
    
    UIBarButtonItem *cancel = [[UIBarButtonItem alloc] initWithTitle:@"Cancel" style:UIBarButtonItemStylePlain target:self action:@selector(cancel)];
    self.navigationItem.leftBarButtonItem = cancel;
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    //[[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault];
}

- (void)cancel
{
    [self dismissViewControllerAnimated:YES completion:nil];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


#pragma mark UIWebView Delegate

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    if ([request.URL.path isEqualToString:@"/device"]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            //[self dismissViewControllerAnimated:YES completion:nil];
        });
    }
    NSData *data = request.HTTPBody;
    NSLog(@"RU: %@", request.URL.path);
    NSLog(@"Data: %@", data);
    //[self dumpCookies:nil];
    return YES;//[request.URL.path isEqualToString:@"/accounts/login"];
}



@end

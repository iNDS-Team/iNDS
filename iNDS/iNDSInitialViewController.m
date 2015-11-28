//
//  iNDSInitialViewController.m
//  iNDS
//
//  Created by Will Cobb on 11/27/15.
//  Copyright © 2015 iNDS. All rights reserved.
//

#import "iNDSInitialViewController.h"

@interface iNDSInitialViewController ()

@end

@implementation iNDSInitialViewController

- (void)viewDidLoad {
    [super viewDidLoad];
}

-(void) viewDidAppear:(BOOL)animated
{
    [self performSegueWithIdentifier:@"ToRoot" sender:self];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


#pragma mark - Navigation

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    self.rootView = segue.destinationViewController;
}


@end

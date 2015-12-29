//
//  iNDSCheatsTableViewController.m
//  iNDS
//
//  Created by Will Cobb on 12/27/15.
//  Copyright © 2015 iNDS. All rights reserved.
//

#import "iNDSCheatsTableViewController.h"
#import "AppDelegate.h"
#import "iNDSEmulatorViewController.h"
#import "SCLAlertView.h"
#import <UnrarKit/UnrarKit.h>
#include <CommonCrypto/CommonDigest.h>

#include "emu.h"
#include "cheatSystem.h"

@interface iNDSCheatsTableViewController () {
    NSString * currentGameId;
    iNDSEmulatorViewController * emulationController;
    NSString *cheatsArchivePath;
    NSData *cheatData;
    BOOL cheatsLoaded;
}

@end

@implementation iNDSCheatsTableViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    emulationController = AppDelegate.sharedInstance.currentEmulatorViewController;
    cheatsArchivePath = [[NSBundle mainBundle] pathForResource:@"cheats" ofType:@"rar"];
    currentGameId = emulationController.game.rawTitle;
    NSString *cheatSavePath = [NSString stringWithUTF8String:(char *)cheats->filename];
    NSLog(@"Sp: %@", cheatSavePath);
    
    //Eventually we might want to create our own NSInput stream to parse XML on the fly to reduce memory overhead. This will work for now though
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        NSError *error = nil;
        URKArchive *archive = [[URKArchive alloc] initWithPath:cheatsArchivePath error:&error];
        cheatData = [archive extractDataFromFile:@"usrcheat1.xml" progress:nil error:&error];
    });
}


- (void) loadCheats
{
    
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark - Table view data source

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell * cell;
    if (indexPath.section == 0) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"Cheat"];
        //build user cheat cell
    } else {
        if (!cheatsLoaded) {
            cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"Loading"];
            cell.textLabel.text = @"Loading Cheats";
            UIActivityIndicatorView * activity = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
            activity.frame = CGRectMake(self.view.frame.size.width - 50, 0, 44, 44);
            [activity startAnimating];
            [cell addSubview:activity];
        } else {
            cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"Cheat"];
            //build db cheat cell
        }
    }
    return cell;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 2;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (section == 0) return 0; //num user cheats
    if (section == 1) {
        if (cheatsLoaded) {
            return 0; //num db cheats
        } else {
            return 1;
        }
    }
    return 0;
}

@end

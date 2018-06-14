//
//  iNDSBugReportTableViewController.m
//  inds
//
//  Created by Will Cobb on 3/30/16.
//  Copyright © 2016 iNDS. All rights reserved.
//

#import "iNDSBugReportTableViewController.h"
#import "iNDSBugReportRomTableViewController.h"
#import "iNDSBugReportSaveStateTableViewController.h"

#import "iNDSGame.h"
#import "UIImage+Utilities.h"
#import "UIDevice-Hardware.h"
#import "AppDelegate.h"
#import <sys/utsname.h>

#import "MBProgressHUD.h"
#import "SCLAlertView.h"

#import "AFHTTPSessionManager.h"

@interface iNDSBugReportTableViewController () <iNDSBugReportSelectionDelegate, UIActionSheetDelegate, UIImagePickerControllerDelegate> {
    iNDSGame    *selectedGame;
    NSString    *saveStatePath;
    
    //<=============== Image
    IBOutlet UILabel        *addImage;
    IBOutlet UIImageView    *bannerImage;
    NSMutableDictionary     *parameters;
    BOOL bannerSet;
    BOOL submitSuccess;
}
@end

@implementation iNDSBugReportTableViewController

- (void)configUI
{
    // Add gesture for hiding keyboard
    UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tapGestureAction:)];
    [self.tableView addGestureRecognizer:tapGesture];
    // Hide tableView footerView
    self.tableView.tableFooterView = [UIView new];
    self.tableView.sectionFooterHeight = CGFLOAT_MIN;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self configUI];
    submit.backgroundColor = self.view.tintColor;
    submit.layer.cornerRadius = 4;
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    [self.tableView beginUpdates];
    [self.tableView endUpdates];
    UITableViewCell *selectImageCell = [self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:2 inSection:1]];
    selectImageCell.layoutMargins = UIEdgeInsetsZero;
    selectImageCell.preservesSuperviewLayoutMargins = NO;
    if (bannerSet)
        selectImageCell.separatorInset = UIEdgeInsetsMake(0, 0, 0, self.tableView.frame.size.width);
    else
        selectImageCell.separatorInset = UIEdgeInsetsMake(0, 0, 0, 0);
}

- (void)tapGestureAction:(UITapGestureRecognizer *)tapGesture
{
    CGPoint hintPoint = [tapGesture locationInView:self.tableView];
    NSIndexPath *hintIndexPath = [self.tableView indexPathForRowAtPoint:hintPoint];
    if (hintIndexPath)
    {
        [self tableView:self.tableView didSelectRowAtIndexPath:hintIndexPath];
    }
    else
    {
        if ([description isFirstResponder])
        {
            [description resignFirstResponder];
        }
    }
}

- (IBAction)cancel:(id)sender
{
    [self dismissViewControllerAnimated:YES completion:nil];
}

- (IBAction)submit:(id)sender
{
    [self.view endEditing:YES];
    if (description.text.length < 1) {
        [AppDelegate.sharedInstance showError:@"Please enter a description"];
        return;
    }
    NSDictionary *infoDictionary = [[NSBundle mainBundle] infoDictionary];
    NSString *deviceName = [self rawDeviceName];
    parameters = [NSMutableDictionary
                  dictionaryWithDictionary: @{@"description": description.text,
                                              @"device": deviceName,
                                              @"isSystem": @([AppDelegate.sharedInstance isSystemApplication]),
                                              @"major": [infoDictionary objectForKey:@"CFBundleShortVersionString"],
                                              @"minor": [infoDictionary objectForKey:@"CFBundleVersion"]
                                              }];
    
    if (selectedGame){
        parameters[@"game"] = selectedGame.title;
        parameters[@"gameCode"] = selectedGame.rawTitle;
    }
    if (saveStatePath) {
        NSData *data = [[NSData alloc] initWithContentsOfFile:saveStatePath];
        parameters[@"save"] = [data base64EncodedStringWithOptions:0];
    }
    if (bannerSet) {
        NSData *bannerData = UIImageJPEGRepresentation(bannerImage.image, 0.5);
        parameters[@"image"] = [bannerData base64EncodedStringWithOptions:0];
    }
    __block MBProgressHUD *hud;
    dispatch_async(dispatch_get_main_queue(), ^{
        hud = [MBProgressHUD showHUDAddedTo:self.view animated:YES];
        hud.mode = MBProgressHUDModeIndeterminate;
        hud.label.text = @"Submitting";
    });
    AFHTTPSessionManager *manager = [[AFHTTPSessionManager alloc]initWithSessionConfiguration:[NSURLSessionConfiguration defaultSessionConfiguration]];
    manager.requestSerializer = [AFJSONRequestSerializer serializer];
    [manager.requestSerializer setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
    manager.responseSerializer.acceptableContentTypes = [NSSet setWithObject:@"text/html"];
    
    [manager POST:kBugUrl parameters:parameters progress:^(NSProgress * _Nonnull uploadProgress) {
        hud.progress = uploadProgress.fractionCompleted;
    } success:^(NSURLSessionDataTask * _Nonnull task, id  _Nullable responseObject) {
        [self sendSuccess];
        NSLog(@"Submit Success");
    } failure:^(NSURLSessionDataTask * _Nullable task, NSError * _Nonnull error) {
        NSLog(@"error: %@", error);
        [self sendError];
    }];
    submit.enabled = NO;
}

- (void)sendError
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [MBProgressHUD hideHUDForView:self.view animated:YES];
        SCLAlertView * alertView = [[SCLAlertView alloc] init];
        alertView.shouldDismissOnTapOutside = YES;
        [alertView showError:self title:@"Error!" subTitle:@"Unable to send bug report! Have no fear, your bug report has been saved and will be sent again at a later time." closeButtonTitle:@"Okay" duration:0.0];
        NSString *savePath = [AppDelegate.sharedInstance.documentsPath stringByAppendingPathComponent:@"bug.json"];
        [parameters writeToFile:savePath atomically:YES];
        [alertView alertIsDismissed:^{
            [self dismissViewControllerAnimated:YES completion:nil];
        }];
    });
    submit.enabled = YES;
}

- (void)sendSuccess
{
    submitSuccess = YES;
    dispatch_async(dispatch_get_main_queue(), ^{
        [MBProgressHUD hideHUDForView:self.view animated:YES];
        SCLAlertView * alertView = [[SCLAlertView alloc] init];
        alertView.shouldDismissOnTapOutside = YES;
        [alertView showSuccess:self title:@"Sent!" subTitle:@"Your bug report has been sent and will be attended to soon." closeButtonTitle:@"Okay" duration:0.0];
        NSString *savePath = [AppDelegate.sharedInstance.documentsPath stringByAppendingPathComponent:@"bug.json"];
        if ([[NSFileManager defaultManager] fileExistsAtPath:savePath]) {
            [[NSFileManager defaultManager] removeItemAtPath:savePath error:nil];
        }
        [alertView alertIsDismissed:^{
            [self dismissViewControllerAnimated:YES completion:nil];
        }];
        
    });
    submit.enabled = YES;
}


- (void)setGame:(iNDSGame *)game
{
    if (game != selectedGame) {
        selectedGame = game;
        saveStatePath = nil;
        
        if (game.gameTitle) {
            // use title from ROM
            NSArray *titleLines = [game.gameTitle componentsSeparatedByString:@"\n"];
            gameName.text = titleLines[0];
        } else {
            // use filename
            gameName.text = game.title;
        }
    }
}

- (void)setSaveStatePath:(NSString *)path
{
    saveStatePath = path;
    NSString * saveStateTitle = [selectedGame nameOfSaveStateAtPath:path];
    if ([saveStateTitle isEqualToString:@"pause"]) {
        saveStateTitle = @"Auto Save";
    }
    saveStateName.text = saveStateTitle;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (indexPath.section == 1) {
        if (indexPath.row == 0) {
            return 50;
        } else if (indexPath.row == 1) {
            return selectedGame ? 50 : 0;
        } else if (indexPath.row == 2) {
            return 50;
        } else if (indexPath.row == 3) {
            if (bannerSet) {
                return 210;
            } else {
                return 0;
            }
        }
    } else {
        return [super tableView:tableView heightForRowAtIndexPath:indexPath];
    }
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(nonnull NSIndexPath *)indexPath
{
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    if (indexPath.section == 1) {
        if (indexPath.row == 0) {
            iNDSBugReportRomTableViewController * romList = [[iNDSBugReportRomTableViewController alloc] init];
            romList.delegate = self;
            [self.navigationController pushViewController:romList animated:YES];
        } else if (indexPath.row == 1) {
            iNDSBugReportSaveStateTableViewController * gameInfo = [[iNDSBugReportSaveStateTableViewController alloc] init];
            gameInfo.game = selectedGame;
            gameInfo.delegate = self;
            [self.navigationController pushViewController:gameInfo animated:YES];
        } else if (indexPath.row == 2) {
            [self selectBanner:nil];
        }
    }
}

#pragma mark -
#pragma mark Image

- (IBAction)selectBanner:(id)sender
{
    [self.view endEditing:YES];
    [self showImagePickerForSourceType:UIImagePickerControllerSourceTypePhotoLibrary];
}


- (void)showImagePickerForSourceType:(UIImagePickerControllerSourceType)sourceType
{
    UIImagePickerController *imagePickerController = [[UIImagePickerController alloc] init];
    imagePickerController.modalPresentationStyle = UIModalPresentationCurrentContext;
    imagePickerController.sourceType = sourceType;
    imagePickerController.delegate = self;
    
    if (sourceType == UIImagePickerControllerSourceTypeCamera)
    {
        imagePickerController.showsCameraControls = YES;
    }
    
    self.imagePickerController = imagePickerController;
    [self presentViewController:self.imagePickerController animated:YES completion:nil];
}


- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary *)info
{
    UIImage *image = [info valueForKey:UIImagePickerControllerOriginalImage];
    if (image.size.width > 500) {
        image = [image imageByScalingAndCroppingForSize:CGSizeMake(500, (int)(image.size.height * (500.0 / image.size.width)))];
    }
    CGFloat imageRatio = image.size.height / image.size.width;
    CGFloat bannerImageWidth = 200 / imageRatio;
    bannerImage.frame = CGRectMake(self.tableView.frame.size.width/2 - bannerImageWidth/2, 5, bannerImageWidth, 200);
    bannerImage.image = image;
    
    bannerSet = YES;
    addImage.text = @"Edit";
    [self dismissViewControllerAnimated:YES completion:NULL];
    self.imagePickerController = nil;
}


- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker
{
    [self dismissViewControllerAnimated:YES completion:NULL];
}


//http://stackoverflow.com/questions/11197509/ios-how-to-get-device-make-and-model
struct utsname systemInfo;
- (NSString *)rawDeviceName
{
    uname(&systemInfo);

    NSString* code = [NSString stringWithCString:systemInfo.machine
                                        encoding:NSUTF8StringEncoding];

    static NSDictionary* deviceNamesByCode = nil;

    if (!deviceNamesByCode) {
        
        deviceNamesByCode = @{@"i386"      :@"Simulator",
                              @"x86_64"    :@"Simulator",
                              @"iPod1,1"   :@"iPod Touch",      // (Original)
                              @"iPod2,1"   :@"iPod Touch",      // (Second Generation)
                              @"iPod3,1"   :@"iPod Touch",      // (Third Generation)
                              @"iPod4,1"   :@"iPod Touch",      // (Fourth Generation)
                              @"iPod7,1"   :@"iPod Touch",      // (6th Generation)
                              @"iPhone1,1" :@"iPhone",          // (Original)
                              @"iPhone1,2" :@"iPhone",          // (3G)
                              @"iPhone2,1" :@"iPhone",          // (3GS)
                              @"iPad1,1"   :@"iPad",            // (Original)
                              @"iPad2,1"   :@"iPad 2",          //
                              @"iPad3,1"   :@"iPad",            // (3rd Generation)
                              @"iPhone3,1" :@"iPhone 4",        // (GSM)
                              @"iPhone3,3" :@"iPhone 4",        // (CDMA/Verizon/Sprint)
                              @"iPhone4,1" :@"iPhone 4S",       //
                              @"iPhone5,1" :@"iPhone 5",        // (model A1428, AT&T/Canada)
                              @"iPhone5,2" :@"iPhone 5",        // (model A1429, everything else)
                              @"iPad3,4"   :@"iPad",            // (4th Generation)
                              @"iPad2,5"   :@"iPad Mini",       // (Original)
                              @"iPhone5,3" :@"iPhone 5c",       // (model A1456, A1532 | GSM)
                              @"iPhone5,4" :@"iPhone 5c",       // (model A1507, A1516, A1526 (China), A1529 | Global)
                              @"iPhone6,1" :@"iPhone 5s",       // (model A1433, A1533 | GSM)
                              @"iPhone6,2" :@"iPhone 5s",       // (model A1457, A1518, A1528 (China), A1530 | Global)
                              @"iPhone7,1" :@"iPhone 6 Plus",   //
                              @"iPhone7,2" :@"iPhone 6",        //
                              @"iPhone8,1" :@"iPhone 6S",       //
                              @"iPhone8,2" :@"iPhone 6S Plus",  //
                              @"iPad4,1"   :@"iPad Air",        // 5th Generation iPad (iPad Air) - Wifi
                              @"iPad4,2"   :@"iPad Air",        // 5th Generation iPad (iPad Air) - Cellular
                              @"iPad4,4"   :@"iPad Mini",       // (2nd Generation iPad Mini - Wifi)
                              @"iPad4,5"   :@"iPad Mini",       // (2nd Generation iPad Mini - Cellular)
                              @"iPad4,7"   :@"iPad Mini"        // (3rd Generation iPad Mini - Wifi (model A1599))
                              };
    }

    NSString* deviceName = [deviceNamesByCode objectForKey:code];

    if (!deviceName) {
        return code;
    }

    return deviceName;
}

@end

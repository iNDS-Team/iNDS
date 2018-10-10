//
//  iNDSCheatsTableViewController.m
//  iNDS
//
//  Created by Will Cobb on 12/27/15.
//  Copyright © 2015 iNDS. All rights reserved.
//

#import "iNDSCheatsTableViewController.h"
#import "iNDSCheatsFolderTableViewController.h"
#import "AppDelegate.h"
#import "iNDSEmulatorViewController.h"
#import "SCLAlertView.h"
#import <UnrarKit/UnrarKit.h>
#include <CommonCrypto/CommonDigest.h>

#include "emu.h"
#include "cheatSystem.h"

#import <UnrarKit/UnrarKit.h>

@interface iNDSCheatsTableViewController () {
    NSString    *currentGameId;
    iNDSEmulatorViewController  *emulationController;
    NSString    *cheatsArchivePath;
    BOOL        cheatsLoaded;
    NSXMLParser *cheatParser;
    
    NSMutableDictionary *cheatDict;
    NSMutableArray *noFolderCodes;
    
    //XML
    NSString    *currentElemet;
    NSString    *currentGame;
    BOOL        inCurrentGame;
    NSString    *currentFolder;
    BOOL        inFolder;
    BOOL        inCheat;
    NSString    *cheatName;
    NSString    *cheatCode;
    NSString    *cheatNote;
    
    //Segue
    NSString    *openFolder;
}

@end

@implementation iNDSCheatsTableViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    self.navigationItem.title = @"Cheats";
    
    emulationController = AppDelegate.sharedInstance.currentEmulatorViewController;
    cheatsArchivePath = [[NSBundle mainBundle] pathForResource:@"cheats" ofType:@"rar"];
    currentGameId = emulationController.game.rawTitle;
    
    UITapGestureRecognizer* tapRecon = [[UITapGestureRecognizer alloc] initWithTarget:AppDelegate.sharedInstance.currentEmulatorViewController action:@selector(toggleSettings:)];
    tapRecon.numberOfTapsRequired = 2;
    //[self.navigationController.navigationBar addGestureRecognizer:tapRecon];
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
//    NSString *cheatSavePath = [NSString stringWithUTF8String:(char *)cheats->filename];
    BOOL cheatSaved = cheats->load();
    
    //Eventually we might want to create our own NSInput stream to parse XML on the fly to reduce memory overhead and increase speed. This will work fine for now though
    if (!cheatSaved) {
        NSLog(@"Loading DB Cheats");
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            NSData *cheatData;
            NSError *error = nil;
            URKArchive *archive = [[URKArchive alloc] initWithPath:cheatsArchivePath error:&error];
            cheatData = [archive extractDataFromFile:@"usrcheat1.xml" progress:nil error:&error];
            currentFolder = @"";
            cheatParser = [[NSXMLParser alloc] initWithData:cheatData];
            cheatParser.delegate = self;
            [cheatParser parse];
            
        });
    } else {
        [self indexCheats];
        dispatch_async(dispatch_get_main_queue(), ^{
            [self.tableView reloadData];
        });
    }
}

- (void)viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    cheats->save();
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


- (void)parser:(NSXMLParser *)parser didStartElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qName attributes:(NSDictionary *)attributeDict {
    currentElemet = elementName;
    if (inCurrentGame) {
        if ([elementName isEqualToString:@"folder"]) {
            inFolder = YES;
        } else if ([elementName isEqualToString:@"cheat"]) {
            inCheat = YES;
        }
    }
}

- (void)indexCheats
{
    cheatDict = [NSMutableDictionary dictionary];
    noFolderCodes = [NSMutableArray array];
    for (u32 i = 0; i < cheats->getSize(); i++) {
        CHEATS_LIST *cheat = cheats->getItemByIndex(i);
        NSString *description = [NSString stringWithCString:cheat->description encoding:NSUTF8StringEncoding];
        NSString *folder = [description componentsSeparatedByString:@"||"][0];
        if (folder.length == 0) {
            [noFolderCodes addObject:[NSNumber numberWithInt:i]];
        } else {
            if (!cheatDict[folder]) {
                cheatDict[folder] = [NSMutableArray array];
            }
            [cheatDict[folder] addObject:[NSNumber numberWithInt:i]];
        }
        
    }
    cheatsLoaded = YES;
    
}

#pragma mark - XML

- (void)parser:(NSXMLParser *)parser foundCharacters:(NSString *)string {
    
    if ([currentElemet isEqualToString:@"gameid"]) {
        inCurrentGame = [string isEqualToString:currentGameId];
    } else if (inCurrentGame) {
        if (inCheat && [currentElemet isEqualToString:@"name"]) {
            cheatName = string;
        } else if (inCheat && [currentElemet isEqualToString:@"codes"]) {
            cheatCode = string;
        } else if (inCheat && [currentElemet isEqualToString:@"note"]) {
            cheatNote = string;
        } else if (inFolder && [currentElemet isEqualToString:@"name"]) {
            currentFolder = string;
        }
        
    }
    currentElemet = @"";
}

- (void)parser:(NSXMLParser *)parser didEndElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qName {
    
    if (inFolder && [elementName isEqualToString:@"folder"]) {
        inFolder = NO;
        currentFolder = @"";
    }
    if (inCheat && [elementName isEqualToString:@"cheat"]) {
        inCheat = NO;
        /*NSLog(@"Cheat:");
        NSLog(@"%@-%@", currentFolder, cheatName);
        NSLog(@"%@", cheatCode);
        NSLog(@"%@", cheatNote);*/
        char *cheatCodeChar = (char *)[cheatCode UTF8String];
        char *cheatDescription = (char *)[[NSString stringWithFormat:@"%@||%@||%@", currentFolder, cheatName, cheatNote] UTF8String];
        cheats->add_AR(cheatCodeChar, cheatDescription, NO);
        cheatName = @"";
        cheatCode = @"";
        cheatNote = @"";
    }
    
}

- (void)parserDidEndDocument:(NSXMLParser *)parser {
    cheats->save();
    [self indexCheats];
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.tableView reloadData];
    });
    
}

#pragma mark - Table view data source

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    return indexPath.section == 0;
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        if (indexPath.section == 0) {  // Del game
            u32 index = (u32)[noFolderCodes[indexPath.row] integerValue];
            cheats->remove(index);
            [self indexCheats];
            [tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
        }
    }
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell * cell;
    if (indexPath.section == 0) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"Cheat"];
        u32 index = (u32)[noFolderCodes[indexPath.row] integerValue];
        CHEATS_LIST *cheat = cheats->getItemByIndex(index);
        cell.accessoryType = cheat->enabled ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
        NSString *description = [NSString stringWithCString:cheat->description encoding:NSUTF8StringEncoding];
        NSString *name = [description componentsSeparatedByString:@"||"][1];
        cell.textLabel.text = name;
        cell.textLabel.lineBreakMode = NSLineBreakByWordWrapping;
        cell.textLabel.numberOfLines = 0;
        cell.textLabel.font = [UIFont systemFontOfSize:14];
    } else {
        if (!cheatsLoaded) {
            cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"Loading"];
            cell.textLabel.text = @"Loading Cheats";
            UIActivityIndicatorView * activity = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
            activity.frame = CGRectMake(self.view.frame.size.width - 50, 0, 44, 44);
            [activity startAnimating];
            [cell addSubview:activity];
        } else {
            cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"Folder"];
            cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
            NSString *folder = cheatDict.allKeys[indexPath.row];
            cell.textLabel.text = folder;
            cell.textLabel.lineBreakMode = NSLineBreakByWordWrapping;
            cell.textLabel.numberOfLines = 0;
            cell.textLabel.font = [UIFont systemFontOfSize:16];
            
            cell.imageView.image = [[UIImage imageNamed:@"Folder"] imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
            cell.imageView.tintColor = [UIColor grayColor];//[UIApplication sharedApplication].delegate.window.tintColor;
        }
    }
    return cell;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 2;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (section == 0) return [noFolderCodes count]; //num user cheats
    if (section == 1) {
        if (cheatsLoaded) {
            return cheatDict.allKeys.count; //num db cheats
        } else {
            return 1;
        }
    }
    return 0;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (cheatsLoaded) {
        if (indexPath.section == 0) {
            u32 index = (u32)[noFolderCodes[indexPath.row] integerValue];
            CHEATS_LIST *cheat = cheats->getItemByIndex(index);
            NSString *description = [NSString stringWithCString:cheat->description encoding:NSUTF8StringEncoding];
            NSString *name = [description componentsSeparatedByString:@"||"][1];
            NSAttributedString *attributedText =
            [[NSAttributedString alloc] initWithString:name attributes:@{NSFontAttributeName: [UIFont systemFontOfSize:14]}];
            CGRect rect = [attributedText boundingRectWithSize:CGSizeMake(tableView.bounds.size.width, CGFLOAT_MAX)
                                                       options:NSStringDrawingUsesLineFragmentOrigin
                                                       context:nil];
            return rect.size.height + 20;
        } else {
            NSString *folder = cheatDict.allKeys[indexPath.row];
            NSAttributedString *attributedText = [[NSAttributedString alloc] initWithString:folder attributes:@{NSFontAttributeName: [UIFont systemFontOfSize:14]}];
            CGRect rect = [attributedText boundingRectWithSize:CGSizeMake(tableView.bounds.size.width, CGFLOAT_MAX)
                                                       options:NSStringDrawingUsesLineFragmentOrigin
                                                       context:nil];
            return MAX(70, rect.size.height + 20);
        }
    }
    return 44;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(nonnull NSIndexPath *)indexPath
{
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    if (indexPath.section == 0) {
        u32 index = (u32)[noFolderCodes[indexPath.row] integerValue];
        CHEATS_LIST *cheat = cheats->getItemByIndex(index);
        cheat->enabled = !cheat->enabled;
        [tableView reloadRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationNone];
    } else {
        openFolder = cheatDict.allKeys[indexPath.row];
        [self performSegueWithIdentifier:@"OpenCheatFolder" sender:self];
    }
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if ([segue.identifier isEqualToString:@"OpenCheatFolder"]) {
        iNDSCheatsFolderTableViewController * vc = segue.destinationViewController;
        vc.folderCheats = cheatDict[openFolder];
        vc.navigationItem.title = openFolder;
    }
}

@end

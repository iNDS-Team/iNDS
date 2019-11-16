//
//  CHBgDropboxSync.m
//  Passwords
//
//  Created by Chris Hulbert on 4/03/12.
//
// This uses ARC
// Designed for DropboxSDK version 1.1

#import "CHBgDropboxSync.h"
#import <QuartzCore/QuartzCore.h>
#import <ObjectiveDropboxOfficial/ObjectiveDropboxOfficial.h>
//#import <DropboxSDK/DropboxSDK.h>
#import "ConciseKit.h"
#import "AppDelegate.h"
#import "ZAActivityBar.h"
#define lastSyncDefaultsKey @"CHBgDropboxSyncLastSyncFiles"

// Privates
@interface CHBgDropboxSync() {
    UILabel* workingLabel;
    DBUserClient* client;
    BOOL anyLocalChanges;
    BOOL syncing;
    NSMutableArray *deletedFiles;
    NSMutableArray *uploadedFiles;
}
- (NSDictionary*)getLocalStatus;
@end

// Singleton instance
CHBgDropboxSync* bgDropboxSyncInstance=nil;

@implementation CHBgDropboxSync

#pragma mark - Showing and hiding the syncing indicator

- (void)showWorking {
    NSLog(@"Syncing started!");
    [ZAActivityBar showSuccessWithStatus:@"Syncing started!" duration:2];
    [[NSNotificationCenter defaultCenter] postNotificationName:@"iNDSDropboxSyncStarted" object:self];
    [[UIApplication sharedApplication] setNetworkActivityIndicatorVisible:YES];
}

- (void)hideWorking {
    //[ZAActivityBar dismiss];
}

#pragma mark - Startup

- (void)startup {
    if (client) return; // Already started
    
    [self showWorking];
    
    client = [DBClientsManager authorizedClient];
    
    // Start getting the remote file list
    deletedFiles  = [NSMutableArray new];
    uploadedFiles = [NSMutableArray new];
    [self grabMetadata];
}

#pragma mark - For keeping track of the last synced status of a file in the nsuserdefaults

// This 'last sync status' is used to justify deletions - that is all it is used for.
// Some thoughts on this 'last sync status' method of keeping track of deletions:
// What happens if we update the 'last sync' for B after updating A?
// Eg we overwrite the last sync state for B after we update A
// Then we've lost track of whether we should do a deletion, and will start mistakenly doing downloads/uploads
// Maybe only remove the last-sync status for each file one at a time as each file attempts deletion
// And at sync completion, grab and update all of them one more time in case something ever slips through the net

// Did the file exist locally at the end of the last sync?
- (BOOL)lastSyncExists:(NSString*)file {
    return [[[NSUserDefaults standardUserDefaults] arrayForKey:lastSyncDefaultsKey] containsObject:file];
}

// Clear all last sync data on pairing change
- (void)lastSyncClear {
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:lastSyncDefaultsKey];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

// Do a full scan of the files and stores them all in the defaults. Only to be used when the sync is totally complete
- (void)lastSyncCompletionRescan {
    [[NSUserDefaults standardUserDefaults] setObject:self.getLocalStatus.allKeys forKey:lastSyncDefaultsKey];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

// Before you attempt to delete a file locally or remotely, call this so that it'll never try to delete that file again.
// Do it before 'attempt delete' instead of 'confirm delete' just in case the delete fails, then we'll fall back to a 'download it again' state for the next sync, which is better than accidentally deleting it erroneously again later
- (void)lastSyncRemove:(NSString*)file {
    NSMutableArray* arr = [NSMutableArray arrayWithArray:[[NSUserDefaults standardUserDefaults] arrayForKey:lastSyncDefaultsKey]];
    [arr removeObject:file];
    [[NSUserDefaults standardUserDefaults] setObject:arr forKey:lastSyncDefaultsKey];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

#pragma mark - Completion / shutdown

// Shutdown code that's common for success/fail/forced shutdowns
- (void)internalCommonShutdown {
    NSLog(@"CHBgDropboxSync internalCommonShutdown");
    // Autorelease the client using the following two lines, because we don't want it to release *just yet* because it probably called the function that called this, and would crash when the stack pops back to it.
    __autoreleasing DBUserClient* autoreleaseClient = client;
    [autoreleaseClient description];
    
    // Now release the client
    //    client.delegate = nil;
    client = nil;
    
    // Free this singleton (put it on the autorelease pool, for safety's sake)
    __autoreleasing CHBgDropboxSync* autoreleaseSingleton = bgDropboxSyncInstance;
    [autoreleaseSingleton description];
    bgDropboxSyncInstance = nil; // Clear the singleton
    
    if (anyLocalChanges) { // Only notify that there were changes at completion, not as we go, so the app doesn't get a half sync state
        [[NSNotificationCenter defaultCenter] postNotificationName:@"CHBgDropboxSyncUpdated" object:nil];
    }
}

// For forced shutdowns eg closing the app
- (void)internalShutdownForced {
    //[self hideWorking];
    NSLog(@"internalShutdownForced");
    [self internalCommonShutdown];
}

// For clean shutdowns on sync success
- (void)internalShutdownSuccess {
    [self lastSyncCompletionRescan];
    NSLog(@"Synced Completed!");
    [ZAActivityBar showSuccessWithStatus:@"Synced Completed!" duration:2];
    [[NSNotificationCenter defaultCenter] postNotificationName:@"iNDSDropboxSyncEnded" object:self];
    [[UIApplication sharedApplication] setNetworkActivityIndicatorVisible:NO];
    [self internalCommonShutdown];
}

// For failed shutdowns
- (void)internalShutdownFailed {
    NSLog(@"Failed to sync!");
    [ZAActivityBar showErrorWithStatus:@"Failed to sync!" duration:3];
    [[NSNotificationCenter defaultCenter] postNotificationName:@"iNDSDropboxSyncEnded" object:self];
    [[UIApplication sharedApplication] setNetworkActivityIndicatorVisible:NO];
    [self internalCommonShutdown];
}

#pragma mark - For when the steps complete

// This re-starts the 'check the metadata' step again, which will then check for any syncing that needs doing, and then kick it off
- (void)stepComplete {
    // Kick off the check the metadata with a little delay so we don't overdo things
    //    [client performSelector:@selector(loadMetadata:) withObject:@"/" afterDelay:.3];
    [self performSelector:@selector(grabMetadata) withObject:nil afterDelay:0.3];
}

- (void)grabMetadata {
    [[client.filesRoutes listFolder:@""] setResponseBlock:^(DBFILESListFolderResult * _Nullable result, DBFILESListFolderError * _Nullable routeError, DBRequestError * _Nullable networkError) {
        if (result) {
            NSMutableDictionary* remoteFiles = [NSMutableDictionary dictionary];
            NSMutableDictionary* remoteFileRevs = [NSMutableDictionary dictionary];
            
            for (DBFILESMetadata* item in result.entries) {
                if ([item isKindOfClass:[DBFILESFileMetadata class]]) {
                    DBFILESFileMetadata* casted = (DBFILESFileMetadata *) item;
                    [remoteFiles setObject:casted.serverModified forKey:[self noSlash:casted.pathDisplay]];
                    [remoteFileRevs setObject:casted.rev forKey:[self noSlash:item.pathDisplay]];
                }
            }
            // Now do the comparisons to figure out what needs doing
            BOOL allComplete = [self syncStepWithRemoteFiles:remoteFiles andRevs:remoteFileRevs];
            
            if (allComplete) { // All done - nothing to do!
                [self internalShutdownSuccess];
            } else {
                NSLog(@"Not done.");
            }
        } else {
            NSLog(@"%@\n%@\n", routeError, networkError);
            [self internalShutdownFailed];
        }
    }];
}


#pragma mark - The async dropbox steps

- (void)startTaskLocalDelete:(NSString*)file {
    NSLog(@"Sync: Deleting local file %@", file);
    NSError *fileErr;
    [[NSFileManager defaultManager] removeItemAtPath:[[[AppDelegate sharedInstance] batteryDir] stringByAppendingPathComponent:file] error:&fileErr];
    
    if (fileErr) {
        NSLog(@"Error: %@", fileErr);
    }
    
    [self stepComplete];
    anyLocalChanges = YES; // So that when we complete, we notify that there were local changes
}

/*
 Upload
 NOTE: This function only works for files < 150 MB
 which we assume is all .dsv files. The rational behind this
 is that currently, batch upload is the only function in dropbox which
 support automatic chunk size calculation while uploading, however,
 this function does not support overwriting. Thus we are limited to these
 fixed-size functions. We could, however, do something in the future where
 we delete the old file before-hand, however this would require an extra
 operation.
 */
- (void)startTaskUpload:(NSString*)file rev:(NSString*)rev {
    NSLog(@"%@", file);
    if (![uploadedFiles containsObject:file]) {
        NSLog(@"Sync: Uploading file %@, %@", file, rev?@"overwriting":@"new");
        /*
         [client uploadFile:file toPath:@"/" withParentRev:rev fromPath:[[[AppDelegate sharedInstance] batteryDir] stringByAppendingPathComponent:file]];
         */
        
        DBFILESWriteMode *mode = [[DBFILESWriteMode alloc] initWithOverwrite];
        NSString *path = [[[AppDelegate sharedInstance] batteryDir] stringByAppendingPathComponent:file];
        [uploadedFiles addObject:file];
        
        /*
         We could set a "clientModified" date, however, Dropbox store both a client
         modified date and a server modified date and we would have to keep track of both
         and it could get confusing. We could possible sort it out later, but for now,
         lets just stick to the old system.
         */
        [[[client.filesRoutes uploadUrl:[@"/" stringByAppendingPathComponent:file] mode:mode autorename:@(NO) clientModified:nil mute:@(YES) propertyGroups:nil strictConflict:@(NO) inputUrl:path]
          setResponseBlock:^(DBFILESFileMetadata *result, DBFILESUploadError *routeError, DBRequestError *networkError) {
              if (result) {
                  NSString *dropboxFilePath = result.pathDisplay;
                  NSLog(@"File successfully uploaded from %@ on local machine to %@ in Dropbox.",
                        path, dropboxFilePath);
                  
                  NSDictionary* attr = $dict(result.serverModified, NSFileModificationDate);
                  NSError *attrErr;
                  [[NSFileManager defaultManager] setAttributes:attr ofItemAtPath:path error:&attrErr];
                  [self stepComplete];
              } else {
                  NSLog(@"%@\n%@\n", routeError, networkError);
                  [self internalShutdownFailed];
              }
          }] setProgressBlock:^(int64_t bytesUploaded, int64_t totalBytesUploaded, int64_t totalBytesExpectedToUploaded) {
              NSLog(@"\n%lld\n%lld\n%lld\n", bytesUploaded, totalBytesUploaded, totalBytesExpectedToUploaded);
          }];
        
    } else {
        NSLog(@"Prevented double file upload");
    }
    
}
// End upload

// Download
- (void)startTaskDownload:(NSString*)file {
    if (![deletedFiles containsObject:file]) {
        NSLog(@"Sync: Downloading file %@", file);
        /*
         [client loadFile:$str(@"/%@", file) intoPath:[[[AppDelegate sharedInstance] batteryDir] stringByAppendingPathComponent:file]];
         */
        NSURL *dest = [NSURL fileURLWithPath:[[[AppDelegate sharedInstance] batteryDir] stringByAppendingPathComponent:file]];
        
        [[client.filesRoutes downloadUrl:$str(@"/%@", file) overwrite:YES destination:dest] setResponseBlock:^(DBFILESFileMetadata * _Nullable result, DBFILESDownloadError * _Nullable routeError, DBRequestError * _Nullable networkError, NSURL * _Nonnull destination) {
            if (result) {
                NSLog(@"Downloaded >%@<, it's DB date is: %@", destination, [result.serverModified descriptionWithLocale:[NSLocale currentLocale]]);
                NSDictionary* attr = $dict(result.serverModified, NSFileModificationDate);
                [[NSFileManager defaultManager] setAttributes:attr ofItemAtPath:destination.path error:nil];
                [self stepComplete];
                self->anyLocalChanges = YES; // So that when we complete, we notify that there were local changes
            } else {
                NSLog(@"dl%@\n%@\n", routeError, networkError);
                [self internalShutdownFailed];
            }
        }];
    } else {
        NSLog(@"Prevented Dropbox Crash. Trying to download a deleted file");
    }
}
// End download

// Remote delete
- (void)startTaskRemoteDelete:(NSString*)file {
    NSLog(@"Sync: Deleting remote file %@", file);
    [deletedFiles addObject:file];
    [[client.filesRoutes delete_V2:$str(@"/%@", file)] setResponseBlock:^(DBFILESMetadata * _Nullable result, DBFILESDeleteError * _Nullable routeError, DBRequestError * _Nullable networkError) {
        if (result) {
            [self stepComplete];
        } else {
            [self internalShutdownFailed];
        }
    }];
    [self stepComplete];
}
// End remote delete

#pragma mark - Figure out what needs doing after we get the remote metadata

// Get the current status of files and folders as a dict: Path (eg 'abc.txt') => last mod date
- (NSDictionary*)getLocalStatus {
    NSMutableDictionary* localFiles = [NSMutableDictionary dictionary];
    NSString* root = [[AppDelegate sharedInstance] batteryDir]; // Where are we going to sync to
    for (NSString* item in [[NSFileManager defaultManager] contentsOfDirectoryAtPath:root error:nil]) {
        // Skip hidden/system files - you may want to change this if your files start with ., however dropbox errors on many 'ignored' files such as .DS_Store which you'll want to skip
        if ([item hasPrefix:@"."]) continue;
        
        // Get the full path and attribs
        NSString* itemPath = [root stringByAppendingPathComponent:item];
        NSDictionary* attribs = [[NSFileManager defaultManager] attributesOfItemAtPath:itemPath error:nil];
        BOOL isFile = $eql(attribs.fileType, NSFileTypeRegular);
        
        if (isFile) {
            [localFiles setObject:attribs.fileModificationDate forKey:item];
        }
    }
    return localFiles;
}

// Starts a single sync step, returning YES if nothing needs doing. RemoteFiles is: Path (eg 'abc.txt') => last mod date
- (BOOL)syncStepWithRemoteFiles:(NSDictionary*)remoteFiles andRevs:(NSDictionary*)remoteRevs {
    NSDictionary* localFiles = [self getLocalStatus]; // Get the local filesystem
    [[NSUserDefaults standardUserDefaults] synchronize]; // Make sure the user defaults data is up to date
    
    NSMutableSet* all = [NSMutableSet set]; // Get a complete list of all files both local and remote
    [all addObjectsFromArray:localFiles.allKeys];
    [all addObjectsFromArray:remoteFiles.allKeys];
    for (NSString* file in all) {
        NSDate* local = [localFiles objectForKey:file];
        NSDate* remote = [remoteFiles objectForKey:file];
        BOOL lastSyncExists = [self lastSyncExists:file];
        if (local && remote) {
            // File is in both places, but are the dates the same?
            double delta = local.timeIntervalSinceReferenceDate - remote.timeIntervalSinceReferenceDate;
            BOOL same = ABS(delta)<2; // If they're within 2 seconds, that's close enough to be the same
            if (!same) {
                // Dates are different, so we need to do something
                // If this was the proper algorithm, we'd check to see if both had changed since the last sync
                // And if so, keep both and rename the older one '*_conflicted'
                if (local.timeIntervalSinceReferenceDate > remote.timeIntervalSinceReferenceDate) {
                    // Local is newer
                    // So send the local file to dropbox, overwriting the existing one with the given 'rev'
                    //NSLog(@"UL1");
                    [self startTaskUpload:file rev:[remoteRevs objectForKey:file]];
                    return NO;
                } else {
                    // Remote is newer
                    // So download the file
                    //NSLog(@"DL1");
                    [self startTaskDownload:file];
                    return NO;
                }
            }
        } else { // Not in both places
            // Say at the end of last sync, it would be in all 3 places: local, remote, and sync
            if (remote && !local) {
                // Dropbox has it, we don't
                // If it was added to db since last sync, it won't be in our sync list, so add it local
                // If it was removed locally since last sync, it'll be in our sync list, so remove from db
                // If never been synced, it won't be in our sync list, so add it locally
                if (lastSyncExists) {
                    // Remove from dropbox
                    //NSLog(@"RD1");
                    [self lastSyncRemove:file]; // Clear the 'last sync' for just this file, so we don't try deleting it again
                    [self startTaskRemoteDelete:file];
                    return NO;
                } else {
                    // Download it
                    //NSLog(@"DL2: %@", remote);
                    [self startTaskDownload:file];
                    return NO;
                }
            }
            if (local && !remote) {
                // We have it, dropbox doesn't
                // If it was added locally since last sync, it won't be in our sync list, so upload it
                // If it was deleted from db since last sync, it will be in our sync list, so delete it locally
                // If never synced, it won't be in our sync list, so upload it
                if (lastSyncExists) {
                    //NSLog(@"RD2");
                    [self lastSyncRemove:file]; // Clear the 'last sync' for just this file, so we don't try deleting it again
                    [self startTaskLocalDelete:file]; // Delete locally
                    return NO;
                } else {
                    // Upload it. 'rev' should be nil here anyway.
                    //NSLog(@"UL2");
                    [self startTaskUpload:file rev:[remoteRevs objectForKey:file]];
                    return NO;
                }
            }
        }
    }
    return YES; // Nothing needs doing
}

#pragma mark - Callbacks for the load-remote-folder-contents

// Remove the leading slash
- (NSString*)noSlash:(NSString*)file {
    return [file stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"/"]];
}

#pragma mark - Singleton management

+ (CHBgDropboxSync*)i {
    if (!bgDropboxSyncInstance) {
        bgDropboxSyncInstance = [[CHBgDropboxSync alloc] init];
    }
    return bgDropboxSyncInstance;
}

#pragma mark - Publicly accessible stuff you should access from your app delegate

// Call me in your app delegate's applicationDidBecomeActive (eg at startup and become-active) and when you link
// and basically any time you've changed data and want to sync again
+ (void)start {
    NSLog(@"Starting Dropbox Sync...");
    if (![[DBClientsManager authorizedClient] isAuthorized]) {
        NSLog(@"Dropbox not linked, so nothing to do");
        return;
    } // Not linked, so nothing to do
    [[self i] startup];
}

// Call me from your app delegate when your app closes/goes to the background/unpairs
+ (void)forceStopIfRunning {
    [bgDropboxSyncInstance internalShutdownForced];
}

// Called when they pair or unpair or restore a backup, clears the lastsync status so we dont inadvertantly delete things next time we sync
+ (void)clearLastSyncData {
    [[self i] lastSyncClear]; // Clear the last sync status so
    // Since last sync status is only used to justify deletes, it is safe to clear (it'll only possibly cause data un-deletion)
}

@end

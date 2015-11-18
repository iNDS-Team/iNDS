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
#import <DropboxSDK/DropboxSDK.h>
#import "ConciseKit.h"
#import "AppDelegate.h"
#import "ZAActivityBar.h"

#define lastSyncDefaultsKey @"CHBgDropboxSyncLastSyncFiles"

// Privates
@interface CHBgDropboxSync() {
    UILabel* workingLabel;
    DBRestClient* client;
    BOOL anyLocalChanges;
}
- (NSDictionary*)getLocalStatus;
@end

// Singleton instance
CHBgDropboxSync* bgDropboxSyncInstance=nil;

@implementation CHBgDropboxSync

#pragma mark - Showing and hiding the syncing indicator

- (void)showWorking {
    [ZAActivityBar showWithStatus:@"Syncing save files..."];
    [[UIApplication sharedApplication] setNetworkActivityIndicatorVisible:YES];
}

- (void)hideWorking {
    [ZAActivityBar dismiss];
}

#pragma mark - Startup

- (void)startup {
    if (client) return; // Already started
    
    [self showWorking];
    
    client = [[DBRestClient alloc] initWithSession:[DBSession sharedSession]];
    client.delegate = self;
    
    // Start getting the remote file list
    [client loadMetadata:@"/"];
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
    // Autorelease the client using the following two lines, because we don't want it to release *just yet* because it probably called the function that called this, and would crash when the stack pops back to it.
    __autoreleasing DBRestClient* autoreleaseClient = client;
    [autoreleaseClient description];
    
    // Now release the client
    client.delegate = nil;
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
    [self internalCommonShutdown];
}

// For clean shutdowns on sync success
- (void)internalShutdownSuccess {
    [self lastSyncCompletionRescan];
    [ZAActivityBar showSuccessWithStatus:@"Synced!" duration:2];
    [[UIApplication sharedApplication] setNetworkActivityIndicatorVisible:NO];
    [self internalCommonShutdown];
}

// For failed shutdowns
- (void)internalShutdownFailed {
    [ZAActivityBar showErrorWithStatus:@"Failed to sync!" duration:4];
    [[UIApplication sharedApplication] setNetworkActivityIndicatorVisible:NO];
    [self internalCommonShutdown];
}

#pragma mark - For when the steps complete

// This re-starts the 'check the metadata' step again, which will then check for any syncing that needs doing, and then kick it off
- (void)stepComplete {
    // Kick off the check the metadata with a little delay so we don't overdo things
    [client performSelector:@selector(loadMetadata:) withObject:@"/" afterDelay:.05];
}

#pragma mark - The async dropbox steps

- (void)startTaskLocalDelete:(NSString*)file {
    NSLog(@"Sync: Deleting local file %@", file);
    [ZAActivityBar showWithStatus:[NSString stringWithFormat:@"Removing file %@", file]];
    [[NSFileManager defaultManager] removeItemAtPath:[[[AppDelegate sharedInstance] batteryDir] stringByAppendingPathComponent:file] error:nil];
    [self stepComplete];
    anyLocalChanges = YES; // So that when we complete, we notify that there were local changes
}

// Upload
- (void)startTaskUpload:(NSString*)file rev:(NSString*)rev {
    NSLog(@"Sync: Uploading file %@, %@", file, rev?@"overwriting":@"new");
    [ZAActivityBar showWithStatus:[NSString stringWithFormat:@"Uploading file %@, %@", file, rev?@"overwriting":@"new"]];
    [client uploadFile:file toPath:@"/" withParentRev:rev fromPath:[[[AppDelegate sharedInstance] batteryDir] stringByAppendingPathComponent:file]];
}
- (void)restClient:(DBRestClient *)client uploadedFile:(NSString *)destPath from:(NSString *)srcPath metadata:(DBMetadata *)metadata {
    // Now the file has uploaded, we need to set its 'last modified' date locally to match the date on dropbox.
    // Unfortunately we can't change the dropbox date to match the local date, which would be more appropriate, really.
    NSDictionary* attr = $dict(metadata.lastModifiedDate, NSFileModificationDate);
    [[NSFileManager defaultManager] setAttributes:attr ofItemAtPath:srcPath error:nil];
    [self stepComplete];
}
- (void)restClient:(DBRestClient*)client uploadFileFailedWithError:(NSError*)error {
    [self internalShutdownFailed];
}
// End upload

// Download
- (void)startTaskDownload:(NSString*)file {
    NSLog(@"Sync: Downloading file %@", file);
    [ZAActivityBar showWithStatus:[NSString stringWithFormat:@"Downloading file %@", file]];
    [client loadFile:$str(@"/%@", file) intoPath:[[[AppDelegate sharedInstance] batteryDir] stringByAppendingPathComponent:file]];
}
- (void)restClient:(DBRestClient*)client loadedFile:(NSString*)destPath contentType:(NSString*)contentType metadata:(DBMetadata*)metadata {
    // Now the file has downloaded, we need to set its 'last modified' date locally to match the date on dropbox
    NSLog(@"Downloaded >%@<, it's DB date is: %@", destPath, [metadata.lastModifiedDate descriptionWithLocale:[NSLocale currentLocale]]);
    NSDictionary* attr = $dict(metadata.lastModifiedDate, NSFileModificationDate);
    [[NSFileManager defaultManager] setAttributes:attr ofItemAtPath:destPath error:nil];
    [self stepComplete];
    anyLocalChanges = YES; // So that when we complete, we notify that there were local changes
}
- (void)restClient:(DBRestClient *)client loadFileFailedWithError:(NSError *)error {
    [self internalShutdownFailed];
}
// End download

// Remote delete
- (void)startTaskRemoteDelete:(NSString*)file {
    NSLog(@"Sync: Deleting remote file %@", file);
    [ZAActivityBar showWithStatus:[NSString stringWithFormat:@"Removing remote file %@", file]];
    [client deletePath:$str(@"/%@", file)];
    [self stepComplete];
}
- (void)restClient:(DBRestClient *)client deletedPath:(NSString *)path {
    [self stepComplete];
}
- (void)restClient:(DBRestClient *)client deletePathFailedWithError:(NSError *)error {
    [self internalShutdownFailed];
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
                    [self startTaskUpload:file rev:[remoteRevs objectForKey:file]];
                    return NO;
                } else {
                    // Remote is newer
                    // So download the file
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
                    [self lastSyncRemove:file]; // Clear the 'last sync' for just this file, so we don't try deleting it again
                    [self startTaskRemoteDelete:file];
                    return NO;
                } else {
                    // Download it
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
                    [self lastSyncRemove:file]; // Clear the 'last sync' for just this file, so we don't try deleting it again
                    [self startTaskLocalDelete:file]; // Delete locally
                    return NO;
                } else {
                    // Upload it. 'rev' should be nil here anyway.
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

// Called by dropbox when the metadata for a folder has returned
- (void)restClient:(DBRestClient*)_client loadedMetadata:(DBMetadata*)metadata {
    NSMutableDictionary* remoteFiles = [NSMutableDictionary dictionary];
    NSMutableDictionary* remoteFileRevs = [NSMutableDictionary dictionary];
    
    for (DBMetadata* item in metadata.contents) {
        if (item.isDirectory) {
            // Ignore directories for simplicity's sake
        } else {
            [remoteFiles setObject:item.lastModifiedDate forKey:[self noSlash:item.path]];
            [remoteFileRevs setObject:item.rev forKey:[self noSlash:item.path]];
        }
    }
    
    // Now do the comparisons to figure out what needs doing
    BOOL allComplete = [self syncStepWithRemoteFiles:remoteFiles andRevs:remoteFileRevs];
    
    if (allComplete) { // All done - nothing to do!
        [self internalShutdownSuccess];
    }
}
- (void)restClient:(DBRestClient*)client metadataUnchangedAtPath:(NSString*)path {
    [self internalShutdownFailed];
}
- (void)restClient:(DBRestClient*)client loadMetadataFailedWithError:(NSError*)error {
    [self internalShutdownFailed];
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
    if (![[DBSession sharedSession] isLinked]) return; // Not linked, so nothing to do
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

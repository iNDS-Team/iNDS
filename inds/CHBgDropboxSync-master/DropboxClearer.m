//
//  DropboxClearer.m
//  Passwords
//
//  Created by Chris on 10/03/12.
//

#import "DropboxClearer.h"
#import <DropboxSDK/DropboxSDK.h>

@interface DropboxClearer() {
    DBRestClient* client;
    int successfulDeletesTogo;
}
@property(copy) dropboxCleared complete;
@end

@implementation DropboxClearer
@synthesize complete;

#pragma mark - Completion / cleanup

- (void)finish:(BOOL)success {    
    // Autorelease the client using the following two lines, because we don't want it to release *just yet* because it probably called the function that called this, and would crash when the stack pops back to it.
    // http://stackoverflow.com/questions/9643770/whats-the-equivalent-of-something-retain-autorelease-in-arc
    __autoreleasing DBRestClient* autoreleaseClient = client;
    [autoreleaseClient description];
    
    // Free the client
    client.delegate = nil;
    client = nil;
    
    // Call the block
    if (complete) {
        complete(success);
        complete = nil;
    }
    [[self class] cancelPreviousPerformRequestsWithTarget:self]; // Cancel the 'timeout' message that was intended for memory management. This'll free self.
}

- (void)timeout {
    [self finish:NO];
}

#pragma mark - Callbacks upon deletion

- (void)restClient:(DBRestClient *)client deletedPath:(NSString *)path {
    successfulDeletesTogo--;
    if (successfulDeletesTogo<=0) {
        [self finish:YES];
    }
}
- (void)restClient:(DBRestClient *)client deletePathFailedWithError:(NSError *)error {
    [self finish:NO];
}

#pragma mark - Getting the list of files to erase

// Called by dropbox when the metadata for a folder has returned
- (void)restClient:(DBRestClient*)_client loadedMetadata:(DBMetadata*)metadata {
    successfulDeletesTogo=0;
    for (DBMetadata* item in metadata.contents) {
        successfulDeletesTogo++;
        [client deletePath:item.path];
    }
    if (!metadata.contents.count) { // All done - nothing to do!
        [self finish:YES];
    }
}
- (void)restClient:(DBRestClient*)client metadataUnchangedAtPath:(NSString*)path {
    [self finish:NO];
}
- (void)restClient:(DBRestClient*)client loadMetadataFailedWithError:(NSError*)error {
    [self finish:NO];
}

#pragma mark - Starting the process

- (void)start {
    client = [[DBRestClient alloc] initWithSession:[DBSession sharedSession]];
    client.delegate = self;
    [client loadMetadata:@"/"];
}

// Create a dropbox clearer that calls your completion block when it's done. Handles memory for you.
+ (void)doClear:(dropboxCleared)complete {
    if ([[DBSession sharedSession] isLinked]) {
        DropboxClearer* dc = [[DropboxClearer alloc] init];
        dc.complete = complete;
        [dc performSelector:@selector(timeout) withObject:nil afterDelay:30]; // A timeout, also this makes the runloop retain the clearer for us
        [dc start];
    } else {
        complete(YES); // Not linked
    }
}

@end

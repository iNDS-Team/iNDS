//
//  iNDSDBManager.h
//  iNDS
//
//  Created by Frederick Morlock on 7/31/18.
//  Copyright © 2018 iNDS. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <sqlite3.h>

@interface iNDSDBManager : NSObject {
    sqlite3* _database;
}

+ (id) sharedInstance;

- (void) query:(NSString*)queryString result:(void (^)(int resultCode, sqlite3_stmt *statement))result;
- (void) openDB;
- (void) closeDB;


@end

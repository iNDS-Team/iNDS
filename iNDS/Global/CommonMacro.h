//
//  CommonMacro.h
//  iNDS
//
//  Created by Shvier on 27/02/2018.
//  Copyright © 2018 iNDS. All rights reserved.
//

#ifndef CommonMacro_h
#define CommonMacro_h

#define ScreenWidth \
([[UIScreen mainScreen] respondsToSelector:@selector(nativeBounds)] ? [UIScreen mainScreen].nativeBounds.size.width/[UIScreen mainScreen].nativeScale : CGRectGetWidth([[UIScreen mainScreen] bounds]))

#define ScreenHeight \
([[UIScreen mainScreen] respondsToSelector:@selector(nativeBounds)] ? [UIScreen mainScreen].nativeBounds.size.height/[UIScreen mainScreen].nativeScale : CGRectGetHeight([[UIScreen mainScreen] bounds]))

#define ScreenMaxLength (MAX(ScreenWidth, ScreenHeight))
#define IsiPhone (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)

#define IsiPhoneX (IsiPhone && ScreenMaxLength == 812.0)

#endif /* CommonMacro_h */

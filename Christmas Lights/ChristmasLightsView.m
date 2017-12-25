#import "ChristmasLightsView.h"
#include "effects.h"
#include <math.h>

static const CGFloat kLEDSize = 5.0;

@implementation ChristmasLightsView {
    NSArray<UIView *> *_lights;
    NSArray<UIColor *> *_palette;
}

- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
//        self.backgroundColor = [UIColor redColor];
        
        _palette = @[[UIColor blueColor], [UIColor yellowColor], [UIColor greenColor], [UIColor redColor]];
        
        NSMutableArray<UIView *> *lights = [NSMutableArray new];
        for (NSInteger i = 0; i < kLEDCount; ++i) {
            UIView *led = [UIView new];
            led.backgroundColor = _palette[i % _palette.count];
            [self addSubview:led];
            [lights addObject:led];
        }
        _lights = [lights copy];
    }
    return self;
}

- (void)layoutSubviews {
    [super layoutSubviews];
    NSLog(@"==== START OF LAYOUT ====");
    
    CGRect rect = self.bounds;
    
    CGFloat topSpace = rect.size.width - 2 * kLEDSize;
    CGFloat sideSpace = rect.size.height;
    CGFloat totalSpace = sideSpace + topSpace + sideSpace;
    
    CGFloat ledSpacingApprox = ceil((totalSpace - kLEDSize * kLEDCount) / (kLEDCount - 1));
    
    NSInteger horzLEDCount = floor(topSpace / (kLEDSize + ledSpacingApprox));
    NSInteger vertLEDCountL = (kLEDCount - horzLEDCount + 1) / 2;
    NSInteger vertLEDCountR = kLEDCount - horzLEDCount - vertLEDCountL;

    CGFloat remainingHeight = sideSpace;
    CGFloat y = remainingHeight;
    for (NSInteger i = 0, count = vertLEDCountL; i < count; ++i) {
        UIView *led = _lights[i];
        led.frame = CGRectMake(0, y - kLEDSize, kLEDSize, kLEDSize);
//        NSLog(@"LED %ld frame = %@", (long)i, NSStringFromCGRect(led.frame));

        NSInteger remainingCount = count - i;
        CGFloat spacing = (remainingCount <= 1 ? 0 : floor((remainingHeight - remainingCount * kLEDSize) / (remainingCount - 1)));
//        NSLog(@"spacing = %.3lf", spacing);
        y -= kLEDSize + spacing;
        remainingHeight -= kLEDSize + spacing;
    }

    CGFloat remainingWidth = topSpace;
    CGFloat x = kLEDSize;
    NSInteger first = vertLEDCountL;
    for (NSInteger i = 0, count = horzLEDCount; i < count; ++i) {
        NSInteger remainingCount = count - i;
        CGFloat spacing = floor((remainingWidth - remainingCount * kLEDSize) / (remainingCount + 1));
        NSLog(@"spacing = %.3lf", spacing);
        x += spacing;
        remainingWidth -= spacing;

        UIView *led = _lights[first+i];
        led.frame = CGRectMake(x, 0, kLEDSize, kLEDSize);
//        NSLog(@"LED %ld frame = %@", (long)(first+i), NSStringFromCGRect(led.frame));
        
        x += kLEDSize;
        remainingWidth -= kLEDSize;
    }

    remainingHeight = sideSpace;
    y = 0;
    first = vertLEDCountL + horzLEDCount;
    for (NSInteger i = 0, count = vertLEDCountR; i < count; ++i) {
        UIView *led = _lights[first+i];
        led.frame = CGRectMake(rect.size.width - kLEDSize, y, kLEDSize, kLEDSize);
//        NSLog(@"LED %ld frame = %@", (long)(first+i), NSStringFromCGRect(led.frame));
        
        NSInteger remainingCount = count - i;
        CGFloat spacing = (remainingCount <= 1 ? 0 : floor((remainingHeight - remainingCount * kLEDSize) / (remainingCount - 1)));
        y += kLEDSize + spacing;
        remainingHeight -= kLEDSize + spacing;
    }
}

@end

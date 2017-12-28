#import "ChristmasLightsView.h"
#include "effects.h"
#include <math.h>

static const NSTimeInterval kAnimationStepTime = 0.25;

static const CGFloat kLEDSize = 8.0;

@implementation ChristmasLightsView {
    NSArray<UIView *> *_lights;
    NSArray<UIColor *> *_palette;
    uint8_t *_pixels;
    NSDate *_startDate;
    CADisplayLink *_displayLink;
    uint32_t _prevTime;
}

- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
//        self.backgroundColor = [UIColor redColor];
        
        _palette = @[[UIColor blackColor], [UIColor blueColor], [UIColor yellowColor], [UIColor greenColor], [UIColor redColor]];
        
        NSMutableArray<UIView *> *lights = [NSMutableArray new];
        for (NSInteger i = 0; i < kLEDCount; ++i) {
            UIView *led = [UIView new];
            led.layer.cornerRadius = kLEDSize / 2;
            [self addSubview:led];
            [lights addObject:led];
        }
        _lights = [lights copy];
        
        _pixels = malloc(sizeof(uint8_t) * kLEDCount);
        _prevTime = UINT32_MAX;
    }
    return self;
}

- (void)didMoveToWindow {
    if (self.window != nil) {
        _startDate = [NSDate date];
        for (int i = 0; i < kLEDCount; i++) {
            _pixels[i] = 0;
        }
        effects_reset();
        if (_displayLink == nil) {
            _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(drawNextFrame:)];
            [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
        }
    } else {
        [_displayLink invalidate];
        _displayLink = nil;
    }
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

- (void)drawNextFrame:(CADisplayLink *)sender {
    if (_startDate == nil) {
        return;
    }
    NSTimeInterval elapsed = [[NSDate date] timeIntervalSinceDate:_startDate];
    
    uint32_t t = (uint32_t)floor(elapsed / kAnimationStepTime);
    if (t == _prevTime) {
        return;
    }
    _prevTime = t;

    NSLog(@"drawNextFrame t=%lu", (unsigned long)t);
    effects_get(t, _pixels);
    [self displayPixels];
}

- (void)displayPixels {
    for (NSInteger i = 0; i < kLEDCount; ++i) {
        UIView *led = _lights[i];
        led.backgroundColor = _palette[_pixels[i]];
    }
}

@end

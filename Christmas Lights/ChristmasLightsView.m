#import "ChristmasLightsView.h"
#include "effects.h"
#include <math.h>

static const CGFloat kLEDSize = 8.0;
static PARAMS params;

@implementation ChristmasLightsView {
    NSArray<UIView *> *_lights;
    NSArray<UIColor *> *_palette;
    uint8_t *_pixels;
    NSDate *_lastTickDate;
    CADisplayLink *_displayLink;
    UILabel *_effectLabel;
    UIButton *_prevButton, *_nextButton;
}

- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
//        self.backgroundColor = [UIColor redColor];
        
        _palette = @[[UIColor blackColor], [UIColor blueColor], [UIColor yellowColor], [UIColor greenColor], [UIColor redColor]];
        
        _effectLabel = [UILabel new];
        _effectLabel.translatesAutoresizingMaskIntoConstraints = false;
        _effectLabel.textColor = [UIColor whiteColor];
        [self addSubview:_effectLabel];

        _prevButton = [UIButton buttonWithType:UIButtonTypeSystem];
        _prevButton.translatesAutoresizingMaskIntoConstraints = false;
        [_prevButton setTitle:@"←" forState:UIControlStateNormal];
        [_prevButton addTarget:self action:@selector(prev:) forControlEvents:UIControlEventTouchUpInside];
        [self addSubview:_prevButton];

        _nextButton = [UIButton buttonWithType:UIButtonTypeSystem];
        _nextButton.translatesAutoresizingMaskIntoConstraints = false;
        [_nextButton setTitle:@"→" forState:UIControlStateNormal];
        [_nextButton addTarget:self action:@selector(next:) forControlEvents:UIControlEventTouchUpInside];
        [self addSubview:_nextButton];

        [NSLayoutConstraint activateConstraints:@[
            [_effectLabel.centerXAnchor constraintEqualToAnchor:self.centerXAnchor],
            [_effectLabel.topAnchor constraintEqualToAnchor:self.topAnchor constant:30],
            [_prevButton.centerYAnchor constraintEqualToAnchor:_effectLabel.centerYAnchor],
            [_prevButton.rightAnchor constraintEqualToAnchor:_effectLabel.leftAnchor constant:-20],
            [_prevButton.widthAnchor constraintEqualToConstant:44],
            [_prevButton.heightAnchor constraintEqualToConstant:44],
            [_nextButton.centerYAnchor constraintEqualToAnchor:_effectLabel.centerYAnchor],
            [_nextButton.leftAnchor constraintEqualToAnchor:_effectLabel.rightAnchor constant:20],
            [_nextButton.widthAnchor constraintEqualToConstant:44],
            [_nextButton.heightAnchor constraintEqualToConstant:44],
        ]];
        
        NSMutableArray<UIView *> *lights = [NSMutableArray new];
        for (NSInteger i = 0; i < kLEDCount; ++i) {
            UIView *led = [UIView new];
            led.layer.cornerRadius = kLEDSize / 2;
            [self addSubview:led];
            [lights addObject:led];
        }
        _lights = [lights copy];
        
        _pixels = malloc(sizeof(uint8_t) * kLEDCount);
    }
    return self;
}

- (IBAction)prev:(id)sender {
    effects_advance(-1);
}
- (IBAction)next:(id)sender {
    effects_advance(1);
}

- (void)didMoveToWindow {
    if (self.window != nil) {
        _lastTickDate = [NSDate date];
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
    if (_lastTickDate == nil) {
        return;
    }
    NSDate *now = [NSDate date];
    NSTimeInterval elapsed = [now timeIntervalSinceDate:_lastTickDate];
    
    uint32_t ticks;
    if (params.next_tick_delay_ms == 0) {
        ticks = 1;
    } else {
        ticks = (uint32_t)floor(elapsed / (params.next_tick_delay_ms / 1000.0));
        if (ticks == 0) {
            return;
        }
    }

    for (; ticks > 0; ticks--) {
        effects_tick(_pixels, &params);
    }
    [self displayPixels];
    _lastTickDate = now;
}

- (void)displayPixels {
    for (NSInteger i = 0; i < kLEDCount; ++i) {
        UIView *led = _lights[i];
        led.backgroundColor = _palette[_pixels[i]];
    }
    _effectLabel.text = [NSString stringWithFormat:@"%02d:%02d", (int)params.effect, (int)params.step];
}

@end

#import "ViewController.h"
#import "ChristmasLightsView.h"

@interface ViewController ()

@end

@implementation ViewController {
    ChristmasLightsView *_lightsView;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor blackColor];
    
    _lightsView = [ChristmasLightsView new];
    _lightsView.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:_lightsView];
    
    [NSLayoutConstraint activateConstraints:@[
        [_lightsView.leftAnchor constraintEqualToAnchor:self.view.leftAnchor constant:10],
        [_lightsView.rightAnchor constraintEqualToAnchor:self.view.rightAnchor constant:-10],
        [_lightsView.topAnchor constraintEqualToAnchor:self.view.topAnchor constant:50],
        [_lightsView.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor constant:-10],
    ]];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}


@end

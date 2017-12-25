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
        [_lightsView.leftAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.leftAnchor constant:10],
        [_lightsView.rightAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.rightAnchor constant:-10],
        [_lightsView.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor constant:10],
        [_lightsView.bottomAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.bottomAnchor constant:-10],
    ]];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}


@end

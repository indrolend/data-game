#import <UIKit/UIKit.h>
#import <MetalKit/MetalKit.h>

#include "Game.hpp"

@interface GameViewController : UIViewController <MTKViewDelegate>
@end

@implementation GameViewController {
    MTKView *_metalView;
    id<MTLCommandQueue> _commandQueue;
    Game _game;
}

- (void)loadView {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    _metalView = [[MTKView alloc] initWithFrame:CGRectZero device:device];

    _metalView.delegate = self;
    _metalView.preferredFramesPerSecond = 60;
    _metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalView.clearColor = MTLClearColorMake(0.015, 0.02, 0.03, 1.0);

    _commandQueue = [device newCommandQueue];

    _game.reset();

    self.view = _metalView;
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    (void)view;
    (void)size;
}

- (void)drawInMTKView:(MTKView *)view {
    _game.update(1.0f / 60.0f);

    MTLRenderPassDescriptor *pass = view.currentRenderPassDescriptor;
    id<CAMetalDrawable> drawable = view.currentDrawable;
    if (!pass || !drawable) return;

    /*
     First bridge from real game state -> iPhone presentation.
     The color changes slightly with simulation time, proving that
     Game::update() is actually running inside the iOS frame loop.
    */
    const GameState& state = _game.state();
    const double pulse = 0.03 + 0.015 * sin(state.time * 2.0);
    pass.colorAttachments[0].clearColor =
        MTLClearColorMake(pulse, 0.02, 0.045, 1.0);

    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    id<MTLRenderCommandEncoder> encoder =
        [commandBuffer renderCommandEncoderWithDescriptor:pass];

    [encoder endEncoding];
    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
}

@end

@interface SceneDelegate : UIResponder <UIWindowSceneDelegate>
@property(nonatomic, strong) UIWindow *window;
@end

@implementation SceneDelegate

- (void)scene:(UIScene *)scene
    willConnectToSession:(UISceneSession *)session
    options:(UISceneConnectionOptions *)connectionOptions {
    (void)session;
    (void)connectionOptions;

    if (![scene isKindOfClass:[UIWindowScene class]]) {
        return;
    }

    UIWindowScene *windowScene = (UIWindowScene *)scene;

    self.window = [[UIWindow alloc] initWithWindowScene:windowScene];
    self.window.rootViewController = [GameViewController new];
    [self.window makeKeyAndVisible];
}

@end

@interface AppDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    (void)application;
    (void)launchOptions;
    return YES;
}

- (UISceneConfiguration *)application:(UIApplication *)application
    configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession
    options:(UISceneConnectionOptions *)options {
    (void)application;
    (void)connectingSceneSession;
    (void)options;

    UISceneConfiguration *configuration =
        [[UISceneConfiguration alloc] initWithName:nil
                                       sessionRole:UIWindowSceneSessionRoleApplication];

    configuration.sceneClass = [UIWindowScene class];
    configuration.delegateClass = [SceneDelegate class];
    return configuration;
}

@end

int main(int argc, char *argv[]) {
    @autoreleasepool {
        return UIApplicationMain(
            argc,
            argv,
            nil,
            NSStringFromClass([AppDelegate class])
        );
    }
}

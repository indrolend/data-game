#import <UIKit/UIKit.h>
#import <MetalKit/MetalKit.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <unordered_map>

#include "DesktopRenderer.hpp"
#include "GLShim.hpp"
#include "Game.hpp"
#include "TouchControls.hpp"
#include "TouchOverlay.hpp"

/*
 Frame path: Game (authoritative state, unchanged) -> DesktopRenderer::draw()
 (the real desktop renderer, compiled against the GL shim in GLShim.cpp) ->
 shim triangle stream -> this Metal layer -> drawable.
 Touch input -> ios_touch::TouchControls -> Game::setTouchControls(), the same
 call the desktop host makes every frame.
*/

static const char kShaderSource[] = R"(
#include <metal_stdlib>
using namespace metal;
struct VIn { float4 pos [[attribute(0)]]; float4 color [[attribute(1)]]; float2 uv [[attribute(2)]]; };
struct VOut { float4 position [[position]]; float4 color; float2 uv; };
vertex VOut shimVertex(VIn in [[stage_in]]) {
    VOut o;
    // GL clip z is [-w, w]; Metal expects [0, w].
    o.position = float4(in.pos.xy, (in.pos.z + in.pos.w) * 0.5, in.pos.w);
    o.color = in.color;
    o.uv = in.uv;
    return o;
}
fragment float4 shimColor(VOut in [[stage_in]]) { return in.color; }
fragment float4 shimTextured(VOut in [[stage_in]], texture2d<float> tex [[texture(0)]], sampler smp [[sampler(0)]]) {
    return tex.sample(smp, in.uv) * in.color;
}
)";

static constexpr float kSimulationStep = 1.0f / 60.0f;

@interface GameViewController : UIViewController <MTKViewDelegate>
@end

@implementation GameViewController {
    MTKView *_metalView;
    id<MTLDevice> _device;
    id<MTLCommandQueue> _commandQueue;
    id<MTLRenderPipelineState> _pipelines[2][2]; // [textured][blended]
    id<MTLDepthStencilState> _depthStates[2][2]; // [depthTest][depthWrite]
    id<MTLSamplerState> _samplers[2];            // [clamp]
    std::unordered_map<std::uint32_t, id<MTLTexture>> _textures;
    std::unordered_map<std::uint32_t, std::uint32_t> _textureVersions;
    std::unordered_map<std::uintptr_t, CGPoint> _menuTouches; // fingers that began on a menu screen

    Game _game;
    DesktopRenderer _renderer;
    ios_touch::TouchControls _controls;
    CFTimeInterval _lastTime;
    double _accumulator;
}

- (BOOL)prefersStatusBarHidden { return YES; }
- (BOOL)prefersHomeIndicatorAutoHidden { return YES; }
- (UIInterfaceOrientationMask)supportedInterfaceOrientations { return UIInterfaceOrientationMaskAllButUpsideDown; }

- (void)loadView {
    _device = MTLCreateSystemDefaultDevice();
    _metalView = [[MTKView alloc] initWithFrame:CGRectZero device:_device];
    _metalView.delegate = self;
    _metalView.preferredFramesPerSecond = 60;
    _metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalView.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    _metalView.clearDepth = 1.0;
    _metalView.clearColor = MTLClearColorMake(0.015, 0.02, 0.03, 1.0);
    _metalView.multipleTouchEnabled = YES;
    _commandQueue = [_device newCommandQueue];
    [self buildPipelines];

    // Shared assets (models, fonts, TV clips) are bundled as assets/{models,fonts,tv-gifs}.
    NSString *assets = [[NSBundle mainBundle].resourcePath stringByAppendingPathComponent:@"assets/models"];
    _renderer.setAssetRoot(std::filesystem::path(assets.UTF8String));

    _game.reset();
    _lastTime = CACurrentMediaTime();
    _accumulator = 0.0;
    self.view = _metalView;

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(releaseInput)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
}

- (void)releaseInput {
    _controls.releaseAll();
    _game.clearInputState();
}

- (void)buildPipelines {
    NSError *error = nil;
    id<MTLLibrary> library = [_device newLibraryWithSource:[NSString stringWithUTF8String:kShaderSource] options:nil error:&error];
    if (!library) { NSLog(@"Shader compile failed: %@", error); return; }

    MTLVertexDescriptor *vd = [MTLVertexDescriptor vertexDescriptor];
    vd.attributes[0].format = MTLVertexFormatFloat4; vd.attributes[0].offset = 0;  vd.attributes[0].bufferIndex = 0;
    vd.attributes[1].format = MTLVertexFormatFloat4; vd.attributes[1].offset = 16; vd.attributes[1].bufferIndex = 0;
    vd.attributes[2].format = MTLVertexFormatFloat2; vd.attributes[2].offset = 32; vd.attributes[2].bufferIndex = 0;
    vd.layouts[0].stride = sizeof(glshim::Vertex);

    for (int textured = 0; textured < 2; ++textured) {
        for (int blended = 0; blended < 2; ++blended) {
            MTLRenderPipelineDescriptor *pd = [MTLRenderPipelineDescriptor new];
            pd.vertexFunction = [library newFunctionWithName:@"shimVertex"];
            pd.fragmentFunction = [library newFunctionWithName:textured ? @"shimTextured" : @"shimColor"];
            pd.vertexDescriptor = vd;
            pd.colorAttachments[0].pixelFormat = _metalView.colorPixelFormat;
            pd.depthAttachmentPixelFormat = _metalView.depthStencilPixelFormat;
            if (blended) {
                MTLRenderPipelineColorAttachmentDescriptor *ca = pd.colorAttachments[0];
                ca.blendingEnabled = YES;
                ca.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
                ca.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
                ca.sourceAlphaBlendFactor = MTLBlendFactorOne;
                ca.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
            }
            _pipelines[textured][blended] = [_device newRenderPipelineStateWithDescriptor:pd error:&error];
            if (!_pipelines[textured][blended]) NSLog(@"Pipeline failed: %@", error);
        }
    }
    for (int test = 0; test < 2; ++test) {
        for (int write = 0; write < 2; ++write) {
            MTLDepthStencilDescriptor *dd = [MTLDepthStencilDescriptor new];
            dd.depthCompareFunction = test ? MTLCompareFunctionLessEqual : MTLCompareFunctionAlways;
            dd.depthWriteEnabled = (test && write) ? YES : NO; // GL ignores depth writes with the test off
            _depthStates[test][write] = [_device newDepthStencilStateWithDescriptor:dd];
        }
    }
    for (int clamp = 0; clamp < 2; ++clamp) {
        MTLSamplerDescriptor *sd = [MTLSamplerDescriptor new];
        sd.minFilter = sd.magFilter = MTLSamplerMinMagFilterLinear;
        sd.sAddressMode = sd.tAddressMode = clamp ? MTLSamplerAddressModeClampToEdge : MTLSamplerAddressModeRepeat;
        _samplers[clamp] = [_device newSamplerStateWithDescriptor:sd];
    }
}

- (id<MTLTexture>)metalTextureFor:(std::uint32_t)texId {
    const glshim::Texture *src = glshim::texture(texId);
    if (!src || !src->valid || src->width <= 0 || src->height <= 0) return nil;
    id<MTLTexture> tex = _textures[texId];
    if (!tex || tex.width != (NSUInteger)src->width || tex.height != (NSUInteger)src->height) {
        MTLTextureDescriptor *td =
            [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                               width:src->width
                                                              height:src->height
                                                           mipmapped:NO];
        tex = [_device newTextureWithDescriptor:td];
        _textures[texId] = tex;
        _textureVersions[texId] = 0;
    }
    if (_textureVersions[texId] != src->version) {
        [tex replaceRegion:MTLRegionMake2D(0, 0, src->width, src->height)
               mipmapLevel:0
                 withBytes:src->rgba.data()
               bytesPerRow:(NSUInteger)src->width * 4];
        _textureVersions[texId] = src->version;
    }
    return tex;
}

- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    const UIEdgeInsets safe = self.view.safeAreaInsets;
    const CGSize size = self.view.bounds.size;
    _controls.setLayout((float)size.width, (float)size.height,
                        {(float)safe.left, (float)safe.top, (float)safe.right, (float)safe.bottom});
}

#pragma mark - Touch input

- (std::uintptr_t)idFor:(UITouch *)touch { return (std::uintptr_t)(__bridge void *)touch; }

- (BOOL)menuActive {
    const GameState &state = _game.state();
    return state.upgradeMenu.active || state.dead || !state.started;
}

// Menu taps act on release so a gameplay finger that is already down when the
// player dies cannot trigger an accidental restart.
- (void)handleMenuTap:(CGPoint)p {
    const GameState &state = _game.state();
    if (state.upgradeMenu.active) {
        // Pick the upgrade track by screen third until a touch-native menu exists.
        const int track = std::min(2, std::max(0, (int)(p.x / (self.view.bounds.size.width / 3.0))));
        _game.chooseTemporaryUpgrade(track);
        return;
    }
    if (state.dead || !state.started) _game.restart();
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        const CGPoint p = [touch locationInView:self.view];
        if ([self menuActive]) {
            _menuTouches[[self idFor:touch]] = p;
            continue;
        }
        _controls.touchBegan([self idFor:touch], (float)p.x, (float)p.y);
    }
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        const CGPoint p = [touch locationInView:self.view];
        _controls.touchMoved([self idFor:touch], (float)p.x, (float)p.y);
    }
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        const std::uintptr_t touchId = [self idFor:touch];
        _controls.touchEnded(touchId);
        const auto menu = _menuTouches.find(touchId);
        if (menu != _menuTouches.end()) {
            const CGPoint began = menu->second;
            _menuTouches.erase(menu);
            if ([self menuActive]) [self handleMenuTap:began];
        }
    }
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        const std::uintptr_t touchId = [self idFor:touch];
        _controls.touchEnded(touchId);
        _menuTouches.erase(touchId);
    }
}

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    (void)view;
    _renderer.resize((int)size.width, (int)size.height);
}

- (void)stepSimulation {
    const CFTimeInterval now = CACurrentMediaTime();
    _accumulator += std::min(0.1, now - _lastTime);
    _lastTime = now;
    // Held controls are re-asserted for every simulation step; one-shot edges and
    // look deltas are delivered only with the first step of the frame.
    ios_touch::Output out = _controls.poll();
    bool first = true;
    while (_accumulator >= kSimulationStep) {
        const float scale = _game.state().localSettings.mouseLookSensitivity;
        _game.setTouchControls(out.moveX, out.moveZ,
                               first ? out.lookX * scale : 0.0f, first ? out.lookY * scale : 0.0f,
                               out.vacuumHeld, out.sprintHeld,
                               first && out.jumpPressed, first && out.meleePressed,
                               first && out.shootPressed, first && out.cameraPressed);
        _game.update(kSimulationStep);
        _accumulator -= kSimulationStep;
        first = false;
    }
    if (first) {
        // No step this frame: keep edges and look deltas for the next one.
        _game.setTouchControls(out.moveX, out.moveZ, out.lookX, out.lookY, out.vacuumHeld, out.sprintHeld,
                               out.jumpPressed, out.meleePressed, out.shootPressed, out.cameraPressed);
    }
}

- (void)drawInMTKView:(MTKView *)view {
    [self stepSimulation];

    MTLRenderPassDescriptor *pass = view.currentRenderPassDescriptor;
    id<CAMetalDrawable> drawable = view.currentDrawable;
    if (!pass || !drawable) return;

    const CGSize drawableSize = view.drawableSize;
    const int pixelWidth = std::max(1, (int)drawableSize.width);
    const int pixelHeight = std::max(1, (int)drawableSize.height);
    _renderer.resize(pixelWidth, pixelHeight);

    glshim::beginFrame(pixelWidth, pixelHeight);
    _renderer.draw(_game.state());
    ios_touch::drawOverlay(_controls);
    const glshim::Frame &frame = glshim::frame();

    pass.colorAttachments[0].clearColor =
        MTLClearColorMake(frame.clear[0], frame.clear[1], frame.clear[2], 1.0);

    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:pass];
    [encoder setFrontFacingWinding:MTLWindingCounterClockwise];

    if (!frame.vertices.empty()) {
        id<MTLBuffer> buffer = [_device newBufferWithBytes:frame.vertices.data()
                                                    length:frame.vertices.size() * sizeof(glshim::Vertex)
                                                   options:MTLResourceStorageModeShared];
        [encoder setVertexBuffer:buffer offset:0 atIndex:0];
        for (const glshim::Batch &batch : frame.batches) {
            id<MTLTexture> tex = batch.texture ? [self metalTextureFor:batch.texture] : nil;
            if (batch.texture && !tex) continue;
            [encoder setRenderPipelineState:_pipelines[tex ? 1 : 0][batch.blend ? 1 : 0]];
            [encoder setDepthStencilState:_depthStates[batch.depthTest ? 1 : 0][batch.depthWrite ? 1 : 0]];
            [encoder setCullMode:batch.cull ? MTLCullModeBack : MTLCullModeNone];
            if (tex) {
                const glshim::Texture *src = glshim::texture(batch.texture);
                [encoder setFragmentTexture:tex atIndex:0];
                [encoder setFragmentSamplerState:_samplers[src->clamp ? 1 : 0] atIndex:0];
            }
            [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:batch.first vertexCount:batch.count];
        }
    }

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

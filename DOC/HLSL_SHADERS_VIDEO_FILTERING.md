# HLSL Shaders and Video Filtering for PS1 Emulation on Xbox 360

## Table of Contents

1. [Overview](#overview)
2. [HLSL Shader Pipeline for Xbox 360 Xenos GPU](#hlsl-shader-pipeline)
3. [Common Shader Types](#common-shader-types)
4. [Video Filtering Techniques](#video-filtering-techniques)
5. [Shader Filters for PS1 Emulators](#shader-filters)
6. [Shader Performance on Xbox 360](#shader-performance)
7. [Direct3D 9 Integration](#direct3d-9-integration)
8. [Sample HLSL Code](#sample-hlsl-code)
9. [Performance Considerations](#performance-considerations)

---

## Overview {#overview}

High-Level Shading Language (HLSL) shaders play a crucial role in modern PS1 emulation, enabling real-time video filtering and enhancement of the original 320x240 (or 256x224) resolution output to modern displays. The Xbox 360's Xenos GPU provides powerful shader capabilities that can be leveraged to apply sophisticated visual effects while maintaining performance.

For PCSX-R Xbox 360, HLSL shaders are used to:
- Scale low-resolution PS1 video output to HD resolutions (720p, 1080p)
- Apply CRT emulation effects for authentic retro aesthetics
- Implement advanced scaling algorithms for pixel-art preservation
- Provide real-time post-processing effects

---

## HLSL Shader Pipeline for Xbox 360 Xenos GPU {#hlsl-shader-pipeline}

### Xenos GPU Architecture

The Xbox 360's Xenos GPU (ATI/AMD design) features:

- **Unified Shader Architecture**: 48 shader units capable of vertex, pixel, and geometry processing
- **Shader Model 3.0+ Support**: Advanced instruction sets and flow control
- **10 MB Embedded Frame Buffer (eDRAM)**: Ultra-fast local memory for render targets
- **128-bit Memory Interface**: High bandwidth for texture sampling

### Pipeline Stages

```
PS1 Framebuffer (320x240/256x224)
         ↓
   [Vertex Shader]
   - Transform vertices
   - Set up UV coordinates
         ↓
   [Rasterizer]
         ↓
   [Pixel Shader]
   - Texture sampling
   - Filter application
   - Color processing
         ↓
   [Output Merger]
   - Blend with framebuffer
         ↓
   Xbox 360 Display (720p/1080p)
```

### Shader Compilation

Xbox 360 uses a modified HLSL compiler that targets the Xenos microcode:

```cpp
// Typical shader compilation flow
ID3DXBuffer* pCode = NULL;
ID3DXBuffer* pErrors = NULL;

HRESULT hr = D3DXCompileShader(
    pShaderCode,           // HLSL source
    strlen(pShaderCode),   // Source length
    NULL,                  // Macros
    NULL,                  // Include handler
    "main",                // Entry point
    "ps_3_0",              // Shader profile
    0,                     // Flags
    &pCode,                // Compiled code
    &pErrors,              // Error messages
    &pConstantTable        // Constant table
);

// Create pixel shader on Xenos device
g_pd3dDevice->CreatePixelShader((DWORD*)pCode->GetBufferPointer(), &g_pPixelShader);
```

---

## Common Shader Types {#common-shader-types}

### Vertex Shaders

Vertex shaders transform 3D geometry and pass interpolated values to pixel shaders:

```hlsl
// Basic vertex shader for full-screen quad rendering
struct VS_INPUT {
    float4 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct VS_OUTPUT {
    float4 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;
    output.Position = input.Position;
    output.TexCoord = input.TexCoord;
    return output;
}
```

### Pixel Shaders

Pixel shaders perform per-pixel operations including texture sampling and color manipulation:

```hlsl
// Basic pixel shader structure
sampler2D Texture : register(s0);

float4 main(float2 texCoord : TEXCOORD0) : COLOR {
    return tex2D(Texture, texCoord);
}
```

### Compute Shaders (Limited on Xbox 360)

While Xbox 360 predates full compute shader support, vertex texture fetch and render-to-vertex-buffer techniques can achieve similar results.

---

## Video Filtering Techniques {#video-filtering-techniques}

### 1. Scanline Emulation

Scanlines simulate the appearance of CRT displays by darkening alternate horizontal lines:

```hlsl
// Basic scanline effect
float4 ScanlinePS(float2 texCoord : TEXCOORD0, uniform float intensity) : COLOR {
    float4 color = tex2D(Texture, texCoord);
    
    // Calculate scanline mask
    float scanline = frac(texCoord.y * ScreenHeight);
    float mask = (scanline < 0.5) ? 1.0 : (1.0 - intensity);
    
    return color * mask;
}
```

### 2. CRT Effects

CRT shaders simulate various aspects of cathode ray tube displays:

- **Curvature**: Barrel distortion of the screen edges
- **Phosphor glow**: RGB subpixel structure simulation
- **Bloom**: Light bleeding and glow effects
- **Vignetting**: Darkening at screen corners

### 3. Scaling Algorithms

Different approaches for upscaling pixel art:

| Algorithm | Description | Best For |
|-----------|-------------|----------|
| Nearest Neighbor | No interpolation, sharp pixels | Pixel preservation |
| Bilinear | Linear interpolation | Speed, smooth images |
| Bicubic | Higher-order interpolation | Quality scaling |
| Lanczos | Sinc-based resampling | High-quality upscaling |
| xBR/xBRZ | Edge-directed scaling | Pixel art |
| HQx | Pattern recognition | Sprite graphics |

### 4. Signal Processing Effects

- **NTSC/PAL artifacts**: Color bleeding, dot crawl
- **Composite video simulation**: Signal degradation
- **RF modulator noise**: For extreme authenticity

---

## Shader Filters for PS1 Emulators {#shader-filters}

### xBR and xBRZ Scaling Filters

xBR (Scale By Rules) algorithms use pattern recognition to intelligently scale pixel art while preserving edges.

#### xBRZ Filter (4x version)

```hlsl
// Simplified xBRZ 4x shader
sampler2D SourceTexture : register(s0);
float2 TextureSize : register(c0);

// Edge detection matrix
static const float3x3 yuv = float3x3(
    0.299, 0.587, 0.114,
    -0.169, -0.331, 0.5,
    0.5, -0.419, -0.081
);

float4 xBRZ4xPS(float2 texCoord : TEXCOORD0) : COLOR {
    float2 ps = 1.0 / TextureSize;
    float2 pos = frac(texCoord * TextureSize) - float2(0.5, 0.5);
    float2 coord = texCoord - pos * ps;
    
    // Sample 3x3 neighborhood
    float3 s00 = tex2D(SourceTexture, coord + float2(-ps.x, -ps.y)).rgb;
    float3 s01 = tex2D(SourceTexture, coord + float2(0, -ps.y)).rgb;
    float3 s02 = tex2D(SourceTexture, coord + float2(ps.x, -ps.y)).rgb;
    float3 s10 = tex2D(SourceTexture, coord + float2(-ps.x, 0)).rgb;
    float3 s11 = tex2D(SourceTexture, coord).rgb;
    float3 s12 = tex2D(SourceTexture, coord + float2(ps.x, 0)).rgb;
    float3 s20 = tex2D(SourceTexture, coord + float2(-ps.x, ps.y)).rgb;
    float3 s21 = tex2D(SourceTexture, coord + float2(0, ps.y)).rgb;
    float3 s22 = tex2D(SourceTexture, coord + float2(ps.x, ps.y)).rgb;
    
    // Convert to YUV for edge detection
    float y00 = dot(s00, yuv[0]);
    float y01 = dot(s01, yuv[0]);
    float y10 = dot(s10, yuv[0]);
    float y11 = dot(s11, yuv[0]);
    
    // Edge detection and interpolation
    float4 color = float4(s11, 1.0);
    
    // Pattern matching logic (simplified)
    float edgeH = abs(y01 - y21);
    float edgeV = abs(y10 - y12);
    
    if (edgeH > edgeV) {
        // Horizontal edge - interpolate vertically
        color.rgb = lerp(s01, s21, pos.y + 0.5);
    } else if (edgeV > edgeH) {
        // Vertical edge - interpolate horizontally
        color.rgb = lerp(s10, s12, pos.x + 0.5);
    }
    
    return color;
}
```

### CRT Shaders

#### Caligari CRT Shader

The Caligari shader simulates CRT phosphor structure and curvature:

```hlsl
// Caligari-style CRT shader
sampler2D Texture : register(s0);
float2 ScreenSize : register(c0);
float2 TextureSize : register(c1);

// CRT parameters
float Curvature : register(c2);      // Screen curvature amount
float ScanlineIntensity : register(c3);  // Scanline darkness
float PhosphorGlow : register(c4);   // Phosphor glow amount

float4 CRTCaligariPS(float2 texCoord : TEXCOORD0) : COLOR {
    float2 curvature = float2(Curvature, Curvature * 0.75);
    
    // Apply barrel distortion
    float2 centered = texCoord - 0.5;
    float2 distorted = centered * (1.0 + curvature * dot(centered, centered));
    float2 finalCoord = distorted + 0.5;
    
    // Discard pixels outside curved screen
    if (finalCoord.x < 0.0 || finalCoord.x > 1.0 ||
        finalCoord.y < 0.0 || finalCoord.y > 1.0) {
        return float4(0.0, 0.0, 0.0, 1.0);
    }
    
    // Sample with subpixel offset for phosphor simulation
    float3 color = 0.0;
    float2 subpixelSize = 1.0 / (TextureSize * 3.0); // RGB triad
    
    // Red phosphor
    color.r = tex2D(Texture, finalCoord + float2(-subpixelSize.x, 0.0)).r;
    // Green phosphor (center)
    color.g = tex2D(Texture, finalCoord).g;
    // Blue phosphor
    color.b = tex2D(Texture, finalCoord + float2(subpixelSize.x, 0.0)).b;
    
    // Apply scanlines
    float scanline = sin(finalCoord.y * TextureSize.y * 3.14159);
    float scanlineMask = lerp(1.0, 0.5 + 0.5 * scanline, ScanlineIntensity);
    
    // Vignetting
    float vignette = 1.0 - curvature * dot(centered, centered);
    
    return float4(color * scanlineMask * vignette, 1.0);
}
```

#### CGWG CRT Shader

The famous cgwg CRT shader focuses on accurate phosphor simulation:

```hlsl
// CGWG CRT shader (simplified version)
sampler2D Texture : register(s0);
float2 TextureSize : register(c0);
float2 OutputSize : register(c1);

// Parameters
float SHARPNESS : register(c2);      // Filter sharpness
float COLOR_BOOST : register(c3);    // Color saturation boost
float InputGamma : register(c4);     // Input gamma correction
float OutputGamma : register(c5);    // Output gamma correction

// Phosphor response lookup
static const float3x3 phosphor_rgb = float3x3(
    1.0, 0.0, 0.0,
    0.0, 1.0, 0.0,
    0.0, 0.0, 1.0
);

float3 ToLinear(float3 color, float gamma) {
    return pow(color, gamma);
}

float3 ToSrgb(float3 color, float gamma) {
    return pow(color, 1.0 / gamma);
}

float4 CGWGCRTPS(float2 texCoord : TEXCOORD0) : COLOR {
    float2 pixelCoord = texCoord * TextureSize;
    float2 texelPos = frac(pixelCoord);
    float2 texelCenter = (floor(pixelCoord) + 0.5) / TextureSize;
    
    // Gaussian-ish filter kernel
    float2 dist = texelPos - 0.5;
    float2 offset = dist * SHARPNESS;
    
    // 4-tap bicubic-like filter
    float2 weights = 0.5 + 0.5 * cos(3.14159 * offset);
    float2 w1 = weights;
    float2 w2 = 1.0 - weights;
    
    float2 coord1 = texelCenter - 0.5 / TextureSize;
    float2 coord2 = texelCenter + 0.5 / TextureSize;
    
    // Sample 4 pixels
    float3 c00 = tex2D(Texture, float2(coord1.x, coord1.y)).rgb;
    float3 c01 = tex2D(Texture, float2(coord2.x, coord1.y)).rgb;
    float3 c10 = tex2D(Texture, float2(coord1.x, coord2.y)).rgb;
    float3 c11 = tex2D(Texture, float2(coord2.x, coord2.y)).rgb;
    
    // Interpolate
    float3 color = lerp(
        lerp(c00, c01, w1.x),
        lerp(c10, c11, w1.x),
        w1.y
    );
    
    // Gamma correction
    color = ToLinear(color, InputGamma);
    
    // Color boost
    float lum = dot(color, float3(0.299, 0.587, 0.114));
    color = lerp(float3(lum, lum, lum), color, COLOR_BOOST);
    
    // Output gamma
    color = ToSrgb(color, OutputGamma);
    
    // Scanline simulation based on output resolution
    float scanline = frac(texCoord.y * OutputSize.y);
    float scanlineMask = 0.65 + 0.35 * cos(scanline * 2.0 * 3.14159);
    
    return float4(color * scanlineMask, 1.0);
}
```

#### Phosphor Shader

Pure phosphor glow simulation without scanlines:

```hlsl
// Phosphor glow shader
sampler2D Texture : register(s0);
float2 TextureSize : register(c0);
float PhosphorPersistence : register(c1);  // How long phosphors glow

float4 PhosphorPS(float2 texCoord : TEXCOORD0) : COLOR {
    float2 ps = 1.0 / TextureSize;
    
    // Sample center and neighbors
    float3 center = tex2D(Texture, texCoord).rgb;
    float3 left = tex2D(Texture, texCoord - float2(ps.x, 0.0)).rgb;
    float3 right = tex2D(Texture, texCoord + float2(ps.x, 0.0)).rgb;
    float3 up = tex2D(Texture, texCoord - float2(0.0, ps.y)).rgb;
    float3 down = tex2D(Texture, texCoord + float2(0.0, ps.y)).rgb;
    
    // Different phosphor colors persist differently
    // Red phosphors fade faster than green, blue fastest
    float3 persistence = float3(0.8, 0.9, 0.7) * PhosphorPersistence;
    
    // Simulate afterglow from neighboring pixels
    float3 glow = (left * persistence + right * persistence + 
                   up * persistence + down * persistence) * 0.25;
    
    // Add glow to current pixel
    float3 color = center + glow * 0.3;
    
    // Phosphor bloom
    color = 1.0 - exp(-color * 1.5);
    
    return float4(saturate(color), 1.0);
}
```

### Scanline Shaders

#### Basic Scanlines

```hlsl
// Simple alternating scanline shader
sampler2D Texture : register(s0);
float2 TextureSize : register(c0);
float ScanlineAlpha : register(c1);
float ScanlineDepth : register(c2);

float4 SimpleScanlinePS(float2 texCoord : TEXCOORD0) : COLOR {
    float4 color = tex2D(Texture, texCoord);
    
    // Calculate scanline position
    float scanlinePos = frac(texCoord.y * TextureSize.y);
    
    // Alternating scanline mask
    float mask = (scanlinePos < 0.5) ? 1.0 : (1.0 - ScanlineAlpha);
    
    // Add slight depth variation
    float depth = 1.0 - ScanlineDepth * abs(scanlinePos - 0.5) * 2.0;
    
    return color * mask * depth;
}
```

#### RGB Scanlines (Subpixel Stripes)

```hlsl
// RGB subpixel scanline simulation
sampler2D Texture : register(s0);
float2 TextureSize : register(c0);

float4 RGBScanlinePS(float2 texCoord : TEXCOORD0) : COLOR {
    float2 ps = 1.0 / (TextureSize * 3.0); // Subpixel resolution
    float3 color = 0.0;
    
    // Offset for RGB subpixels within each pixel
    float2 baseCoord = texCoord;
    
    // Sample red subpixel
    color.r = tex2D(Texture, baseCoord + float2(-ps.x, 0.0)).r;
    
    // Sample green subpixel (center)
    color.g = tex2D(Texture, baseCoord).g;
    
    // Sample blue subpixel
    color.b = tex2D(Texture, baseCoord + float2(ps.x, 0.0)).b;
    
    // Scanline gaps between rows
    float row = frac(texCoord.y * TextureSize.y);
    float gap = (row > 0.85) ? 0.3 : 1.0;
    
    return float4(color * gap, 1.0);
}
```

### Bicubic and Lanczos Scaling

#### Bicubic Scaling Shader

```hlsl
// Bicubic scaling implementation
sampler2D Texture : register(s0);
float2 TextureSize : register(c0);

// Bicubic weight function
float Weight(float x) {
    float a = -0.75; // Catmull-Rom parameter
    
    x = abs(x);
    
    if (x < 1.0) {
        return ((a + 2.0) * x - (a + 3.0)) * x * x + 1.0;
    } else if (x < 2.0) {
        return (((x - 5.0) * x + 8.0) * x - 4.0) * a;
    }
    
    return 0.0;
}

float4 BicubicPS(float2 texCoord : TEXCOORD0) : COLOR {
    float2 pixelCoord = texCoord * TextureSize;
    float2 texelPos = frac(pixelCoord);
    float2 texelCenter = floor(pixelCoord);
    
    float2 ps = 1.0 / TextureSize;
    
    float4 color = 0.0;
    float weightSum = 0.0;
    
    // Sample 4x4 neighborhood
    for (int y = -1; y <= 2; y++) {
        for (int x = -1; x <= 2; x++) {
            float2 samplePos = (texelCenter + float2(x, y)) * ps;
            float weight = Weight(texelPos.x - x) * Weight(texelPos.y - y);
            
            color += tex2D(Texture, samplePos) * weight;
            weightSum += weight;
        }
    }
    
    return color / weightSum;
}
```

#### Lanczos Scaling Shader

```hlsl
// Lanczos resampling shader
sampler2D Texture : register(s0);
float2 TextureSize : register(c0);
int LanczosTaps : register(c1);  // Typically 2 or 3

// Lanczos kernel
float Lanczos(float x, float a) {
    if (x == 0.0) return 1.0;
    if (abs(x) >= a) return 0.0;
    
    float pi_x = 3.14159 * x;
    return a * sin(pi_x) * sin(pi_x / a) / (pi_x * pi_x);
}

float4 LanczosPS(float2 texCoord : TEXCOORD0) : COLOR {
    float2 pixelCoord = texCoord * TextureSize;
    float2 texelPos = frac(pixelCoord);
    float2 texelCenter = floor(pixelCoord);
    
    float2 ps = 1.0 / TextureSize;
    float a = float(LanczosTaps);
    
    float4 color = 0.0;
    float weightSum = 0.0;
    
    // Sample kernel-sized neighborhood
    int range = LanczosTaps;
    for (int y = -range + 1; y <= range; y++) {
        for (int x = -range + 1; x <= range; x++) {
            float2 samplePos = (texelCenter + float2(x, y)) * ps;
            float weight = Lanczos(texelPos.x - x, a) * Lanczos(texelPos.y - y, a);
            
            color += tex2D(Texture, samplePos) * weight;
            weightSum += weight;
        }
    }
    
    return color / weightSum;
}
```

### HQx Filters

#### HQ2x Shader

HQx filters use pattern matching to smooth jagged edges in pixel art:

```hlsl
// HQ2x edge smoothing shader
sampler2D Texture : register(s0);
float2 TextureSize : register(c1);

// Threshold for color difference
float Threshold : register(c2);

// Compare two colors
bool Diff(float3 c1, float3 c2) {
    return length(c1 - c2) > Threshold;
}

float4 HQ2xPS(float2 texCoord : TEXCOORD0) : COLOR {
    float2 ps = 1.0 / TextureSize;
    
    // Get 3x3 neighborhood
    float3 s00 = tex2D(Texture, texCoord + float2(-ps.x, -ps.y)).rgb;
    float3 s01 = tex2D(Texture, texCoord + float2(0, -ps.y)).rgb;
    float3 s02 = tex2D(Texture, texCoord + float2(ps.x, -ps.y)).rgb;
    float3 s10 = tex2D(Texture, texCoord + float2(-ps.x, 0)).rgb;
    float3 s11 = tex2D(Texture, texCoord).rgb;
    float3 s12 = tex2D(Texture, texCoord + float2(ps.x, 0)).rgb;
    float3 s20 = tex2D(Texture, texCoord + float2(-ps.x, ps.y)).rgb;
    float3 s21 = tex2D(Texture, texCoord + float2(0, ps.y)).rgb;
    float3 s22 = tex2D(Texture, texCoord + float2(ps.x, ps.y)).rgb;
    
    // Pattern detection (simplified)
    bool diff01 = Diff(s00, s01) || Diff(s01, s02) || Diff(s10, s11) || Diff(s11, s12);
    bool diff10 = Diff(s00, s10) || Diff(s10, s20) || Diff(s01, s11) || Diff(s11, s21);
    bool diff12 = Diff(s02, s12) || Diff(s12, s22) || Diff(s01, s11) || Diff(s11, s21);
    bool diff21 = Diff(s20, s21) || Diff(s21, s22) || Diff(s10, s11) || Diff(s11, s12);
    
    float2 pos = frac(texCoord * TextureSize);
    float3 result = s11;
    
    // Interpolate based on pattern
    if (!diff01 && !diff10 && diff12 && diff21) {
        // Top-left corner
        result = (pos.x < 0.5 && pos.y < 0.5) ? lerp(s11, (s01 + s10) * 0.5, 0.5) : s11;
    } else if (!diff01 && !diff12 && diff10 && diff21) {
        // Top-right corner
        result = (pos.x >= 0.5 && pos.y < 0.5) ? lerp(s11, (s01 + s12) * 0.5, 0.5) : s11;
    } else if (!diff10 && !diff21 && diff01 && diff12) {
        // Bottom-left corner
        result = (pos.x < 0.5 && pos.y >= 0.5) ? lerp(s11, (s21 + s10) * 0.5, 0.5) : s11;
    } else if (!diff12 && !diff21 && diff01 && diff10) {
        // Bottom-right corner
        result = (pos.x >= 0.5 && pos.y >= 0.5) ? lerp(s11, (s21 + s12) * 0.5, 0.5) : s11;
    }
    
    return float4(result, 1.0);
}
```

### SuperEagle Filter

```hlsl
// SuperEagle 2x scaling filter
sampler2D Texture : register(s0);
float2 TextureSize : register(c1);

float4 SuperEaglePS(float2 texCoord : TEXCOORD0) : COLOR {
    float2 ps = 1.0 / TextureSize;
    float2 fp = frac(texCoord * TextureSize);
    float2 texel = texCoord - fp * ps;
    
    // Sample 3x3 pattern
    float3 b = tex2D(Texture, texel + float2(0.0, -ps.y)).rgb;
    float3 d = tex2D(Texture, texel + float2(-ps.x, 0.0)).rgb;
    float3 e = tex2D(Texture, texel).rgb;
    float3 f = tex2D(Texture, texel + float2(ps.x, 0.0)).rgb;
    float3 h = tex2D(Texture, texel + float2(0.0, ps.y)).rgb;
    
    // Color comparison
    float db = length(d - b);
    float df = length(d - f);
    float hb = length(h - b);
    float hf = length(h - f);
    
    float3 result = e;
    
    // Pattern matching for edge smoothing
    if (db < df && hb < hf) {
        // Diagonal edge detected
        if (fp.x + fp.y < 1.0) {
            result = lerp(e, b, 0.5);
        } else {
            result = lerp(e, h, 0.5);
        }
    } else if (db >= df && hb >= hf) {
        // Other diagonal
        if (fp.x - fp.y < 0.0) {
            result = lerp(e, d, 0.5);
        } else {
            result = lerp(e, f, 0.5);
        }
    }
    
    return float4(result, 1.0);
}
```

### 2xSaI Filter

```hlsl
// 2xSaI scaling algorithm
sampler2D Texture : register(s0);
float2 TextureSize : register(c1);

float4 Scale2xSaiPS(float2 texCoord : TEXCOORD0) : COLOR {
    float2 ps = 1.0 / TextureSize;
    float2 fp = frac(texCoord * TextureSize);
    float2 texel = texCoord - fp * ps;
    
    // Sample 3x3 pattern
    float3 a = tex2D(Texture, texel + float2(-ps.x, -ps.y)).rgb;
    float3 b = tex2D(Texture, texel + float2(0.0, -ps.y)).rgb;
    float3 c = tex2D(Texture, texel + float2(ps.x, -ps.y)).rgb;
    float3 d = tex2D(Texture, texel + float2(-ps.x, 0.0)).rgb;
    float3 e = tex2D(Texture, texel).rgb;
    float3 f = tex2D(Texture, texel + float2(ps.x, 0.0)).rgb;
    float3 g = tex2D(Texture, texel + float2(-ps.x, ps.y)).rgb;
    float3 h = tex2D(Texture, texel + float2(0.0, ps.y)).rgb;
    float3 i = tex2D(Texture, texel + float2(ps.x, ps.y)).rgb;
    
    float3 result = e;
    
    // 2xSaI pattern logic (simplified)
    float abd = length(a - b) + length(a - d) + length(b - d);
    float bcf = length(b - c) + length(b - f) + length(c - f);
    float dgh = length(d - g) + length(d - h) + length(g - h);
    float fhi = length(f - h) + length(f - i) + length(h - i);
    
    if (abd < bcf && abd < dgh && abd < fhi) {
        result = lerp(e, (a + b + d) / 3.0, 0.5);
    } else if (bcf < abd && bcf < dgh && bcf < fhi) {
        result = lerp(e, (b + c + f) / 3.0, 0.5);
    } else if (dgh < abd && dgh < bcf && dgh < fhi) {
        result = lerp(e, (d + g + h) / 3.0, 0.5);
    } else if (fhi < abd && fhi < bcf && fhi < dgh) {
        result = lerp(e, (f + h + i) / 3.0, 0.5);
    }
    
    return float4(result, 1.0);
}
```

### Artistic Filters

#### Waterpaint Effect

```hlsl
// Waterpaint/oil painting effect
sampler2D Texture : register(s0);
float2 TextureSize : register(c0);
float FilterRadius : register(c1);

float4 WaterpaintPS(float2 texCoord : TEXCOORD0) : COLOR {
    float2 ps = 1.0 / TextureSize;
    float radius = FilterRadius;
    
    float3 sum = 0.0;
    float3 sum2 = 0.0;
    float count = 0.0;
    
    // Sample neighborhood
    for (float y = -radius; y <= radius; y += 1.0) {
        for (float x = -radius; x <= radius; x += 1.0) {
            float2 offset = float2(x, y) * ps;
            float3 color = tex2D(Texture, texCoord + offset).rgb;
            
            sum += color;
            sum2 += color * color;
            count += 1.0;
        }
    }
    
    // Calculate variance and apply smoothing
    float3 mean = sum / count;
    float3 variance = (sum2 / count) - (mean * mean);
    float3 stddev = sqrt(variance);
    
    // Quantize to create waterpaint effect
    float3 quantized = floor(mean * 8.0) / 8.0;
    float3 result = lerp(mean, quantized, 0.7);
    
    // Add slight edge enhancement
    float3 center = tex2D(Texture, texCoord).rgb;
    float edge = length(center - mean);
    result -= edge * 0.1;
    
    return float4(saturate(result), 1.0);
}
```

#### Mudlord Effects

Mudlord's shaders include various retro effects:

```hlsl
// Mudlord-style NTSC composite simulation
sampler2D Texture : register(s0);
float2 TextureSize : register(c0);
float Time : register(c1);  // For animated noise

// Noise function
float Noise(float2 p) {
    return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453);
}

float4 MudlordNTSCPS(float2 texCoord : TEXCOORD0) : COLOR {
    float3 color = tex2D(Texture, texCoord).rgb;
    
    // Chromatic aberration (channel separation)
    float aberration = 0.003;
    float2 ps = 1.0 / TextureSize;
    
    float3 shifted;
    shifted.r = tex2D(Texture, texCoord + float2(aberration, 0.0)).r;
    shifted.g = tex2D(Texture, texCoord).g;
    shifted.b = tex2D(Texture, texCoord - float2(aberration, 0.0)).b;
    
    // Composite artifacting (color bleeding)
    float bleed = 0.0;
    for (int i = -4; i <= 4; i++) {
        bleed += tex2D(Texture, texCoord + float2(i * ps.x, 0.0)).r;
    }
    bleed /= 9.0;
    shifted.r = lerp(shifted.r, bleed, 0.3);
    
    // Add noise
    float noise = Noise(texCoord * TextureSize + Time) * 0.05;
    shifted += noise;
    
    // Color bleeding between lines (Y/C separation)
    float y = dot(shifted, float3(0.299, 0.587, 0.114));
    float2 c = float2(
        dot(shifted, float3(0.596, -0.275, -0.321)),
        dot(shifted, float3(0.212, -0.523, 0.311))
    );
    
    // Blur chroma
    c *= 0.7;
    
    // Convert back to RGB
    float3 final;
    final.r = y + 1.403 * c.y;
    final.g = y - 0.344 * c.x - 0.714 * c.y;
    final.b = y + 1.773 * c.x;
    
    return float4(saturate(final), 1.0);
}
```

---

## Shader Performance on Xbox 360 {#shader-performance}

### Xenos GPU Capabilities

The Xbox 360 Xenos GPU can handle PS1 emulation shaders efficiently:

| Operation | Performance | Notes |
|-----------|-------------|-------|
| Simple 2x scaling | ~2000 FPS | Minimal shader instructions |
| xBRZ 4x | ~500 FPS | Moderate complexity |
| Full CRT (cgwg) | ~300 FPS | Heavy texture sampling |
| Multi-pass chain | ~120 FPS | Depends on pass count |

### Performance Factors

1. **Texture Sampling**: Each texture sample costs ~4-8 GPU cycles
2. **Instruction Count**: Pixel shaders limited to 1024 instructions
3. **Register Pressure**: More temporaries = lower occupancy
4. **Branching**: Avoid dynamic branching in pixel shaders
5. **Precision**: Half-precision (`half`/`min16float`) when possible

### Optimization Strategies

```hlsl
// BAD: Excessive texture samples in loop
for (int i = 0; i < 16; i++) {
    color += tex2D(Texture, coord + offsets[i]);
}

// GOOD: Unroll small loops, use bilinear filtering where possible
// Reduce samples by using larger steps with filtering
```

```hlsl
// BAD: Complex branching per pixel
if (condition) {
    // 100 instructions
} else {
    // 100 different instructions
}

// GOOD: Predicated execution or separate shaders
// Compile multiple shader variants
```

---

## Direct3D 9 Integration {#direct3d-9-integration}

### Xbox 360 D3D9 Setup

The Xbox 360 uses a modified Direct3D 9 API:

```cpp
// Initialize D3D on Xbox 360
IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);

D3DPRESENT_PARAMETERS d3dpp;
ZeroMemory(&d3dpp, sizeof(d3dpp));
d3dpp.BackBufferWidth = 1280;
d3dpp.BackBufferHeight = 720;
d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
d3dpp.BackBufferCount = 1;
d3dpp.EnableAutoDepthStencil = FALSE;
d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

IDirect3DDevice9* pDevice;
pD3D->CreateDevice(0, D3DDEVTYPE_HAL, NULL, 
                   D3DCREATE_HARDWARE_VERTEXPROCESSING,
                   &d3dpp, &pDevice);
```

### Render-to-Texture Pipeline

```cpp
// Create render target for PS1 framebuffer
IDirect3DTexture9* pFrameTexture;
pDevice->CreateTexture(1024, 512, 1, D3DUSAGE_RENDERTARGET,
                       D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, 
                       &pFrameTexture, NULL);

IDirect3DSurface9* pFrameSurface;
pFrameTexture->GetSurfaceLevel(0, &pFrameSurface);

// Render PS1 output
pDevice->SetRenderTarget(0, pFrameSurface);
RenderPS1Frame();

// Apply shaders
pDevice->SetRenderTarget(0, pBackBuffer);
ApplyShaderChain(pFrameTexture);
```

### Shader Loading and Application

```cpp
class ShaderFilter {
private:
    IDirect3DDevice9* m_pDevice;
    IDirect3DPixelShader9* m_pPixelShader;
    IDirect3DVertexShader9* m_pVertexShader;
    IDirect3DVertexBuffer9* m_pQuadVB;
    ID3DXConstantTable* m_pConstants;
    
public:
    bool LoadShader(const char* hlslCode, const char* entryPoint) {
        ID3DXBuffer* pCode = NULL;
        ID3DXBuffer* pErrors = NULL;
        
        HRESULT hr = D3DXCompileShader(
            hlslCode, strlen(hlslCode),
            NULL, NULL, entryPoint, "ps_3_0",
            0, &pCode, &pErrors, &m_pConstants
        );
        
        if (FAILED(hr)) {
            OutputDebugString((char*)pErrors->GetBufferPointer());
            return false;
        }
        
        m_pDevice->CreatePixelShader(
            (DWORD*)pCode->GetBufferPointer(), 
            &m_pPixelShader
        );
        
        return true;
    }
    
    void Apply(IDirect3DTexture9* pSource, IDirect3DSurface9* pDest) {
        // Set render target
        m_pDevice->SetRenderTarget(0, pDest);
        
        // Set shaders
        m_pDevice->SetPixelShader(m_pPixelShader);
        m_pDevice->SetVertexShader(m_pVertexShader);
        
        // Set constants
        D3DXHANDLE hTexSize = m_pConstants->GetConstantByName(NULL, "TextureSize");
        if (hTexSize) {
            float texSize[2] = {1024.0f, 512.0f};
            m_pConstants->SetFloatArray(m_pDevice, hTexSize, texSize, 2);
        }
        
        // Set texture
        m_pDevice->SetTexture(0, pSource);
        
        // Draw full-screen quad
        m_pDevice->SetStreamSource(0, m_pQuadVB, 0, sizeof(Vertex));
        m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
        
        // Cleanup
        m_pDevice->SetTexture(0, NULL);
        m_pDevice->SetPixelShader(NULL);
    }
};
```

### Multi-Pass Shader Chain

```cpp
void ApplyShaderChain(std::vector<ShaderFilter*>& shaders, 
                      IDirect3DTexture9* pSource,
                      IDirect3DSurface9* pFinalTarget) {
    
    IDirect3DTexture9* pTempTex[2];
    CreateTempTextures(pTempTex, 2);
    
    int currentSource = 0;
    IDirect3DTexture9* pCurrentSource = pSource;
    
    for (size_t i = 0; i < shaders.size(); i++) {
        bool isLastPass = (i == shaders.size() - 1);
        
        IDirect3DSurface9* pDestSurface;
        if (isLastPass) {
            pDestSurface = pFinalTarget;
        } else {
            pTempTex[currentSource]->GetSurfaceLevel(0, &pDestSurface);
        }
        
        shaders[i]->Apply(pCurrentSource, pDestSurface);
        
        pDestSurface->Release();
        
        if (!isLastPass) {
            pCurrentSource = pTempTex[currentSource];
            currentSource = 1 - currentSource; // Flip between temps
        }
    }
}
```

---

## Sample HLSL Code {#sample-hlsl-code}

### Complete Vertex Shader

```hlsl
// Full-screen quad vertex shader
struct VS_INPUT {
    float4 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct VS_OUTPUT {
    float4 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;
    
    // No transformation needed for full-screen pass
    output.Position = input.Position;
    output.TexCoord = input.TexCoord;
    
    return output;
}
```

### Complete Pixel Shader: PS1 RGB Decoder

```hlsl
// Decode PS1 16-bit RGB to 32-bit
sampler2D PS1FrameBuffer : register(s0);

// PS1 uses 16-bit colors: 5-5-5-1 or 5-6-5 format
float DecodeFormat : register(c0);  // 0 = 5551, 1 = 565

float4 DecodePS1Color(float2 texCoord : TEXCOORD0) : COLOR {
    float4 packed = tex2D(PS1FrameBuffer, texCoord);
    
    // Assuming 5551 format (most common for PS1)
    // R: bits 0-4, G: bits 5-9, B: bits 10-14, A: bit 15
    float r = floor(packed.r * 31.0 + 0.5) / 31.0;
    float g = floor(packed.g * 31.0 + 0.5) / 31.0;
    float b = floor(packed.b * 31.0 + 0.5) / 31.0;
    
    // Expand to full range
    r = pow(r, 2.2);  // Gamma correction
    g = pow(g, 2.2);
    b = pow(b, 2.2);
    
    return float4(r, g, b, 1.0);
}
```

### Complete Multi-Effect Shader

```hlsl
// Combined scanline + curvature + glow effect
sampler2D Texture : register(s0);
float2 TextureSize : register(c0);
float2 OutputSize : register(c1);

float Curvature : register(c2);
float ScanlineIntensity : register(c3);
float GlowStrength : register(c4);

float4 CombinedPS(float2 texCoord : TEXCOORD0) : COLOR {
    // --- Curvature ---
    float2 curvedCoord = texCoord;
    if (Curvature > 0.0) {
        float2 centered = texCoord - 0.5;
        float distortion = dot(centered, centered) * Curvature;
        curvedCoord = centered * (1.0 + distortion) + 0.5;
        
        // Vignetting
        float vignette = 1.0 - distortion * 2.0;
    }
    
    // --- Glow (pre-sample) ---
    float2 ps = 1.0 / TextureSize;
    float3 glow = 0.0;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float2 offset = float2(x, y) * ps * 2.0;
            glow += tex2D(Texture, curvedCoord + offset).rgb;
        }
    }
    glow /= 9.0;
    
    // --- Main sample ---
    float3 color = tex2D(Texture, curvedCoord).rgb;
    
    // Add glow
    color = lerp(color, glow, GlowStrength);
    
    // --- Scanlines ---
    float scanline = frac(curvedCoord.y * OutputSize.y);
    float scanlineMask = lerp(1.0, 0.5 + 0.5 * cos(scanline * 2.0 * 3.14159), 
                              ScanlineIntensity);
    color *= scanlineMask;
    
    // Apply vignetting
    float2 centered = curvedCoord - 0.5;
    float vignette = 1.0 - dot(centered, centered) * Curvature * 0.5;
    color *= saturate(vignette);
    
    return float4(color, 1.0);
}
```

---

## Performance Considerations {#performance-considerations}

### Instruction Count Optimization

```hlsl
// BAD: 30+ instructions
float a = x * 2.0;
float b = a + 1.0;
float c = b * 0.5;
float d = c - 0.25;

// GOOD: ~5 instructions through algebra
float d = x + 0.25;
```

### Texture Cache Optimization

```hlsl
// BAD: Random access pattern
for (int i = 0; i < 9; i++) {
    float r = rand(i);
    color += tex2D(tex, coord + r);
}

// GOOD: Sequential, spatially local access
// Access neighbors in order, small radius
```

### Precision Selection

```hlsl
// Use half-precision where full float32 isn't needed
half3 color = tex2D(tex, uv).rgb;  // 16-bit per component
half2 coord = uv * half2(1024.0, 512.0);

// Reserve float for positions and accumulations
float4 position = float4(x, y, z, w);
float accumulator = 0.0;
```

### Multi-Pass vs Single-Pass

| Approach | Pros | Cons |
|----------|------|------|
| Single-Pass | No intermediate targets, no context switches | Limited by instruction count, complex |
| Multi-Pass | Modular, simpler shaders, better cache use | More memory bandwidth, setup overhead |

For Xbox 360, multi-pass is often better due to the 1024 instruction limit.

### Memory Usage

- Render targets should use eDRAM-resident formats where possible
- Avoid excessive ping-ponging between render targets
- Reuse texture memory for temporary buffers
- Consider using half-precision formats (R16G16B16A16F) for intermediate passes

### Shader Precompilation

Precompile shaders to avoid runtime compilation overhead:

```cpp
// Save compiled shader
ID3DXBuffer* pCompiled;
D3DXCompileShader(..., &pCompiled, ...);
FILE* file = fopen("shader.bin", "wb");
fwrite(pCompiled->GetBufferPointer(), 1, 
       pCompiled->GetBufferSize(), file);
fclose(file);

// Load precompiled shader
FILE* file = fopen("shader.bin", "rb");
fseek(file, 0, SEEK_END);
size_t size = ftell(file);
fseek(file, 0, SEEK_SET);
void* data = malloc(size);
fread(data, 1, size, file);
fclose(file);

pDevice->CreatePixelShader((DWORD*)data, &pShader);
```

### Frame Time Budget

For 60 FPS PS1 emulation with shaders:

- **Total frame time**: 16.67ms
- **PS1 emulation**: ~8-10ms
- **Shader processing**: ~2-4ms
- **VSync/present**: ~2-3ms
- **Headroom**: 1-2ms

Target shader chain complexity to fit within 2-4ms GPU time.

---

## Conclusion

HLSL shaders on the Xbox 360 provide powerful capabilities for enhancing PS1 emulation visuals. The Xenos GPU's unified shader architecture and high memory bandwidth make it well-suited for real-time video filtering at HD resolutions.

Key recommendations:

1. **Use multi-pass rendering** for complex effects to avoid instruction limits
2. **Profile shader performance** using PIX or GPU timestamp queries
3. **Optimize texture sampling** - minimize samples and use appropriate filtering modes
4. **Consider precision** - use `half` types where appropriate
5. **Precompile shaders** to avoid runtime compilation stalls
6. **Chain effects intelligently** - order passes to minimize intermediate buffer requirements

With careful optimization, the Xbox 360 can deliver PS1 emulation with high-quality CRT emulation, advanced scaling algorithms, and real-time effects while maintaining full-speed 60 FPS gameplay.

---

## References

- Xbox 360 SDK Documentation
- Direct3D 9 Reference
- Xenos GPU Architecture Whitepapers
- RetroArch shader repository (libretro/common-shaders)
- HLSL reference documentation
- xBR/xBRZ algorithm papers
- CRT emulation research papers

---

*Document Version: 1.0*  
*Last Updated: January 2026*  
*Maintainer: PCSX-R Xbox 360 Development Team*

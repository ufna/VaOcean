Overview
========

VaOcean is the ocean surface simulation plugin for [Unreal Engine 4](https://www.unrealengine.com/).

Key features:
* Component that renders displacement and gradient (normal) maps in real time
* Sample content, including water shader and grid meshes to be used on scene
* Set of global shaders that perform FFT calculation and other tech stuff on GPU

Check the **[Wiki](https://github.com/ufna/VaOcean/wiki)** tab to know more about the plugin.

Current version: **0.6 A 1** (UE 4.19)

Right now the plugin is a state of development. More features should be added soon! 

To install this plugin simpy drop it into the plugin folder in your project root directory. The enable it from the plug in menu. An example map is included.

![SCREENSHOT](SCREENSHOT.jpg)


Legal info
----------

Unreal® is a trademark or registered trademark of Epic Games, Inc. in the United States of America and elsewhere. Unreal® Engine, Copyright 1998 – 2014, Epic Games, Inc. All rights reserved.

Plugin is completely **free** and available under [MIT open-source license](LICENSE).


References
----------

1. [Tessendorf, Jerry. Simulating Ocean Water.](http://graphics.ucsd.edu/courses/rendering/2005/jdewall/tessendorf.pdf) In SIGGRAPH 2002 Course Notes #9 (Simulating Nature: Realistic and Interactive Techniques), ACM Press.

1. Phillips, O.M. 1957. On the generation of waves by turbulent wind. Journal of Fluid Mechanics. 2 (5): 417–445.

1. [NVIDIA SDK sample](https://developer.nvidia.com/dx11-samples) "FFT Ocean"

1. Kevin Gao, [Ocean simulation sample in C++ AMP](https://blogs.msdn.microsoft.com/nativeconcurrency/2011/11/10/ocean-simulation-sample-in-c-amp/)

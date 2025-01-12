[HOME](../Readme.md)

## RIS Patterns / c++ plugins

![network_example](network_example_pc.png)

#### SamplePoints
Allows you to sample the closest points from the file and store their sorted indexes and distances in the **ArrayData** structure for further reading. Similar to **pcopen**/**nearpoints** vex functions.

#### ReadAttribute
Read specified point attributes from **ArrayData** structure indexes.
An empty filename means using the same file.
<br/><br/><br/>


![network_example](network_example.png)

#### Closest
The plugin is similar to the **xyzdist** vex function. It allows you to find the closest point on the surface/spline and store it in the **ClosestData** structure for further reading.

#### Interpolator
Read interpolated attribute value from geometry at point matching the **ClosestData** structure. Similar to the **prim_attribute** vex function.
An empty filename means using the same file.
<br/><br/>

## OSL Patterns / Shaders
OSL shaders here are very simple examples demonstrating how you can manipulate results from samplers to build procedural effects.

![network_example](network_example_tex.png)
#### BuildCoords
Utility shader for building array of uv's from point-cloud-like data.

#### SampleTexture
Utility shader for batch reading textures from an array of uv's.
<br/><br/>

## VOP structures
Custom structures are defined in C++ and OSL headers. They are also defined as a Houdini Vop type in *vop/structs.json*, which allows you to work with those structures in **OSL Generic Shader Builder**.
<br/><br/>

## Note about derivatives in OSL and RIS
The derivative calculation is required for Bump Mapping and Mip-Map optimization. RIS and OSL have very different mechanisms of derivative calculation in RenderMan. Be careful when using C++/RIS results as texture coordinates With OSL textures. Check if it works with **Dx()**, and **Dy()** functions or manually control texture filtering with the **width** parameter.

[HOME](../Readme.md)

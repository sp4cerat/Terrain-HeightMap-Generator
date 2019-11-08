# Terrain-HeightMap-Generator
## Terrain Generator with Erosion

In 2014 I have worked on a terrain generator. The basic 3D perlin noise works well to generate a generic terrain, but to get more appealing mountains, different approaches are necessary. For height maps, perlin noise can be used too, but does not look as appealing as erosion based terrain.

To achieve similar looking structures, Diffusion-limited_aggregation (DLA) is the solution. The initial result looks as the upper left image. Using gaussian smoothing, the final height image, lower right, can be obtained. The final rendering of the 3D image follows on the lower left.

The algorithm requires three steps:
Create the DLA image. When attaching a new point, draw a line to the referring point and add random midpoint displacement to the line to make it more interesting.
Create multiple copies of the DLA image and apply gaussian smoothing. The smoothing radius is increased exponentially from one image to the next.

Sum up all copies weighted and normalize the result.
DLA cannot easily create an infinite procedural heightmap, but using a sparse set of points, its possible to cover a quite large area and refine the DLA once you get closer. Due to the smoothing radius of 1..256 here, one pixel is influenced by an area of 512x512 pixels around the target pixel.

I have searched to find a reference for this algorithm, but so far I could not find one yet.

![Sample](https://github.com/sp4cerat/Terrain-HeightMap-Generator/blob/master/data/title2.png?raw=true)

![Sample](https://github.com/sp4cerat/Terrain-HeightMap-Generator/blob/master/data/title.png?raw=true)

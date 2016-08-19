Jerry the Ogre
==============
contact: Keenan Crane (kcrane@uiuc.edu)

Files
--------------------------------------------
This archive contains a surface description of "Jerry the Ogre," including:

1. the original Catmull-Clark control mesh
2. five blend shape triangle meshes (with normals)
3. texture atlases for each of the blend shapes
4. occlusion maps for each of the blend shapes
5. a diffuse color map
6. a displacement map
7. a normal map
8. two geometry images

Note that some of the included data can be generated from the meshes, but is
provided for convenience.  All meshes are stored as Wavefront OBJ, all images
are stored as PNG.  Some notes about individual items:

1. The original surface is stored as the control polygons of a Catmull-Clark
subdivision surface.  Note that your implementation must support control
polygons with an arbitrary number of vertices (some commercial packages support
only triangles and quads).  The surface is homeomorphic to a sphere, but the
embedding has self-intersections.

2. The blend shapes are stored as triangle meshes of identical topology.  The
back of the head has been removed to allow for a simple parameterization.  Each
of the surfaces is homeomorphic to a disk.

3. Each blend shape mesh stores the same parameterization (specified by
"vt" lines in the OBJ files), which maps to the unit square.  This
parameterization corresponds to the layout of the textures.

6. Darker values in the displacement map correspond to smaller displacements.

7. The normal map is stored with respect to local tangent spaces on the surface.

8. The geometry image gi_conformal uses a conformal parameterization which is
different from the parameterization stored with blend shapes.


License
--------------------------------------------

As the sole author of this data, I hereby release it into the public domain.


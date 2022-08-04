/***********************************************************************
elevationShader - Shader vertex to compute elevation and vertex location.
Copyright (c) 2016 Thomas Wolf

-- adapted from SurfaceRenderer by Oliver Kreylos
Copyright (c) 2012-2015 Oliver Kreylos

This file is part of the Magic Sand.

The Magic Sand is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Magic Sand is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Magic Sand; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#version 150

// these are for the programmable pipeline system and are passed in
// by default from OpenFrameworks
uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 textureMatrix;
uniform mat4 modelViewProjectionMatrix;

in vec4 position;
in vec4 color;
in vec4 normal;
in vec2 texcoord;
// this is the end of the default functionality

// this is something we're creating for this shader
out vec2 varyingtexcoord;
out float depthfrag;

uniform sampler2DRect tex0; // Sampler for the depth image-space elevation texture
uniform vec2 depthTransformation; // Normalisation factor and offset applied by openframeworks
uniform vec2 contourLineFboTransformation; // Transformation from elevation to normalized contourline fbo unit factor and offset

uniform mat4 rs2WorldMatrix; // Transformation from kinect image space to kinect world space
uniform mat4 rs2ProjMatrix; // Transformation from kinect world space to proj image space
uniform vec4 basePlaneEq; // Base plane equation

void main()
{
    // copy position so we can work with it.
    vec4 pos = position;
    varyingtexcoord = pos.xy;//texcoord;

    /* Set the vertex' depth image-space z coordinate from the texture: */
    vec4 texel0 = texture(tex0, varyingtexcoord);
    float depth1 = texel0.r;
    float depth = depth1 * depthTransformation.x + depthTransformation.y;

    pos.z = depth;
    pos.w = 1;

    /* Transform the vertex from depth image space to world space: */
    vec4 vertexCc = rs2WorldMatrix * pos;  // Transposed multiplication (Row-major order VS col major order
    vec4 vertexCcx = vertexCc * depth;
    vertexCcx.w = 1;

    /* Take into account baseplane orientation and location: */
    float elevation = dot(basePlaneEq,vertexCcx);///vertexCc.w;
    depthfrag = (elevation-contourLineFboTransformation.y)/contourLineFboTransformation.x;

    /* Transform vertex to proj coordinates: */
    vec4 screenPos = rs2ProjMatrix * vertexCcx;
    vec4 projectedPoint = screenPos / screenPos.z;

    projectedPoint.z = 0;
    projectedPoint.w = 1;

	gl_Position = modelViewProjectionMatrix * projectedPoint;
}

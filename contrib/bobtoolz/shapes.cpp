/*
BobToolz plugin for GtkRadiant
Copyright (C) 2001 Gordon Biggans

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "StdAfx.h"

#include "shapes.h"

#include "gtkr_list.h"

#include "DPoint.h"
#include "DPlane.h"

#include "str.h"
#include "misc.h"
#include "funchandlers.h"

#include "iundo.h"

#include "refcounted_ptr.h"

#include <vector>
#include <list>
#include <map>
#include <algorithm>

#include "scenelib.h"

//#include "dialogs-gtk.h"

/************************
	Cube Diagram
************************/

/*

		7 ----- 5
	    /|    /|
	   / |   / |
	  /  |  /  |
	4 ----- 6  |
	 |  2|_|___|8
	 |  /  |   /
	 | /   |  /       ----> WEST, definitely
	 |/    | /
	1|_____|/3

*/

/************************
	Global Variables
************************/

vec3_t	g_Origin = {0.0f, 0.0f, 0.0f};

extern bool bFacesAll[];

/************************
	Helper Functions
************************/

float Deg2Rad(float angle)
{
	return (float)(angle*Q_PI/180);
}

void AddFaceWithTexture(scene::Node* brush, vec3_t va, vec3_t vb, vec3_t vc, const char* texture, bool detail)
{
	_QERFaceData faceData;
	FillDefaultTexture(&faceData, va, vb, vc, texture);
	if(detail)
		faceData.m_texdef.contents |= FACE_DETAIL;
#if 0
	brush->m_brush->addPlane(faceData.m_p0, faceData.m_p1, faceData.m_p2, faceData.m_texdef);
#endif
}

void AddFaceWithTextureScaled(scene::Node* brush, vec3_t va, vec3_t vb, vec3_t vc, 
							  const char* texture, bool bVertScale, bool bHorScale, 
							  float minX, float minY, float maxX, float maxY)
{
	qtexture_t* pqtTexInfo;

  // TTimo: there used to be a call to pfnHasShader here
  //   this was not necessary. In Radiant everything is shader.
  //   If a texture doesn't have a shader script, a default shader object is used.
  // The IShader object was leaking also
	// collect texture info: sizes, etc
	IShader* i = QERApp_Shader_ForName(texture);
  pqtTexInfo = i->getTexture();	// shader width/height doesn't come out properly

	if(pqtTexInfo)
	{
		float scale[2] = {0.5f, 0.5f};
		float shift[2] = {0, 0};

		if(bHorScale)
		{
			int texWidth = pqtTexInfo->width;
			float width = maxX - minX;

			scale[0] = width/texWidth;
			shift[0] = -(float)((int)maxX%(int)width)/scale[0];
		}

		if(bVertScale)
		{
			int texHeight = pqtTexInfo->height;
			float height = maxY - minY;

			scale[1] = height/texHeight;
			shift[1] = (float)((int)minY%(int)height)/scale[1];
		}

		_QERFaceData addFace;
		FillDefaultTexture(&addFace, va, vb, vc, texture);
		addFace.m_texdef.scale[0] = scale[0];
		addFace.m_texdef.scale[1] = scale[1];
		addFace.m_texdef.shift[0] = shift[0];
		addFace.m_texdef.shift[1] = shift[1];

#if 0
	  brush->m_brush->addPlane(addFace.m_p0, addFace.m_p1, addFace.m_p2, addFace.m_texdef);
#endif
	}
	else
	{
		// shouldn't even get here, as default missing texture should be returned if
		// texture doesn't exist, but just in case
		AddFaceWithTexture(brush, va, vb, vc, texture, FALSE);
		Sys_ERROR("BobToolz::Invalid Texture Name-> %s", texture);
	}
  // the IShader is not kept referenced, DecRef it
  i->DecRef();
}

/************************
	--Main Functions--
************************/

void Build_Wedge(int dir, vec3_t min, vec3_t max, bool bUp)
{
  scene::Node* newBrush = Brush_AllocNode();

	vec3_t v1, v2, v3, v5, v6, v7, v8;
	VectorCopy(min, v1);
	VectorCopy(min, v2);
	VectorCopy(min, v3);
	VectorCopy(max, v5);
	VectorCopy(max, v6);
	VectorCopy(max, v7);
	VectorCopy(max, v8);

	v2[0] = max[0];
	v3[1] = max[1];

	v6[0] = min[0];
	v7[1] = min[1];
	v8[2] = min[2];

	if(bUp)
	{

		if(dir != MOVE_EAST)
			AddFaceWithTexture(newBrush, v1, v3, v6, "textures/common/caulk", FALSE);

		if(dir != MOVE_WEST)
			AddFaceWithTexture(newBrush, v7, v5, v8, "textures/common/caulk", FALSE);

		if(dir != MOVE_NORTH)
			AddFaceWithTexture(newBrush, v1, v7, v2, "textures/common/caulk", FALSE);

		if(dir != MOVE_SOUTH)
			AddFaceWithTexture(newBrush, v3, v8, v6, "textures/common/caulk", FALSE);

		AddFaceWithTexture(newBrush, v1, v2, v3, "textures/common/caulk", FALSE);

		if(dir == MOVE_EAST)
			AddFaceWithTexture(newBrush, v1, v3, v5, "textures/common/caulk", FALSE);

		if(dir == MOVE_WEST)
			AddFaceWithTexture(newBrush, v2, v6, v8, "textures/common/caulk", FALSE);

		if(dir == MOVE_NORTH)
			AddFaceWithTexture(newBrush, v1, v6, v5, "textures/common/caulk", FALSE);

		if(dir == MOVE_SOUTH)
			AddFaceWithTexture(newBrush, v7, v3, v8, "textures/common/caulk", FALSE);
	}
	else
	{
		if(dir != MOVE_WEST)
			AddFaceWithTexture(newBrush, v7, v5, v8, "textures/common/caulk", FALSE);

		if(dir != MOVE_EAST)
			AddFaceWithTexture(newBrush, v1, v3, v6, "textures/common/caulk", FALSE);

		if(dir != MOVE_NORTH)
			AddFaceWithTexture(newBrush, v3, v8, v6, "textures/common/caulk", FALSE);

		if(dir != MOVE_SOUTH)
			AddFaceWithTexture(newBrush, v1, v7, v2, "textures/common/caulk", FALSE);

		
		AddFaceWithTexture(newBrush, v6, v5, v7, "textures/common/caulk", FALSE);

		if(dir == MOVE_WEST)
			AddFaceWithTexture(newBrush, v1, v5, v3, "textures/common/caulk", FALSE);

		if(dir == MOVE_EAST)
			AddFaceWithTexture(newBrush, v2, v8, v6, "textures/common/caulk", FALSE);

		if(dir == MOVE_NORTH)
			AddFaceWithTexture(newBrush, v1, v5, v6, "textures/common/caulk", FALSE);

		if(dir == MOVE_SOUTH)
			AddFaceWithTexture(newBrush, v7, v8, v3, "textures/common/caulk", FALSE);
	}

	GetWorldspawn()->m_traverse->insert(newBrush);
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

void Build_StairStep_Wedge(int dir, vec3_t min, vec3_t max, const char* mainTexture, const char* riserTexture, bool detail)
{
  scene::Node* newBrush = Brush_AllocNode();

	//----- Build Outer Bounds ---------

	vec3_t v1, v2, v3, v5, v6, v7, v8;
	VectorCopy(min, v1);
	VectorCopy(min, v2);
	VectorCopy(min, v3);
	VectorCopy(max, v5);
	VectorCopy(max, v6);
	VectorCopy(max, v7);
	VectorCopy(max, v8);

	v2[0] = max[0];
	v3[1] = max[1];

	v6[0] = min[0];
	v7[1] = min[1];

	v8[2] = min[2];
	//v8 needed this time, becoz of sloping faces (2-4-6-8)

	//----------------------------------

	AddFaceWithTexture(newBrush, v6, v5, v7, mainTexture, detail);

	if(dir != MOVE_EAST)
	{
		if(dir == MOVE_WEST)
			AddFaceWithTexture(newBrush, v5, v2, v7, riserTexture, detail);
		else
			AddFaceWithTexture(newBrush, v5, v2, v7, "textures/common/caulk", detail);
	}

	if(dir != MOVE_WEST)
	{
		if(dir == MOVE_EAST)
			AddFaceWithTexture(newBrush, v1, v3, v6, riserTexture, detail);
		else
			AddFaceWithTexture(newBrush, v1, v3, v6, "textures/common/caulk", detail);
	}

	if(dir != MOVE_NORTH)
	{
		if(dir == MOVE_SOUTH)
			AddFaceWithTexture(newBrush, v3, v5, v6, riserTexture, detail);
		else
			AddFaceWithTexture(newBrush, v3, v5, v6, "textures/common/caulk", detail);
	}

	if(dir != MOVE_SOUTH)
	{
		if(dir == MOVE_NORTH)
			AddFaceWithTexture(newBrush, v1, v7, v2, riserTexture, detail);
		else
			AddFaceWithTexture(newBrush, v1, v7, v2, "textures/common/caulk", detail);
	}

		
	if(dir == MOVE_EAST)
		AddFaceWithTexture(newBrush, v1, v5, v3, "textures/common/caulk", detail);

	if(dir == MOVE_WEST)
		AddFaceWithTexture(newBrush, v2, v8, v6, "textures/common/caulk", detail);

	if(dir == MOVE_NORTH)
		AddFaceWithTexture(newBrush, v1, v5, v6, "textures/common/caulk", detail);

	if(dir == MOVE_SOUTH)
		AddFaceWithTexture(newBrush, v7, v8, v3, "textures/common/caulk", detail);

  GetWorldspawn()->m_traverse->insert(newBrush);
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

// internal use only, to get a box without finishing construction
scene::Node* Build_Get_BoundingCube_Selective(vec3_t min, vec3_t max, char* texture, bool* useFaces)
{
  scene::Node* newBrush = Brush_AllocNode();

	//----- Build Outer Bounds ---------

	vec3_t v1, v2, v3, v5, v6, v7;
	VectorCopy(min, v1);
	VectorCopy(min, v2);
	VectorCopy(min, v3);
	VectorCopy(max, v5);
	VectorCopy(max, v6);
	VectorCopy(max, v7);

	v2[0] = max[0];
	v3[1] = max[1];

	v6[0] = min[0];
	v7[1] = min[1];

	//----------------------------------

	//----- Add Six Cube Faces ---------

	if(useFaces[0])
		AddFaceWithTexture(newBrush, v1, v2, v3, texture, FALSE);
	if(useFaces[1])
		AddFaceWithTexture(newBrush, v1, v3, v6, texture, FALSE);
	if(useFaces[2])
		AddFaceWithTexture(newBrush, v1, v7, v2, texture, FALSE);

	if(useFaces[3])
		AddFaceWithTexture(newBrush, v5, v6, v3, texture, FALSE);
	if(useFaces[4])
		AddFaceWithTexture(newBrush, v5, v2, v7, texture, FALSE);
	if(useFaces[5])
		AddFaceWithTexture(newBrush, v5, v7, v6, texture, FALSE);

	//----------------------------------

	return newBrush;
}

scene::Node* Build_Get_BoundingCube(vec3_t min, vec3_t max, char* texture)
{
	return Build_Get_BoundingCube_Selective(min, max, texture, bFacesAll);
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

void Build_StairStep(vec3_t min, vec3_t max, const char* mainTexture, const char* riserTexture, int direction)
{
  scene::Node* newBrush = Brush_AllocNode();

	//----- Build Outer Bounds ---------

	vec3_t v1, v2, v3, v5, v6, v7;
	VectorCopy(min, v1);
	VectorCopy(min, v2);
	VectorCopy(min, v3);
	VectorCopy(max, v5);
	VectorCopy(max, v6);
	VectorCopy(max, v7);

	v2[0] = max[0];
	v3[1] = max[1];

	v6[0] = min[0];
	v7[1] = min[1];

	//----------------------------------

	AddFaceWithTexture(newBrush, v6, v5, v7, mainTexture, FALSE);
	// top gets current texture


	if(direction == MOVE_EAST)
		AddFaceWithTexture(newBrush, v1, v3, v6, riserTexture, FALSE);
	else
		AddFaceWithTexture(newBrush, v1, v3, v6, "textures/common/caulk", FALSE);
	// west facing side, etc...

	
	if(direction == MOVE_NORTH)
		AddFaceWithTexture(newBrush, v1, v7, v2, riserTexture, FALSE);
	else
		AddFaceWithTexture(newBrush, v1, v7, v2, "textures/common/caulk", FALSE);

	if(direction == MOVE_SOUTH)
		AddFaceWithTexture(newBrush, v3, v5, v6, riserTexture, FALSE);
	else
		AddFaceWithTexture(newBrush, v3, v5, v6, "textures/common/caulk", FALSE);
	
	if(direction == MOVE_WEST)
		AddFaceWithTexture(newBrush, v7, v5, v2, riserTexture, FALSE);
	else
		AddFaceWithTexture(newBrush, v7, v5, v2, "textures/common/caulk", FALSE);


	AddFaceWithTexture(newBrush, v1, v2, v3, "textures/common/caulk", FALSE);
	// base is caulked

	GetWorldspawn()->m_traverse->insert(newBrush);
	// finish brush
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

void BuildDoorsX2(vec3_t min, vec3_t max, 
				  bool bSclMainHor, bool bSclMainVert, 
				  bool bSclTrimHor, bool bSclTrimVert,
				  const char* mainTexture, const char* trimTexture,
				  int direction)
{
	int xy;
	if(direction == 0)
		xy = 0;
	else
		xy = 1;

	//----- Build Outer Bounds ---------

	vec3_t v1, v2, v3, v5, v6, v7, ve_1, ve_2, ve_3;
	VectorCopy(min, v1);
	VectorCopy(min, v2);
	VectorCopy(min, v3);
	VectorCopy(max, v5);
	VectorCopy(max, v6);
	VectorCopy(max, v7);

	v2[0] = max[0];
	v3[1] = max[1];

	v6[0] = min[0];
	v7[1] = min[1];

	float width = (max[xy] - min[xy])/2;
	
	if(direction == 0)
	{
		VectorCopy(v1, ve_1);
		VectorCopy(v3, ve_2);
		VectorCopy(v6, ve_3);
	}
	else
	{
		VectorCopy(v7, ve_1);
		VectorCopy(v1, ve_2);
		VectorCopy(v2, ve_3);
	}

	ve_1[xy] += width;
	ve_2[xy] += width;
	ve_3[xy] += width;

	//----------------------------------

  scene::Node* newBrush1 = Brush_AllocNode();
	scene::Node* newBrush2 = Brush_AllocNode();

	AddFaceWithTexture(newBrush1, v1, v2, v3, "textures/common/caulk", FALSE);
	AddFaceWithTexture(newBrush1, v5, v7, v6, "textures/common/caulk", FALSE);

	AddFaceWithTexture(newBrush2, v1, v2, v3, "textures/common/caulk", FALSE);
	AddFaceWithTexture(newBrush2, v5, v7, v6, "textures/common/caulk", FALSE);

	if(direction == 0)
	{
		AddFaceWithTexture(newBrush1, v1, v3, v6, "textures/common/caulk", FALSE);
		AddFaceWithTexture(newBrush2, v5, v2, v7, "textures/common/caulk", FALSE);
	}
	else
	{
		AddFaceWithTexture(newBrush1, v1, v7, v2, "textures/common/caulk", FALSE);
		AddFaceWithTexture(newBrush2, v5, v6, v3, "textures/common/caulk", FALSE);
	}

	if(direction == 0)
	{
		AddFaceWithTextureScaled(newBrush1, v1, v7, v2, mainTexture, bSclMainVert, bSclMainHor,
			min[0], min[2], max[0], max[2]);
		AddFaceWithTextureScaled(newBrush1, v5, v6, v3, mainTexture, bSclMainVert, bSclMainHor,
			max[0], min[2], min[0], max[2]);

		
		AddFaceWithTextureScaled(newBrush2, v1, v7, v2, mainTexture, bSclMainVert, bSclMainHor,
			min[0], min[2], max[0], max[2]);
		AddFaceWithTextureScaled(newBrush2, v5, v6, v3, mainTexture, bSclMainVert, bSclMainHor,
			max[0], min[2], min[0], max[2]);	// flip max/min to reverse tex dir


	
		AddFaceWithTextureScaled(newBrush1, ve_3, ve_2, ve_1, trimTexture, bSclTrimVert, bSclTrimHor,
			min[1], min[2], max[1], max[2]);

		AddFaceWithTextureScaled(newBrush2, ve_1, ve_2, ve_3, trimTexture, bSclTrimVert, bSclTrimHor,
			max[1], min[2], min[1], max[2]);
	}
	else
	{
		AddFaceWithTextureScaled(newBrush1, v1, v3, v6, mainTexture, bSclMainVert, bSclMainHor,
			min[1], min[2], max[1], max[2]);
		AddFaceWithTextureScaled(newBrush1, v5, v2, v7, mainTexture, bSclMainVert, bSclMainHor,
			max[1], min[2], min[1], max[2]);

		
		AddFaceWithTextureScaled(newBrush2, v1, v3, v6, mainTexture, bSclMainVert, bSclMainHor,
			min[1], min[2], max[1], max[2]);
		AddFaceWithTextureScaled(newBrush2, v5, v2, v7, mainTexture, bSclMainVert, bSclMainHor,
			max[1], min[2], min[1], max[2]);	// flip max/min to reverse tex dir


		AddFaceWithTextureScaled(newBrush1, ve_1, ve_2, ve_3, trimTexture, bSclTrimVert, bSclTrimHor,
			min[0], min[2], max[0], max[2]);

		AddFaceWithTextureScaled(newBrush2, ve_3, ve_2, ve_1, trimTexture, bSclTrimVert, bSclTrimHor,
			max[0], min[2], min[0], max[2]);
	}

	//----------------------------------
	

  scene::Node* pEDoor1 = GlobalEntityCreator().createEntity("func_door");
	scene::Node* pEDoor2 = GlobalEntityCreator().createEntity("func_door");

	if(direction == 0)
	{
    pEDoor1->m_entity->setkeyvalue("angle", "180");
    pEDoor2->m_entity->setkeyvalue("angle", "360");
	}
	else
	{
    pEDoor1->m_entity->setkeyvalue("angle", "270");
    pEDoor2->m_entity->setkeyvalue("angle", "90");
	}

	srand((unsigned)time(NULL));

	char teamname[256];
	sprintf(teamname, "t%i", rand());
  pEDoor1->m_entity->setkeyvalue("team", teamname);
  pEDoor2->m_entity->setkeyvalue("team", teamname);

	pEDoor1->m_traverse->insert(newBrush1);
	pEDoor2->m_traverse->insert(newBrush2);

  GlobalSceneGraph().root()->m_traverse->insert(pEDoor1);
  GlobalSceneGraph().root()->m_traverse->insert(pEDoor2);

//	ResetCurrentTexture();
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

void MakeBevel(vec3_t vMin, vec3_t vMax)
{
  scene::Node* patch = Patch_AllocNode();
  aabb_t aabb;
  aabb_construct_for_vec3(&aabb, vMin, vMax);
#if 0
  patch->m_patch->ConstructPrefab(&aabb, eBevel, 2); // 2 == XY view
#endif

  GetWorldspawn()->m_traverse->insert(patch);
}

void BuildCornerStairs(vec3_t vMin, vec3_t vMax, int nSteps, const char* mainTexture, const char* riserTex)
{
	vec3_t* topPoints = new vec3_t[nSteps+1];
	vec3_t* botPoints = new vec3_t[nSteps+1];

	bool bFacesUse[6] = {TRUE, TRUE, FALSE, TRUE, FALSE, FALSE};

	vec3_t centre;
	VectorCopy(vMin, centre);
	centre[0] = vMax[0];

	int height = (int)(vMax[2] - vMin[2]) / nSteps;

	vec3_t vTop, vBot;
	VectorCopy(vMax, vTop);
	VectorCopy(vMin, vBot);
	vTop[2] = vMin[2] + height;

  int i;
	for(i = 0; i <= nSteps; i++)
	{
		VectorCopy(centre, topPoints[i]);
		VectorCopy(centre, botPoints[i]);
		
		topPoints[i][2] = vMax[2];
		botPoints[i][2] = vMin[2];

		topPoints[i][0] -= 10 * sinf( Q_PI * i / ( 2 * nSteps ) );
		topPoints[i][1] += 10 * cosf( Q_PI * i / ( 2 * nSteps ) );

		botPoints[i][0] = topPoints[i][0];
		botPoints[i][1] = topPoints[i][1];
	}

	vec3_t tp[3];
	for(int j = 0; j < 3; j++)
		VectorCopy(topPoints[j], tp[j]);

	for(i = 0; i < nSteps; i++)
	{
    scene::Node* brush = Build_Get_BoundingCube_Selective(vBot, vTop, "textures/common/caulk", bFacesUse);

		for(int j = 0; j < 3; j++)
			tp[j][2] = vTop[2];

		AddFaceWithTexture(brush, tp[2], tp[1], tp[0], mainTexture, FALSE);

		AddFaceWithTexture(brush, centre, botPoints[i+1], topPoints[i+1], "textures/common/caulk", FALSE);
		AddFaceWithTexture(brush, centre, topPoints[i], botPoints[i], riserTex, FALSE);

		GetWorldspawn()->m_traverse->insert(brush);

		vTop[2] += height;
		vBot[2] += height;
	}

	delete[] topPoints;
	delete[] botPoints;

	vMin[2] += height;
	vMax[2] += height;
	MakeBevel(vMin, vMax);
}

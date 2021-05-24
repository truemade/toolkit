/*
FbxToTrueSkateCommunity

Copyright 2012-2021 True Axis Pty Ltd, Australia

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

// Note: From True Axis 2021
// 
// Note: This converter is adapted from one we use internally, with a few differences, mainly to export to a human readable text format.
// Note: Code quality was not our priority with this too, rather that we complete it as quikckly as possible, as all it was originally 
// Note: intended to do was to get our art assets into the game for our internal pipe line, with out consideration that we may later 
// Note: release to the public. We release now incase anybody wants to improve on it for the benifit of the community.
// Note: If you ask us nicely, we might be willing to try help explain some things. (if we can remember).
// Note: There is a legacy code in this exporter that may not be revelent, due to True Skate tech changes over the years.
// Note: There is a good chance in the future we will do a big tech upgrade, and this will all become old, but I expect we
// Note: we would keep supporting this format.
// Note: It was designed to work with .fbx files exported from Maya set up with a special ShaderFX network, but we have instead
// Note: provided a Blender shader setup and exporter to achieve the same result.
// Note: It would probably be easy to make things to wrong by creating fbx files in slightly different formats then expected.


//#define FBXSDK_DEFINE_NAMESPACE 0
//#define FBXSDK_NAMESPACE_USING 0
//#define FBXSDK_VERSION_REVISION 2012

#define _CRT_SECURE_NO_DEPRECATE
#include <tchar.h>

#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <thread>

#include <stdio.h>
#include <time.h>

#include "fbxsdk.h" // using Autodesk FBX SDK 2015.1, but it isn't expected upgrading to a newer version would cause too many issues.
#include "NvTriStrip/include/NvTriStrip.h"

const bool bReplaceUvs = false;

typedef unsigned long u32;
typedef long s32;
typedef unsigned short u16;
typedef short s16;
typedef unsigned char u8;
typedef char s8;
typedef char Char;

const u32 ATTRIBUTE_FLAG_OBJECT_ID_MASK =		0xFF;

const u32 ATTRIBUTE_FLAG_TRANSITION =			0x100;
const u32 ATTRIBUTE_FLAG_BOWL =					0x200;
const u32 ATTRIBUTE_FLAG_VERT =					0x400;
const u32 ATTRIBUTE_FLAG_INCLINE =				0x800;
const u32 ATTRIBUTE_FLAG_TRANSITION_TOP =		0x1000;
const u32 ATTRIBUTE_FLAG_RAIL =					0x2000;
const u32 ATTRIBUTE_FLAG_LIP =					0x4000;
const u32 ATTRIBUTE_FLAG_REVERSE_TRANSITION =	0x8000;
const u32 ATTRIBUTE_FLAG_STAIRS =				0x10000;
const u32 ATTRIBUTE_FLAG_LIP_GUARDS =			0x20000;
const u32 ATTRIBUTE_FLAG_FENCE_SIDE =			0x40000;
const u32 ATTRIBUTE_FLAG_SLOW =					0x80000;

const u32 ATTRIBUTE_FLAG_MATERIAL_MASK =					0xF00000;
const u32 ATTRIBUTE_FLAG_CONCRETE =							0x000000;
const u32 ATTRIBUTE_FLAG_METAL =							0x100000;
const u32 ATTRIBUTE_FLAG_WOOD =								0x200000;
const u32 ATTRIBUTE_FLAG_GRASS =							0x300000;
const u32 ATTRIBUTE_FLAG_PADDING =							0x400000;
const u32 ATTRIBUTE_FLAG_RESPAWN =							0x500000;
const u32 ATTRIBUTE_FLAG_RESPAWN_DELAYED =					0x600000;
//const u32 ATTRIBUTE_FLAG_MATERIAL =						0x700000;
const u32 ATTRIBUTE_FLAG_SKY_ACHEIVEMENT =					0x800000;

const u32 ATTRIBUTE_GRIND_EDGE_WIDE =			0x1000000;
const u32 ATTRIBUTE_GRIND_EDGE_RAIL =			0x2000000;
const u32 ATTRIBUTE_GRIND_LIP =					0x4000000;
const u32 ATTRIBUTE_GRIND_POP_OFF =				0x8000000;

int g_nObjectId = 0;
double g_fScaleBy = 1.0f;
bool g_bOrderTransparancy = true;
bool g_bPrecalculateTransparancyOrder = false;
bool g_bReverseColourLayers = false;


FbxAMatrix g_fbxMatrixApplyRotation;
enum ShaderMode
{
	TECH2_DECAL = 0,
	TECH2_SOLID = 1,
	TECH2_TRANPARANT = 2
};

float MaxF(float a, float b) { return a > b ? a : b; }
size_t strlcpy(char *d, const char *s, size_t bufsize)
{
  size_t len;
  size_t ret;

  if (!d || !s || (int)bufsize <= 0) return 0;
  len = strlen(s);
  ret = len;
  if (len >= bufsize) len = bufsize-1;
  memcpy(d, s, len);
  d[len] = 0;

  return ret;
}

const char* stristr(const char* str1, const char* str2 )
{
    const char* p1 = str1 ;
    const char* p2 = str2 ;
    const char* r = *p2 == 0 ? str1 : 0 ;

    while( *p1 != 0 && *p2 != 0 )
    {
        if( tolower( *p1 ) == tolower( *p2 ) )
        {
            if( r == 0 )
            {
                r = p1 ;
            }

            p2++ ;
        }
        else
        {
            p2 = str2 ;
            if( tolower( *p1 ) == tolower( *p2 ) )
            {
                r = p1 ;
                p2++ ;
            }
            else
            {
                r = 0 ;
            }
        }

        p1++ ;
    }

    return *p2 == 0 ? r : 0 ;
}

#define TA_ASSERT(x)			\
	do							\
	{							\
		if (!(x))				\
		{						\
			__asm { int 3 }		\
		}						\
	} while (0)



FbxVector4 GetNormal(FbxVector4 v4)
{
	v4.Normalize();
	return v4;
}


const int SPRITE_PADDING_SIZE = 1;
const int MAX_UV_LISTS = 3;
const int MAX_COLOUR_LISTS = 2;

struct ImportMesh
{
	int nUvSetCount;
	int nColourSetCount;
	int nTextureIndex;
	std::vector<float> positionList;
	std::vector<float> colourSetList[MAX_COLOUR_LISTS];	// 	red green blue alpha
	std::vector<float> uvSetList[MAX_UV_LISTS];	
	std::vector<float> normalList;
	std::basic_string<char> strName;
};
std::vector<ImportMesh*> g_importMeshList;

struct ImportTexture
{
	int nId;
	bool bLayeredTexture;
	bool bDecal;
	std::basic_string<char> strFileName;
	std::basic_string<char> strName;
	std::vector<int> textureArray; // if bLayeredTexture = true

	float pfColor[4];
	float fSpecular;

	float fBlendSharpness_G;
	float fBlendLevel_G;
	float fBlendMode_G;
	float pfShadowColor_G[4];
	float pfHighlightColor_G[4];
	float fIgnoreBaseColor_G;
	float fSpecular_G;

	float fBlendSharpness_B;
	float fBlendLevel_B;
	float fBlendMode_B;
	float pfShadowColor_B[4];
	float pfHighlightColor_B[4];
	float fIgnoreBaseColor_B;
	float fSpecular_B;

	ImportTexture()
	{
		nId = - 1;
		bLayeredTexture = false;
		bDecal = false;

		pfColor[0] = 1.0f;
		pfColor[1] = 1.0f;
		pfColor[2] = 1.0f;
		pfColor[3] = 1.0f;

		fSpecular = 1.0f;

		fBlendSharpness_G = 16.0f;
		fBlendLevel_G = 0.75f;
		fBlendMode_G = 0.0f;
		for (int i = 0; i < 4; i++)
			pfShadowColor_G[i] = 0.0f;
		for (int i = 0; i < 4; i++)
			pfHighlightColor_G[i] = 1.0f;
		fIgnoreBaseColor_G = 0.0f;
		fSpecular_G = 1.0f;
		fBlendSharpness_B = 16.0f;
		fBlendLevel_B = 0.75f;
		fBlendMode_B = 0.0f;
		for (int i = 0; i < 4; i++)
			pfShadowColor_B[i] = 0.0f;
		for (int i = 0; i < 4; i++)
			pfHighlightColor_B[i] = 1.0f;
		fIgnoreBaseColor_B = 0.0f;
		fSpecular_B = 1.0f;
	}
	
};
std::vector<ImportTexture*> g_importTextureList;

struct ImportCollisionMesh
{
	std::vector<float> positionList;
	std::vector<int> polygonNumVerticesList;
	std::vector<int> polygonIndiciesList;	
	std::vector<u32> polygonAttirbuteList;

	std::vector<float> grindEdgeList;
	std::vector<u32> grindEdgeAttributeList;
};
ImportCollisionMesh g_importCollisionMesh;

struct ImportOob // Object Oriented Box
{
	enum { MAX_NAME_LENGTH = 64 };
	char szName[MAX_NAME_LENGTH];
	FbxAMatrix fbxMatrix;
};

std::vector<ImportOob*> g_importOobList;

struct SortPrimitives
{
	PrimitiveGroup* pPrimitiveGroup;
	float fDistanceSqrd;
};
bool SortPrimitivesSort(const SortPrimitives& a, const SortPrimitives& b)
{
	return a.fDistanceSqrd > b.fDistanceSqrd ? true : false;
}
bool SortRemoveList(const int& a, const int& b)
{
	return a > b ? true : false;
}




void CollisionMeshRemoveUnusedVerticies(ImportCollisionMesh& mesh)
{
	std::vector<int> positionCountList;
	std::vector<int> positionRemapList;
	for (size_t i = 0; i < mesh.positionList.size() / 3; i++)
		positionCountList.push_back(0);

	size_t nIndex = 0;
	for (size_t i = 0; i < mesh.polygonNumVerticesList.size(); i++)
	{
		int nNumSides = mesh.polygonNumVerticesList[i];

		for (int i = 0; i < nNumSides; i++)
		{
			int nVertex = mesh.polygonIndiciesList[nIndex++];
			positionCountList[nVertex]++;
		}
	}
	nIndex = 0;
	for (size_t i = 0; i < mesh.positionList.size() / 3; i++)
	{
		if (positionCountList[i])
		{
			positionRemapList.push_back(nIndex);
			mesh.positionList[nIndex * 3 + 0] = mesh.positionList[i * 3 + 0];
			mesh.positionList[nIndex * 3 + 1] = mesh.positionList[i * 3 + 1];
			mesh.positionList[nIndex * 3 + 2] = mesh.positionList[i * 3 + 2];
			nIndex++;
		}
		else
		{
			positionRemapList.push_back(-1);
		}
	}
	mesh.positionList.resize(nIndex * 3);

	nIndex = 0;
	for (size_t i = 0; i < g_importCollisionMesh.polygonNumVerticesList.size(); i++)
	{
		int nNumSides = g_importCollisionMesh.polygonNumVerticesList[i];

		for (int i = 0; i < nNumSides; i++)
		{
			int nVertex = g_importCollisionMesh.polygonIndiciesList[nIndex];
			TA_ASSERT(positionRemapList[nVertex] >= 0);
			g_importCollisionMesh.polygonIndiciesList[nIndex] = positionRemapList[nVertex];
			nIndex++;
		}
	}
}

enum
{
	PRIMITIVE_TYPE_STRIP = 0,
	PRIMITIVE_TYPE_TRIANGLES = 1
};

struct Mesh
{
	bool bDecal;
	int nPrimitiveType;
	int nColourSetCount;
	int nUvSetCount;
	std::vector<float> positionList;
	std::vector<float> colourSetList[MAX_COLOUR_LISTS];	// 	red green blue alpha
	std::vector<float> uvSetList[MAX_UV_LISTS];	
	std::vector<float> normalList;
	std::vector<float> fadeDistanceList;	
	std::vector<u16> triagnleIndexList;
	std::vector<u16> stripIndexList;
};
std::vector<Mesh*> g_meshList;

template <typename Type> void WriteInterger(Type nValue, FILE* pFile, char* szComment)
{
	long long nValueOut = (long long)nValue;
	if (szComment)
		fprintf(pFile, "%lld #%s\n", nValueOut, szComment);
	else
		fprintf(pFile, "%lld\n", nValueOut);
}

void WriteS32(s32 nValue, FILE* pFile, char* szComment = 0) { WriteInterger<s32>(nValue, pFile, szComment); }
void WriteU32(u32 nValue, FILE* pFile, char* szComment = 0) { WriteInterger<u32>(nValue, pFile, szComment); }
void WriteU16(u16 nValue, FILE* pFile, char* szComment = 0) { WriteInterger<u16>(nValue, pFile, szComment); }
void WriteS16(u16 nValue, FILE* pFile, char* szComment = 0) { WriteInterger<s16>(nValue, pFile, szComment); }
void WriteU8(u8 nValue, FILE* pFile, char* szComment = 0) { WriteInterger<u8>(nValue, pFile, szComment); } 
void WriteS8(s8 nValue, FILE* pFile, char* szComment = 0) { WriteInterger<s8>(nValue, pFile, szComment); } 
void WriteFloat(float fValue, FILE* pFile, char* szComment = 0) 
{ 
	if (szComment)
		fprintf(pFile, "%f #%s\n", fValue, szComment);
	else
		fprintf(pFile, "%f\n", fValue);
}
void WriteComment(FILE* pFile, char* szComment = 0)
{
	fprintf(pFile, "#%s\n", szComment);
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
void WriteString(const char* szString, FILE* pFile)
{
	fprintf(pFile, "%s\n", szString);
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
static void WriteU32ColourFromFloatArray(float pfColour[4], FILE* pFile)
{						
	int r = (int)(pfColour[0] * 256.0f);
	int g = (int)(pfColour[1] * 256.0f);
	int b = (int)(pfColour[2] * 256.0f);
	int a = (int)(pfColour[3] * 256.0f);

	if (r < 0) r = 0; else if (r > 255) r = 255;
	if (g < 0) g = 0; else if (g > 255) g = 255;
	if (b < 0) b = 0; else if (b > 255) b = 255;
	if (a < 0) a = 0; else if (a > 255) a = 255;
						
	//u32 nColour = (a << 24) + (b << 16) + (g << 8) + r;
	WriteU8(r, pFile);
	WriteU8(g, pFile);
	WriteU8(b, pFile);
	WriteU8(a, pFile);
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
class FileChunk
{
public:
	FileChunk(FILE* pFile, const char* szName, const char* szComment = 0)
	{
		TA_ASSERT(strlen(szName) == 4);
		if (szComment)
			fprintf(pFile, "<%s\n", szName);
		else
			fprintf(pFile, "<%s #", szComment);
		m_pFile = pFile;
	}

	~FileChunk()
	{
		fprintf(m_pFile, ">\n");
	}

private:
	FILE* m_pFile;
	long m_nChunkStartPos;
};




int s_nTexture = -1;
bool s_bQuietReporting = false;

bool m_bWriteOverStatus = true;
void ReportError(const char* szName, ...)
{
	if (s_bQuietReporting)
		return;
	m_bWriteOverStatus = false;
	va_list list;    
	va_start(list, szName);
	printf("\n");
	vprintf(szName, list);
	va_end(list);
}

void FatalError(const char* szName, ...)
{	
	va_list list;    
	va_start(list,szName);
	printf("\n");
	vprintf(szName, list);
	va_end(list);
	exit(1);
}

void SetStatus(char* szName, float fProgres)
{
	if (s_bQuietReporting)
		return;
	static char* szLastName = "";
	if (szLastName != szName || strcmp(szLastName, szName))
	{
		szLastName = szName;
		m_bWriteOverStatus = false;
	}
	if (m_bWriteOverStatus)
		printf("\r");
	else
		printf("\n");

	printf("%s %d%%                      ", szName , (int)(fProgres * 100.0f));
	m_bWriteOverStatus = true;
}

//---------------------------------------------------------------------------------
//	const FbxDouble3 lDiffuse = GetMaterialProperty(pMaterial,
//		FbxSurfaceMaterial::sDiffuse, FbxSurfaceMaterial::sDiffuseFactor, mDiffuse.mTextureName);
//---------------------------------------------------------------------------------
FbxDouble3 GetMaterialProperty(const FbxSurfaceMaterial * pMaterial,
    const char * pPropertyName,
    const char * pFactorPropertyName,
    int* pnTextureIndex)
{
	const char* szName = pMaterial->GetName();
    FbxDouble3 lResult(0, 0, 0);
    const FbxProperty lProperty = pMaterial->FindProperty(pPropertyName);
    const FbxProperty lFactorProperty = pMaterial->FindProperty(pFactorPropertyName);
    if (lProperty.IsValid() && lFactorProperty.IsValid())
    {
        lResult = lProperty.Get<FbxDouble3>();
        double lFactor = lFactorProperty.Get<double>();
        if (lFactor != 1)
        {
            lResult[0] *= lFactor;
            lResult[1] *= lFactor;
            lResult[2] *= lFactor;
        }
    }

	/*{
		int lTextureIndex;
        FOR_EACH_TEXTURE(lTextureIndex)
        {
            const FbxProperty lProperty = pMaterial->FindProperty(FbxLayerElement::TEXTURE_CHANNEL_NAMES[lTextureIndex]);
			int lTextureCount = lProperty.GetSrcObjectCount(FbxLayeredTexture::ClassId);
			if (lTextureCount)
				int nTest = 0;
        }
	}*/

	//pMaterial->GetCon

	//ddfdfefe; // there shold be a diffuse collour connection

    if (lProperty.IsValid())
    {
        const int lTextureCount = lProperty.GetSrcObjectCount(FbxCriteria::ObjectType(FbxTexture::ClassId));
        if (lTextureCount)
        {
			const FbxTexture* lTexture = FbxCast<FbxTexture>(lProperty.GetSrcObject(FbxCriteria::ObjectType(FbxTexture::ClassId), 0));
            if (lTexture && lTexture->GetUserDataPtr())
            {
				*pnTextureIndex = (int)(size_t)lTexture->GetUserDataPtr();
            }
        }
	}

    return lResult;
}


//---------------------------------------------------------------------------------
// hack because layered textures seem to have gone broken
//---------------------------------------------------------------------------------
int GetLayeredTexture(int nTextureId)
{
	for (int nTexture = 0; nTexture < (int)g_importTextureList.size(); nTexture++)
	{
		ImportTexture* pTexture = g_importTextureList[nTexture];
		if (pTexture->bLayeredTexture && pTexture->textureArray[0] == nTextureId)
			return nTexture;
	}

	ImportTexture* pTexture = new ImportTexture;		
	//pTexture->nId = -1;	
	pTexture->nId = g_importTextureList[nTextureId]->nId;
	pTexture->strFileName = "";
	pTexture->strName = "";
	pTexture->bLayeredTexture = true;


	pTexture->textureArray.push_back(nTextureId);

	for (int nTexture = 0; nTexture < (int)g_importTextureList.size(); nTexture++)
	{
		ImportTexture* pTextureSignage = g_importTextureList[nTexture];
		if (stristr(pTextureSignage->strName.c_str(), "signage"))
		{
			pTexture->textureArray.push_back(nTexture);
			break;
		}
	}

	TA_ASSERT(pTexture->textureArray.size() == 2);

	

	//pFbxTexture->SetUserDataPtr((void*)g_importTextureList.size());
	g_importTextureList.push_back(pTexture);

	return (int)g_importTextureList.size() - 1;

}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
int CreateDecalTextureInTheCaseWhereItIsSharingWithAnAlphaTextures(int nTextureIndex)
{
	ImportTexture* pTextureIn = g_importTextureList[nTextureIndex];
	if (!pTextureIn->bLayeredTexture)
		return nTextureIndex;
	if (pTextureIn->bDecal)
		return nTextureIndex;

	for (int i = 0; i < (int)g_importTextureList.size(); i++)
	{
		if (i == nTextureIndex)
			continue;
		ImportTexture* pTexture = g_importTextureList[i];
		if (pTexture->textureArray.size() != pTextureIn->textureArray.size())
			continue;
		int nSize = pTexture->textureArray.size();
		int j;
		for (j = 0; j < nSize; j++)
			if (pTexture->textureArray[j] != pTextureIn->textureArray[j])
				break;
		if (j != nSize)	
			continue;

		if (pTexture->bDecal)
			return i;
	}

	ImportTexture* pTexture = new ImportTexture;		
	pTexture->nId = pTextureIn->nId;
	pTexture->strFileName = pTextureIn->strFileName;
	pTexture->strName = pTextureIn->strName;
	pTexture->bLayeredTexture = true;
	pTexture->bDecal = true;
	for (int j = 0; j < pTextureIn->textureArray.size(); j++)
		pTexture->textureArray.push_back(pTextureIn->textureArray[j]);

	g_importTextureList.push_back(pTexture);
	
	return (int)g_importTextureList.size() - 1;


}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
void RecurseFbxNodes(FbxNode* pNode)
{
	if (!pNode)
		return;

    FbxString strName = pNode->GetName();
	const char* szName = strName;
 
    FbxNodeAttribute::EType eAttributeType;
 
    if (pNode->GetNodeAttribute() == NULL)
    {
		int nTest = 0;
    }
    else
    {
        eAttributeType = pNode->GetNodeAttribute()->GetAttributeType();

		FbxAMatrix fbxMatrix = g_fbxMatrixApplyRotation * pNode->EvaluateGlobalTransform();
		

 

		switch (eAttributeType)
        {
        case FbxNodeAttribute::eMesh:
			{
				FbxMesh* pMesh = (FbxMesh*)pNode->GetNodeAttribute();
				int nNumPolygons = pMesh->GetPolygonCount();

				int nMaterialCount = pNode->GetMaterialCount();

			
				
				FbxLayerElementArrayTemplate<int>* pMaterialIndicies = 0;
				pMesh->GetMaterialIndices(&pMaterialIndicies);

				FbxStringList uvSetNameList;
				pMesh->GetUVSetNames(uvSetNameList);
				int nUvSetCount = uvSetNameList.GetCount();
				int nVertexColorCount = pMesh->GetElementVertexColorCount();
				if (stristr(szName, "signage") )
				{
					nVertexColorCount = 0;
					nUvSetCount = 1;
				}
				else
				if (stristr(szName, "decal"))
				{
					nVertexColorCount = 0;
					//nUvSetCount = 1;
				}
				else
				if (nUvSetCount <=1)
					nVertexColorCount = 0; // legacy



				bool* pbShownUnMappedError = new bool[nUvSetCount];
				for (int nUvLayer = 0; nUvLayer < nUvSetCount; nUvLayer++)
					pbShownUnMappedError[nUvLayer] = false;


				int nNumMaterialIndicies = 0;
				if (pMaterialIndicies)
					nNumMaterialIndicies = pMaterialIndicies->GetCount();

				for (int nMaterial = 0; nMaterial < nMaterialCount; nMaterial++)
				{
					FbxSurfaceMaterial* pMaterial;
					if (nNumMaterialIndicies == nNumPolygons)						
						pMaterial = pNode->GetMaterial(nMaterial);
					else
						pMaterial = pNode->GetMaterial(0);	

					
					int nTextureIndex = (int)(size_t)pMaterial->GetUserDataPtr();
					if (nTextureIndex <= 0)
						continue;
					nTextureIndex -= 1;

					
					if (stristr(szName, "decal"))
					{
						if (nUvSetCount > 1)
						{
							nTextureIndex = CreateDecalTextureInTheCaseWhereItIsSharingWithAnAlphaTextures(nTextureIndex);
							//pMaterial->SetUserDataPtr((void*)(nTextureIndex + 1)); // not sure if this is needed?
						}
					}


					ImportMesh* pImportMesh = new ImportMesh();
					pImportMesh->strName = szName;
					g_importMeshList.push_back(pImportMesh);

					pImportMesh->nUvSetCount = nUvSetCount;
					if (pImportMesh->nUvSetCount > MAX_UV_LISTS)
					{
						ReportError("Mesh %s has too many uv sets", szName);
						pImportMesh->nUvSetCount = MAX_UV_LISTS;
					}

					pImportMesh->nColourSetCount = nVertexColorCount;
					if (pImportMesh->nColourSetCount > MAX_COLOUR_LISTS)
					{
						ReportError("Mesh %s has too many colour sets", szName);
						pImportMesh->nColourSetCount = MAX_COLOUR_LISTS;
					}
					
					pImportMesh->nTextureIndex = nTextureIndex;
					
					ImportTexture& importTexture = *g_importTextureList[nTextureIndex];
					
					if (strstr(importTexture.strFileName.c_str(), "Alpha") || 
						strstr(importTexture.strFileName.c_str(), "alpha") ||
						(importTexture.textureArray.size() && strstr(g_importTextureList[importTexture.textureArray[0]]->strFileName.c_str(), "Alpha")) || 
						(importTexture.textureArray.size() && strstr(g_importTextureList[importTexture.textureArray[0]]->strFileName.c_str(), "alpha")))
					{
						pImportMesh->nColourSetCount = 0;
					}


				/*	if (nUvSetCount == 1)
					{
						pImportMesh->nTextureIndex = nTextureIndex;
					}
					else
					{
						nTextureIndex = GetLayeredTexture(nTextureIndex);
						pImportMesh->nTextureIndex = nTextureIndex;
					}*/

					int nVertexOffset = 0;
					for (int nPolygon = 0; nPolygon < nNumPolygons; nPolygon++)
					{
						int nNumVertices = pMesh->GetPolygonSize(nPolygon);	
						if (pMaterialIndicies->GetAt(nPolygon) != nMaterial)
						{
							nVertexOffset += nNumVertices;
							continue;
						}
					
						int uvSetNameListCount = uvSetNameList.GetCount();
						if (uvSetNameListCount > nUvSetCount)
							uvSetNameListCount = nUvSetCount;

						for (int nVertex = 0; nVertex < nNumVertices - 2; nVertex++)
						{
							int pnVertexIds[3];
							int n1 = 0;
							int n2 = nVertex + 2;
							int n3 = nVertex + 1;
							pnVertexIds[0] = pMesh->GetPolygonVertex(nPolygon, n1);
							pnVertexIds[1] = pMesh->GetPolygonVertex(nPolygon, n2);
							pnVertexIds[2] = pMesh->GetPolygonVertex(nPolygon, n3);


							//GetPolygonVertexNormal (int pPolyIndex, int pVertexIndex, FbxVector4 &pNormal) const

							FbxColor ppfbxColor[MAX_COLOUR_LISTS][3];

							for (int nColourSetList = 0; nColourSetList < pImportMesh->nColourSetCount; nColourSetList++)
							{
								ppfbxColor[nColourSetList][0] = FbxColor(0.0, 0.0, 0.0f);
								ppfbxColor[nColourSetList][1] = FbxColor(0.0, 0.0, 0.0f);
								ppfbxColor[nColourSetList][2] = FbxColor(0.0, 0.0, 0.0f);
							}

							FbxVector2 pfbxv2UVs[3];
//							FbxVector2 pfbxv2UVsLayer2[3];
							FbxVector2 pfbxv2UVLayers[MAX_UV_LISTS][3];
							int nUvReadCount = (uvSetNameListCount < MAX_UV_LISTS) ? uvSetNameListCount : MAX_UV_LISTS;


							if (uvSetNameListCount > 0 && !bReplaceUvs)
							{
								bool pUnmapped = true;
															
								for (int nUvLayer = 0; nUvLayer < nUvReadCount; ++nUvLayer)
								{
									pMesh->GetPolygonVertexUV(nPolygon, 0, uvSetNameList[nUvLayer], pfbxv2UVLayers[nUvLayer][0], pUnmapped);
//									TA_ASSERT(!pUnmapped);									
									if (pUnmapped && !pbShownUnMappedError[nUvLayer])
									{
										pbShownUnMappedError[nUvLayer] = true;
										ReportError("Mesh %s has unmapped uvs on layer %d", szName, nUvLayer);
									}

									pMesh->GetPolygonVertexUV(nPolygon, nVertex + 2, uvSetNameList[nUvLayer], pfbxv2UVLayers[nUvLayer][1], pUnmapped);
//									TA_ASSERT(!pUnmapped);									
									if (pUnmapped && !pbShownUnMappedError[nUvLayer])
									{
										pbShownUnMappedError[nUvLayer] = true;
										ReportError("Mesh %s has unmapped uvs on layer %d", szName, nUvLayer);
									}

									pMesh->GetPolygonVertexUV(nPolygon, nVertex + 1, uvSetNameList[nUvLayer], pfbxv2UVLayers[nUvLayer][2], pUnmapped);
//									TA_ASSERT(!pUnmapped);									
									if (pUnmapped && !pbShownUnMappedError[nUvLayer])
									{
										pbShownUnMappedError[nUvLayer] = true;
										ReportError("Mesh %s has unmapped uvs on layer %d", szName, nUvLayer);
									}
								}
									
								for (int nColourSetList = 0; nColourSetList < pImportMesh->nColourSetCount; nColourSetList++)
								{
									FbxGeometryElementVertexColor* pColour = pMesh->GetElementVertexColor(nColourSetList);
									if (pColour)
									{
										FbxLayerElement::EMappingMode eMappingMode = pColour->GetMappingMode();
										FbxLayerElement::EReferenceMode eReferenceMode = pColour->GetReferenceMode();
										ppfbxColor[nColourSetList][0] = pColour->GetDirectArray().GetAt(pColour->GetIndexArray().GetAt(nVertexOffset + n1));
										ppfbxColor[nColourSetList][1] = pColour->GetDirectArray().GetAt(pColour->GetIndexArray().GetAt(nVertexOffset + n2));
										ppfbxColor[nColourSetList][2] = pColour->GetDirectArray().GetAt(pColour->GetIndexArray().GetAt(nVertexOffset + n3));

										

									}
								}

							}
							else
							{


								FbxVector4& controlPoint0 = pMesh->GetControlPointAt(pnVertexIds[0]);
								FbxVector4& controlPoint1 = pMesh->GetControlPointAt(pnVertexIds[1]);
								FbxVector4& controlPoint2 = pMesh->GetControlPointAt(pnVertexIds[2]);
							
								FbxVector4 v3A = pMesh->GetControlPointAt(pnVertexIds[0]);
								FbxVector4 v3B = pMesh->GetControlPointAt(pnVertexIds[1]);
								FbxVector4 v3C = pMesh->GetControlPointAt(pnVertexIds[2]);
								


								v3A = fbxMatrix.MultT(v3A);
								v3B = fbxMatrix.MultT(v3B);
								v3C = fbxMatrix.MultT(v3C);

								
								/*
								{
									TA::Vec3 v3Cross_ = (v3A_ - v3B_).Cross(v3A_ - v3C_);
									float fMag = v3Cross.GetMagnitude();
									if (fMag < 0.000000000001f)
										continue;
									v3Cross /= fMag;

									TA::Mat33 m33;
									m33.SetToLookDownVector(v3Cross);
									v3A_ /= m33;
									v3B_ /= m33;
									v3C_ /= m33;
								}

								
								
								FbxVector4 v3Cross = (v3A - v3B).CrossProduct(v3A - v3C);
								{
									double fMag = v3Cross.GetMagnitude();
									if (fMag < 0.000000000001)
										continue;
									v3Cross /= fMag;

									FbxAMatrix fbxMatrixUv;
									

									// z 
									fbxMatrixUv. = v3Cross;

									// x
									int pnAxis[3];
									v3Cross.GetAxisOrder(pnAxis);

									Vec3 v3UnitVector2 = Vec3::GetUnitVector(pnAxis[2]);
									v3X = v3Vector.Cross(v3UnitVector2);
									v3X.Normalise();

									// y
									v3Y = v3Z.Cross(v3X);
								}*/

								float fUfReplaceScale = 0.004f;

								pfbxv2UVs[0].mData[0] = v3A[0] * fUfReplaceScale;
								pfbxv2UVs[0].mData[1] = v3A[1] * fUfReplaceScale;
								pfbxv2UVs[1].mData[0] = v3B[0] * fUfReplaceScale;
								pfbxv2UVs[1].mData[1] = v3B[1] * fUfReplaceScale;
								pfbxv2UVs[2].mData[0] = v3C[0] * fUfReplaceScale;
								pfbxv2UVs[2].mData[1] = v3C[1] * fUfReplaceScale;

								if (uvSetNameListCount > 1)
								{
									for (int nUvLayer = 1; nUvLayer < nUvReadCount; ++nUvLayer)
									{
										pfbxv2UVLayers[nUvLayer][0].mData[0] = v3A[0] * fUfReplaceScale;
										pfbxv2UVLayers[nUvLayer][0].mData[1] = v3A[1] * fUfReplaceScale;
										pfbxv2UVLayers[nUvLayer][1].mData[0] = v3B[0] * fUfReplaceScale;
										pfbxv2UVLayers[nUvLayer][1].mData[1] = v3B[1] * fUfReplaceScale;
										pfbxv2UVLayers[nUvLayer][2].mData[0] = v3C[0] * fUfReplaceScale;
										pfbxv2UVLayers[nUvLayer][2].mData[1] = v3C[1] * fUfReplaceScale;
									}
								}
							}

							FbxVector4 pfbxv4Normals[3];
							pMesh->GetPolygonVertexNormal(nPolygon, 0, pfbxv4Normals[0]);
							pMesh->GetPolygonVertexNormal(nPolygon, nVertex + 2, pfbxv4Normals[1]);
							pMesh->GetPolygonVertexNormal(nPolygon, nVertex + 1, pfbxv4Normals[2]);
							

							for (int i = 0; i < 3; i++)
							{
								FbxVector4 v3Position = pMesh->GetControlPointAt(pnVertexIds[i]);
								v3Position = fbxMatrix.MultT(v3Position);
								v3Position *= g_fScaleBy;

								pImportMesh->positionList.push_back(v3Position[0]);
								pImportMesh->positionList.push_back(v3Position[1]);
								pImportMesh->positionList.push_back(v3Position[2]);


								for (int nUvLayer = 0; nUvLayer < nUvReadCount; ++nUvLayer)
								{
									pImportMesh->uvSetList[nUvLayer].push_back((float)pfbxv2UVLayers[nUvLayer][i].mData[0]);
									pImportMesh->uvSetList[nUvLayer].push_back(1.0f - (float)pfbxv2UVLayers[nUvLayer][i].mData[1]);									
								}

								for (int nColourSetList = 0; nColourSetList < pImportMesh->nColourSetCount; nColourSetList++)
								{
									pImportMesh->colourSetList[nColourSetList].push_back((float)ppfbxColor[nColourSetList][i].mRed);
									pImportMesh->colourSetList[nColourSetList].push_back((float)ppfbxColor[nColourSetList][i].mGreen);
									pImportMesh->colourSetList[nColourSetList].push_back((float)ppfbxColor[nColourSetList][i].mBlue);
									pImportMesh->colourSetList[nColourSetList].push_back((float)ppfbxColor[nColourSetList][i].mAlpha);
								}


								FbxVector4 v3Normal = pfbxv4Normals[i];
								v3Normal = fbxMatrix.MultR(v3Normal);

								if (v3Normal.Length() > 0.0000001f)
									v3Normal.Normalize();
								else
									v3Normal.Set(0.0f, 0.0f, 1.0f, 0.0f);

								pImportMesh->normalList.push_back(v3Normal[0]);
								pImportMesh->normalList.push_back(v3Normal[1]);
								pImportMesh->normalList.push_back(v3Normal[2]);

							}
						}
						nVertexOffset += nNumVertices;
					}

					if (pImportMesh->positionList.size() == 0)
					{
						//TA_ASSERT(0); // No verts in the object
						ReportError("Empty object found: %s", pImportMesh->strName.c_str());
						g_importMeshList.pop_back();
					}
				}

				if (pbShownUnMappedError)
					delete [] pbShownUnMappedError;
			}
			break;
        } 
    }
 


    for (int i = 0; i < pNode->GetChildCount(); i++)
    {
        RecurseFbxNodes(pNode->GetChild(i));
    }
}


//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
void RecurseFbxNodesForCollision(FbxNode* pNode)
{
	if (!pNode)
		return;

    FbxString strName = pNode->GetName();
	const char* szName = strName;
 
    FbxNodeAttribute::EType eAttributeType;

    if (pNode->GetNodeAttribute() == NULL)
    {
		int nTest = 0;
    }
    else
    {
        eAttributeType = pNode->GetNodeAttribute()->GetAttributeType();

		FbxAMatrix fbxMatrix = pNode->EvaluateGlobalTransform();


		
		int nObjectId = 0;
		if (strlen(szName) > 3 &&
			szName[0] == 'G' && szName[1] == 'a' && szName[2] == 'p')
		{
			ImportOob* pOob = new ImportOob;
			g_importOobList.push_back(pOob);

			pOob->fbxMatrix = fbxMatrix * g_fScaleBy;			
			pOob->fbxMatrix[3][0] = fbxMatrix[3][0] * g_fScaleBy;
			pOob->fbxMatrix[3][1] = fbxMatrix[3][1] * g_fScaleBy;
			pOob->fbxMatrix[3][2] = fbxMatrix[3][2] * g_fScaleBy;

			strlcpy(pOob->szName, &szName[3], ImportOob::MAX_NAME_LENGTH);
			for (int i = 0; pOob->szName[i]; i++)
			{
				if (pOob->szName[i] == '*') // allow multiply boxes with the same name.
				{
					pOob->szName[i] = 0;
					break;
				}
			}
			return;
		}
		else
		if (strlen(szName) >= 4 &&
			szName[0] == 'R' && szName[1] == 'a' && szName[2] == 'i' && szName[3] == 'l')
		{
			nObjectId = ++g_nObjectId;
			TA_ASSERT(nObjectId <= 255);
			nObjectId &= ATTRIBUTE_FLAG_OBJECT_ID_MASK;
		}
		else
		if (strlen(szName) >= 5 &&
			szName[0] == 'L' && szName[1] == 'e' && szName[2] == 'd' && szName[3] == 'g' && szName[4] == 'e')
		{
			nObjectId = ++g_nObjectId;
			TA_ASSERT(nObjectId <= 255);
			nObjectId &= ATTRIBUTE_FLAG_OBJECT_ID_MASK;
		}
  


		switch (eAttributeType)
        {
        case FbxNodeAttribute::eMesh:
			{
				FbxMesh* pMesh = (FbxMesh*)pNode->GetNodeAttribute();
				int nNumPolygons = pMesh->GetPolygonCount();

				int nOffset = (int)g_importCollisionMesh.positionList.size() / 3;

				for (int nVertex = 0; nVertex < pMesh->GetControlPointsCount(); nVertex++)
				{

					FbxVector4 v4Position = pMesh->GetControlPointAt(nVertex);
					v4Position = fbxMatrix.MultT(v4Position);
					v4Position *= g_fScaleBy;

					g_importCollisionMesh.positionList.push_back(v4Position[0]);
					g_importCollisionMesh.positionList.push_back(v4Position[1]);
					g_importCollisionMesh.positionList.push_back(v4Position[2]);
				}				

				FbxLayerElementArrayTemplate<int>* pMaterialIndicies = 0;
				pMesh->GetMaterialIndices(&pMaterialIndicies);




				for (int nPolygon = 0; nPolygon < nNumPolygons; nPolygon++)
				{
					int nNumVertices = pMesh->GetPolygonSize(nPolygon);					

					FbxSurfaceMaterial* pMaterial = pNode->GetMaterial(pMaterialIndicies->GetAt(nPolygon));
					u32 nPolygonAttribute = (u32)(size_t)pMaterial->GetUserDataPtr();
					nPolygonAttribute |= nObjectId;
					if ((nPolygonAttribute & ATTRIBUTE_GRIND_EDGE_WIDE) ||
						(nPolygonAttribute & ATTRIBUTE_GRIND_EDGE_RAIL) ||
						(nPolygonAttribute & ATTRIBUTE_GRIND_LIP))
					{
						if (nNumVertices != 4)
							continue;
						int nVertexA = pMesh->GetPolygonVertex(nPolygon, 0);
						int nVertexB = pMesh->GetPolygonVertex(nPolygon, 1);
						int nVertexC = pMesh->GetPolygonVertex(nPolygon, 2);
						int nVertexD = pMesh->GetPolygonVertex(nPolygon, 3);




						FbxVector4 v3A(g_importCollisionMesh.positionList[nVertexA * 3 + 0 + nOffset * 3], g_importCollisionMesh.positionList[nVertexA * 3 + 1 + nOffset * 3], g_importCollisionMesh.positionList[nVertexA * 3 + 2 + nOffset * 3], 0.0);
						FbxVector4 v3B(g_importCollisionMesh.positionList[nVertexB * 3 + 0 + nOffset * 3], g_importCollisionMesh.positionList[nVertexB * 3 + 1 + nOffset * 3], g_importCollisionMesh.positionList[nVertexB * 3 + 2 + nOffset * 3], 0.0);
						FbxVector4 v3C(g_importCollisionMesh.positionList[nVertexC * 3 + 0 + nOffset * 3], g_importCollisionMesh.positionList[nVertexC * 3 + 1 + nOffset * 3], g_importCollisionMesh.positionList[nVertexC * 3 + 2 + nOffset * 3], 0.0);
						FbxVector4 v3D(g_importCollisionMesh.positionList[nVertexD * 3 + 0 + nOffset * 3], g_importCollisionMesh.positionList[nVertexD * 3 + 1 + nOffset * 3], g_importCollisionMesh.positionList[nVertexD * 3 + 2 + nOffset * 3], 0.0);

						FbxVector4 v3Cross = (v3A - v3B).CrossProduct(v3A - v3D);
						if (fabsf(v3A[0] - v3B[0]) + fabsf(v3A[2] - v3B[2]) > fabsf(v3A[0] - v3D[0]) + fabsf(v3A[2] - v3D[2]))
						{
							FbxVector4 v3Tmp = v3D;
							v3D = v3C;
							v3C = v3B;
							v3B = v3A;
							v3A = v3Tmp;
						}
						double fHeight = fabs(v3B[1] - v3A[1]);
						if (v3A[1] > v3B[1])
							v3A[1] = v3B[1];
						if (v3C[1] > v3D[1])
							v3C[1] = v3D[1];

						if (v3Cross.DotProduct((v3A - v3C).CrossProduct(FbxVector4(0.0, 1.0, 0.0, 0.0))) < 0.0)
						{
							FbxVector4 v3Tmp = v3A;
							v3A = v3C;
							v3C = v3Tmp;
						}



						if ((v3A - v3C).Length() > 0.00001f)
						{
							g_importCollisionMesh.grindEdgeAttributeList.push_back(nPolygonAttribute);
							g_importCollisionMesh.grindEdgeList.push_back(v3A[0]);
							g_importCollisionMesh.grindEdgeList.push_back(v3A[1]);
							g_importCollisionMesh.grindEdgeList.push_back(v3A[2]);
							g_importCollisionMesh.grindEdgeList.push_back(v3C[0]);
							g_importCollisionMesh.grindEdgeList.push_back(v3C[1]);
							g_importCollisionMesh.grindEdgeList.push_back(v3C[2]);
						}


						if ((nPolygonAttribute & ATTRIBUTE_GRIND_EDGE_WIDE) ||
							(nPolygonAttribute & ATTRIBUTE_GRIND_EDGE_RAIL))
						{
							if (nPolygonAttribute & ATTRIBUTE_GRIND_POP_OFF)
							{
								int nOffset = (int)g_importCollisionMesh.positionList.size() / 3;
						
								g_importCollisionMesh.positionList.push_back(v3A[0]);
								g_importCollisionMesh.positionList.push_back(v3A[1] + fHeight * 2.0f);
								g_importCollisionMesh.positionList.push_back(v3A[2]);
								g_importCollisionMesh.positionList.push_back(v3A[0]);
								g_importCollisionMesh.positionList.push_back(v3A[1] - fHeight * 0.5f);
								g_importCollisionMesh.positionList.push_back(v3A[2]);
								g_importCollisionMesh.positionList.push_back(v3C[0]);
								g_importCollisionMesh.positionList.push_back(v3C[1] + fHeight * 2.0f);
								g_importCollisionMesh.positionList.push_back(v3C[2]);
								g_importCollisionMesh.positionList.push_back(v3C[0]);
								g_importCollisionMesh.positionList.push_back(v3C[1] - fHeight * 0.5f);
								g_importCollisionMesh.positionList.push_back(v3C[2]);

								g_importCollisionMesh.polygonNumVerticesList.push_back(4);
								g_importCollisionMesh.polygonAttirbuteList.push_back(ATTRIBUTE_GRIND_POP_OFF);	
								g_importCollisionMesh.polygonIndiciesList.push_back(0 + nOffset);			
								g_importCollisionMesh.polygonIndiciesList.push_back(1 + nOffset);			
								g_importCollisionMesh.polygonIndiciesList.push_back(3 + nOffset);			
								g_importCollisionMesh.polygonIndiciesList.push_back(2 + nOffset);
							}
							continue; 
						}
					}

					g_importCollisionMesh.polygonNumVerticesList.push_back(nNumVertices);
					g_importCollisionMesh.polygonAttirbuteList.push_back(nPolygonAttribute);	

					for (int nVertex = 0; nVertex < nNumVertices; nVertex++)
					{
						int nVertexId = pMesh->GetPolygonVertex(nPolygon, nNumVertices - nVertex - 1);
						g_importCollisionMesh.polygonIndiciesList.push_back(nVertexId + nOffset);							
					}
				}	
			}
			break;
        } 
    }
 


    for (int i = 0; i < pNode->GetChildCount(); i++)
    {
        RecurseFbxNodesForCollision(pNode->GetChild(i));
    }
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
std::vector<ImportTexture*> g_decalLightmapTextures;

ImportTexture* GetDecalLightmapTexture(int nLightmapId, ImportMesh* pMesh)
{
	for (int i = 0; i < g_decalLightmapTextures.size(); ++i)
	{
		if (pMesh && g_importTextureList[pMesh->nTextureIndex]->bLayeredTexture)
		{			
			if (g_importTextureList[pMesh->nTextureIndex]->textureArray.size() > g_decalLightmapTextures[i]->textureArray.size())
				continue;

			int j;
			for (j = 0; j < g_decalLightmapTextures[i]->textureArray.size()- 1; j++)
			{
				if (g_decalLightmapTextures[i]->textureArray[i] != g_importTextureList[pMesh->nTextureIndex]->textureArray[i])
					break;
			}
			if (j != g_decalLightmapTextures[i]->textureArray.size() - 1)
				continue;

			if (g_decalLightmapTextures[i]->textureArray[j] == nLightmapId)
			{
				return g_decalLightmapTextures[i];
			}
		}
		else
		{
			if (g_decalLightmapTextures[i]->textureArray[0] == pMesh->nTextureIndex
				&& g_decalLightmapTextures[i]->textureArray[1] == nLightmapId)
			{
				return g_decalLightmapTextures[i];
			}
		}
	}

	ImportTexture* pTextureNew = new ImportTexture;		
	pTextureNew->nId = -1;
	pTextureNew->strFileName = "";
	pTextureNew->strName = pMesh->strName;
	pTextureNew->bLayeredTexture = true;
	pTextureNew->bDecal = true;
	if (pMesh && g_importTextureList[pMesh->nTextureIndex]->bLayeredTexture)
	{
		for (int i = 0; i < g_importTextureList[pMesh->nTextureIndex]->textureArray.size() && i < 2; i++)
			pTextureNew->textureArray.push_back(g_importTextureList[pMesh->nTextureIndex]->textureArray[i]);
	}
	else
	{
		pTextureNew->textureArray.push_back(pMesh->nTextureIndex);
	}
	pTextureNew->textureArray.push_back(nLightmapId);

//	if (pMesh)
//		pMesh->nTextureIndex = (int)g_importTextureList.size();

	pTextureNew->nId = (int)g_importTextureList.size();

	g_importTextureList.push_back(pTextureNew);
	g_decalLightmapTextures.push_back(pTextureNew);

	return pTextureNew;
}

struct Found // found lightmap for vertex
{
	ImportTexture* pLightmapTexture;
	struct Vertex
	{
		bool bFound;
		float u;
		float v;
		float fScore;

		Vertex()
		{
			bFound = false;
			u = 0.0f;
			v = 0.0f;
			fScore = -FLT_MAX;
		}

	};
	Vertex pVert[3];

	Found()
	{
		pLightmapTexture = 0;
	}
};

std::vector<ImportMesh*> g_decalMeshes;
void AddPolygonToDecalList(ImportMesh* pMesh, int nVertId, int nTextureId)
{
	ImportMesh* pToAdd = 0;
	for (int i = 0; i < g_decalMeshes.size(); ++i)
	{
		if (g_decalMeshes[i]->nTextureIndex == nTextureId)
		{
			pToAdd = g_decalMeshes[i];
			break;
		}
	}

	if (!pToAdd)
	{
		pToAdd = new ImportMesh();
		char szId[16];
		sprintf(szId, "%d_", nTextureId);
		pToAdd->strName = szId + pMesh->strName;
		pToAdd->nTextureIndex = nTextureId;
		pToAdd->nColourSetCount = pMesh->nColourSetCount;
		pToAdd->nUvSetCount = pMesh->nUvSetCount;
		g_decalMeshes.push_back(pToAdd);
	}

/*
	std::vector<float> positionList;
	std::vector<float> colourSetList[MAX_COLOUR_LISTS];	// 	red green blue alpha
	std::vector<float> uvSetList[MAX_UV_LISTS];	
	std::vector<float> normalList;
*/
	int nPositionIndex = nVertId * 3;
	if (pMesh->positionList.size() > nVertId)
	{
		for (int i = 0; i < 3; ++i)
		{
			pToAdd->positionList.push_back(pMesh->positionList[nPositionIndex + i]);			
		}
	}

	int nNormalIndex = nVertId * 3;
	if (pMesh->normalList.size() > nVertId)
	{
		for (int i = 0; i < 3; ++i)
		{
			pToAdd->normalList.push_back(pMesh->normalList[nPositionIndex + i]);			
		}
	}

	int nColourIndex = nVertId * 4;
	for (int nSet = 0; nSet < MAX_COLOUR_LISTS; ++nSet)
	{
		if (pMesh->colourSetList[nSet].size() > nVertId)
		{
			for (int i = 0; i < 4; ++i)
				pToAdd->colourSetList[nSet].push_back(pMesh->colourSetList[nSet][nColourIndex + i]);
		}
	}

	int nUVIndex = nVertId * 2;
	for (int nSet = 0; nSet < MAX_UV_LISTS; ++nSet)
	{
		if (pMesh->uvSetList[nSet].size() > nVertId)
		{
			for (int i = 0; i < 2; ++i)
				pToAdd->uvSetList[nSet].push_back(pMesh->uvSetList[nSet][nUVIndex + i]);
		}
	}
}

void UpdateDecalLightMapping()
{
	std::vector<int> meshRemoveList;
	for (size_t nMesh = 0; nMesh < g_importMeshList.size(); ++nMesh)
	{
		ImportMesh* pMesh = g_importMeshList[nMesh];
		const char* szName = pMesh->strName.c_str();
		bool bAutoGenerateLightmapUVs = (pMesh->nUvSetCount == 1) && (stristr(szName, "decal") || stristr(szName, "signage")) ;
		
		int nHack = 0;
		if (bAutoGenerateLightmapUVs)
		{
			meshRemoveList.push_back(nMesh);

			if (pMesh->uvSetList[1].size() == 0)
			{
				for (size_t i = 0; i < (pMesh->positionList.size() * 2) / 3; ++i)
				{
					pMesh->uvSetList[1].push_back(0.0f);
				}
			}

			if (pMesh->nUvSetCount == 1)
				pMesh->nUvSetCount = 2;

			int nInnerFaceCount = pMesh->positionList.size() / 9;
			for (int nInnerFace = 0; nInnerFace < nInnerFaceCount; ++nInnerFace)
			{
				std::vector<Found> foundList;
				

				FbxVector4 v3CenterInner;
				FbxVector4 v3NormalInner;
				{
					FbxVector4 v3A, v3B, v3C;
					v3A[0] = pMesh->positionList[nInnerFace * 9 + 0];
					v3A[1] = pMesh->positionList[nInnerFace * 9 + 1];
					v3A[2] = pMesh->positionList[nInnerFace * 9 + 2];

					v3B[0] = pMesh->positionList[nInnerFace * 9 + 3];
					v3B[1] = pMesh->positionList[nInnerFace * 9 + 4];
					v3B[2] = pMesh->positionList[nInnerFace * 9 + 5];

					v3C[0] = pMesh->positionList[nInnerFace * 9 + 6];
					v3C[1] = pMesh->positionList[nInnerFace * 9 + 7];
					v3C[2] = pMesh->positionList[nInnerFace * 9 + 8];

					v3NormalInner = (v3B - v3A).CrossProduct(v3C - v3A);
					v3NormalInner.Normalize();

					v3CenterInner = (v3A + v3B + v3C) * (1.0f / 3.0f);
				}

					
				for (size_t nMeshTest = 0; nMeshTest < g_importMeshList.size(); ++nMeshTest)
				{
					if (nMeshTest == nMesh)
						continue;

					ImportMesh* pMeshTest = g_importMeshList[nMeshTest];	
					const char* szNameTest = pMeshTest->strName.c_str();
					if (stristr(szNameTest, "decal") || stristr(szNameTest, "signage") || stristr(szNameTest, "alpha"))
						continue;

					if (pMeshTest->nUvSetCount < 2)
						continue;

					int nUvLightmap = pMeshTest->nUvSetCount - 1;

					int nLightmapId = g_importTextureList[pMeshTest->nTextureIndex]->textureArray[2];
					ImportTexture* pLightmapTexture = GetDecalLightmapTexture(nLightmapId, pMesh);

					int nFaceCount = pMeshTest->positionList.size() / 9;
					for (int nFace = 0; nFace < nFaceCount; ++nFace)
					{
						FbxVector4 v3A, v3B, v3C;
						v3A[0] = pMeshTest->positionList[nFace * 9 + 0];
						v3A[1] = pMeshTest->positionList[nFace * 9 + 1];
						v3A[2] = pMeshTest->positionList[nFace * 9 + 2];

						v3B[0] = pMeshTest->positionList[nFace * 9 + 3];
						v3B[1] = pMeshTest->positionList[nFace * 9 + 4];
						v3B[2] = pMeshTest->positionList[nFace * 9 + 5];

						v3C[0] = pMeshTest->positionList[nFace * 9 + 6];
						v3C[1] = pMeshTest->positionList[nFace * 9 + 7];
						v3C[2] = pMeshTest->positionList[nFace * 9 + 8];

						FbxVector4 v3Center = (v3A + v3B + v3C) * (1.0f / 3.0f);

						FbxVector4 v3Normal = (v3B - v3A).CrossProduct(v3C - v3A);
						v3Normal.Normalize();
						float fNormalDot = v3Normal.DotProduct(v3NormalInner);
						if (fNormalDot < 0.0f)
							continue;


//						int nVertCount = pMesh->positionList.size() / 3;
				//		int nVertFoundCount = 0;
						for (int nVertId = 0; nVertId < 3; ++nVertId)
						{
							int nVert = nInnerFace * 3 + nVertId;

							FbxVector4 v3Point;
							v3Point[0] = pMesh->positionList[nVert * 3 + 0];
							v3Point[1] = pMesh->positionList[nVert * 3 + 1];
							v3Point[2] = pMesh->positionList[nVert * 3 + 2];

							float fDist = (v3Point - v3A).DotProduct(v3Normal);
							float fTolerance = 0.4f;//100.4f;
							float fToleranceNegative = -0.4f;//100.4f;
							if (fDist < fTolerance && fDist >= fToleranceNegative)
							{
								v3Point -= v3Normal * fDist;

								float fDot = (v3A - v3Point).DotProduct(v3Normal);



								FbxVector4 v0 = v3C - v3A;
								FbxVector4 v1 = v3B - v3A;
								FbxVector4 v2 = v3Point - v3A;
								float d00 = v0.DotProduct(v0);
								float d01 = v0.DotProduct(v1);
								float d02 = v0.DotProduct(v2);
								float d11 = v1.DotProduct(v1);
								float d12 = v1.DotProduct(v2);

								float invD = 1.0f / (d00 * d11 - d01 * d01);
								float uTest = (d11 * d02 - d01 * d12) * invD;
								float vTest = (d00 * d12 - d01 * d02) * invD;

								if (uTest >= -0.001f && vTest >= -0.001f
									&& uTest + vTest <= 1.001f)
								{
									// Close enough, grab the lightmap uvs
									float dA = v2.Length();
									float dB = (v3Point - v3B).Length();
									float dC = (v3Point - v3C).Length();
									float dT = 1.0f / (dA + dB + dC);

									float rA = (1.0f - (dA * dT)) * 0.5f;
									float rB = (1.0f - (dB * dT)) * 0.5f;
									float rC = (1.0f - (dC * dT)) * 0.5f;

									float u = 0.0f;
									u += pMeshTest->uvSetList[nUvLightmap][nFace * 6 + 0] * rA;
									u += pMeshTest->uvSetList[nUvLightmap][nFace * 6 + 2] * rB;
									u += pMeshTest->uvSetList[nUvLightmap][nFace * 6 + 4] * rC;

									float v = 0.0f;
									v += pMeshTest->uvSetList[nUvLightmap][nFace * 6 + 1] * rA;
									v += pMeshTest->uvSetList[nUvLightmap][nFace * 6 + 3] * rB;
									v += pMeshTest->uvSetList[nUvLightmap][nFace * 6 + 5] * rC;

									//pMesh->uvSetList[1][nVert * 2 + 0] = u;
									//pMesh->uvSetList[1][nVert * 2 + 1] = v;

									Found* pFound = 0;
									for (int nFound = 0; nFound < foundList.size(); nFound++)
									{
										if (foundList[nFound].pLightmapTexture == pLightmapTexture)
										{
											pFound = &foundList[nFound];
										}
									}
									if (!pFound)
									{
										foundList.push_back(Found());
										pFound = &foundList.back();
										pFound->pLightmapTexture = pLightmapTexture;
									}

									
									float fDistanceA = (v3CenterInner - v3A).DotProduct(GetNormal((v3B - v3A).CrossProduct(v3Normal)));
									float fDistanceB = (v3CenterInner - v3B).DotProduct(GetNormal((v3C - v3B).CrossProduct(v3Normal)));
									float fDistanceC = (v3CenterInner - v3C).DotProduct(GetNormal((v3A - v3C).CrossProduct(v3Normal)));
									float fMaxDistance = MaxF(MaxF(fDistanceA, fDistanceB), fDistanceC);

								//	float fScore = fNormalDot;// - fMaxDistance - fDist * 10.0f;

									
									float fScore = fNormalDot - fDist * 10.0f;

									
									/*if (fDist > 0.0f)
									{
										pMesh->positionList[nVert * 3 + 0] = v3Point.x;
										pMesh->positionList[nVert * 3 + 1] = v3Point.y;
										pMesh->positionList[nVert * 3 + 2] = v3Point.z;
									}*/

									if (fScore > pFound->pVert[nVertId].fScore)
									{
										pFound->pVert[nVertId].bFound = true;
										pFound->pVert[nVertId].u = u;
										pFound->pVert[nVertId].v = v;
										pFound->pVert[nVertId].fScore = fScore;
									}

									

								}
							}
						}
					}
				}
				
				// Find best light map to apply to triangle
				float fBestScore = 0;
				Found* pBestFound = 0;
				for (int nFound = 0; nFound < foundList.size(); nFound++)
				{
					Found& found = foundList[nFound];
					float fScore = 0.0f;
					for (int nVertId = 0; nVertId < 3; nVertId++)
					{
						if (found.pVert[nVertId].bFound)
							fScore += 100000.0f + found.pVert[nVertId].fScore;
					}
					if (fBestScore < fScore)
					{
						fBestScore = fScore;
						pBestFound = &found;
					}
				}

				if (pBestFound)
				{
					Found& found = *pBestFound;
					int nVertToAdd = nInnerFace * 3;
					
					for (int nVertId = 0; nVertId < 3; nVertId++)
					{
						int nVert = nInnerFace * 3 + nVertId;
						if (found.pVert[nVertId].bFound)
						{
							pMesh->uvSetList[1][nVert * 2 + 0] = found.pVert[nVertId].u;
							pMesh->uvSetList[1][nVert * 2 + 1] = found.pVert[nVertId].v;
						}
					}
					
					AddPolygonToDecalList(pMesh, nVertToAdd++, found.pLightmapTexture->nId);
					AddPolygonToDecalList(pMesh, nVertToAdd++, found.pLightmapTexture->nId);
					AddPolygonToDecalList(pMesh, nVertToAdd++, found.pLightmapTexture->nId);
				}
				else
				{
					// couldn't find a light map to add to, so just add to the first light map
					int nVertToAdd = nInnerFace * 3;
					AddPolygonToDecalList(pMesh, nVertToAdd++, g_decalLightmapTextures[0]->nId);
					AddPolygonToDecalList(pMesh, nVertToAdd++, g_decalLightmapTextures[0]->nId);
					AddPolygonToDecalList(pMesh, nVertToAdd++, g_decalLightmapTextures[0]->nId);
				}
				

			}

		}
	}

	std::sort(meshRemoveList.begin(), meshRemoveList.end(), SortRemoveList); // reverst sort the list to remove bigger indices first so everything doesn't get messed up.
	for (int i = 0; i < meshRemoveList.size(); ++i)
	{
		g_importMeshList.erase(g_importMeshList.begin() + meshRemoveList[i]);
	}

	for (int i = 0; i < g_decalMeshes.size(); ++i)
	{
		g_importMeshList.insert(g_importMeshList.begin(), g_decalMeshes[i]);
	}
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
int CalculateHash(float x, float y, float z, u32 nHashSize)
{
	float xy = (x * y);
	float yz = (y * z);
	float zx = (z * x);
	return ((u32&)x + (u32&)y + (u32&)z + (u32&)xy + (u32&)yz + (u32&)zx) % nHashSize;
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
int AddTextureFileName(const char* szTextureFileName)
{		
    const char* szFullPath =szTextureFileName;


		
	char szFileName[_MAX_PATH];
	char szDrive[_MAX_DRIVE];
	char szDir[_MAX_DIR];
	char szName[_MAX_FNAME];
	char szExt[_MAX_EXT];
	_splitpath(szFullPath, szDrive, szDir, szName, szExt);
	_makepath(szFileName, "", "", szName, szExt);

		
	bool bFoundDuplicate = false;
	for (int i = 0; i < (int)g_importTextureList.size(); i++)
	{
		if (strlen(szFileName) > 0 && strcmp(szFileName, g_importTextureList[i]->strFileName.c_str()) == 0)
		{
			return i;
		}
	}

	ImportTexture* pTexture = new ImportTexture;		
	pTexture->nId = g_importTextureList.size();
	pTexture->strFileName = szFileName;
	pTexture->strName = szName;
	pTexture->bLayeredTexture = false;

	g_importTextureList.push_back(pTexture);
	return g_importTextureList.size() - 1;
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
int FindTextureFileName(const char* szTextureFileName)
{	
    const char* szFullPath =szTextureFileName;

	char szFileName[_MAX_PATH];
	char szDrive[_MAX_DRIVE];
	char szDir[_MAX_DIR];
	char szName[_MAX_FNAME];
	char szExt[_MAX_EXT];
	_splitpath(szFullPath, szDrive, szDir, szName, szExt);
	_makepath(szFileName, "", "", szName, szExt);

		
	bool bFoundDuplicate = false;
	for (size_t i = 0; i < g_importTextureList.size(); i++)
	{
		if (strlen(szFileName) > 0 && strcmp(szFileName, g_importTextureList[i]->strFileName.c_str()) == 0)
		{
			return i;
		}
	}

	return 0;
}


//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
	const int nMaxFileName = 1024;
	char szFbxFileNameIn[nMaxFileName] = {};
	char szFbxCollisionFileNameIn[nMaxFileName] = {};
	char szVisFileNameOut[nMaxFileName] = {};
	char szFbxGapFileNameIn[nMaxFileName] = {};

	bool bShowHelp = false;
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-quiet") == 0)
		{
			s_bQuietReporting = true;
		}
		else
		if (strcmp(argv[i], "-vis") == 0)
		{
			i++;
			if (i < argc)
			{
				strlcpy(szFbxFileNameIn, argv[i], nMaxFileName);
			}
			else
			{
				printf("Input error after -vis\n");
				bShowHelp = true;
				break;
			}
		}
		else
		if (strcmp(argv[i], "-col") == 0)
		{
			i++;
			if (i < argc)
			{
				strlcpy(szFbxCollisionFileNameIn, argv[i], nMaxFileName);
			}
			else
			{
				printf("Input error after -col\n");
				bShowHelp = true;
				break;
			}
		}
		else
		if (strcmp(argv[i], "-out") == 0)
		{
			i++;
			if (i < argc)
			{
				strlcpy(szVisFileNameOut, argv[i], nMaxFileName);
			}
			else
			{
				printf("Input error after -out\n");
				bShowHelp = true;
				break;
			}
		}
		else
		if (strcmp(argv[i], "-scale") == 0)
		{
			if (i < argc)
			{
				g_fScaleBy = atof(argv[i]);
			}
			else
			{
				printf("Input error after -scale\n");
				bShowHelp = true;
				break;
			}
		}
		else
		{
			bShowHelp = true;
			break;
		}
	}

	if (bShowHelp)
	{
		printf(
			"about: Converts .fbx files into True Skate skatepark format.\n"
			"useage:\n"
			"fbxToTrueSkate -vis <filename> -col <filename> -out <filename> [-scale <eg 1.23>] [-quite]\n"
			"\n"
			"-vis: Visible geometry .fbx file.\n"
			"-col: Collision geometry .fbx file.\n"
			"-out: Output file.\n"
			"-scale: Scale factor to apply.\n"
			"-quite: Show no status reports during conversion.\n");

		return 0;
	}
	
	
	g_fbxMatrixApplyRotation.SetIdentity();
#ifdef TM_MATRIX
	g_m33ApplyRotation.Clear();
#endif



	//---------------------------------------------------------------------------------
	// Import Fbx file using the FbxSdk
	//---------------------------------------------------------------------------------
		

	


	//---------------------------------------------------------------------------------
	// Import Fbx file using the FbxSdk
	//---------------------------------------------------------------------------------
    FbxManager* g_pFbxManager = 0;
    FbxScene* g_pFbxScene = 0;

	SetStatus("Load vis FBX", 0.0f);
	
	g_pFbxManager = FbxManager::Create();

	FbxIOSettings* pFbxIoSettings = FbxIOSettings::Create(g_pFbxManager, IOSROOT );
	g_pFbxManager->SetIOSettings(pFbxIoSettings);
	
	FbxImporter* pFbxImporter = FbxImporter::Create(g_pFbxManager, "");

	const char* szFilename = szFbxFileNameIn;//

	bool bImportStatus = pFbxImporter->Initialize(szFilename, -1, g_pFbxManager->GetIOSettings());

	if (!bImportStatus) 
		FatalError("Call to FbxImporter::Initialize() failed.\nError returned: \"%s\"", pFbxImporter->GetStatus().GetErrorString());

	g_pFbxScene = FbxScene::Create(g_pFbxManager, "myScene");

	pFbxImporter->Import(g_pFbxScene, true);

	bool bImportSucceded = false;
	while (pFbxImporter->IsImporting(bImportSucceded))
	{
		float fStatus = pFbxImporter->GetProgress() / 100.0f;
		SetStatus("Load vis FBX", fStatus);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	SetStatus("Load vis FBX", 1.0f);

	if (!bImportSucceded) 
	{
		FatalError("Call to FbxImporter::Import() failed.\nError returned: %s\n\n", pFbxImporter->GetStatus().GetErrorString());
	}


	SetStatus("Process vis FBX", 0.0f);

	

	//---------------------------------------------------------------------------------
	// Get what we need from the Fbx file and store in our own internal format
	//---------------------------------------------------------------------------------
	FbxNode* pRootNode = g_pFbxScene->GetRootNode();



		
	

	int nNumTextures = g_pFbxScene->GetTextureCount();
	for (int nTexture = 0; nTexture < nNumTextures; nTexture++)
	{
		FbxTexture* pFbxTexture = g_pFbxScene->GetTexture(nTexture);
		FbxString strTextureType =  pFbxTexture->GetTextureType();
		const char* szTextureType = strTextureType;
		const char* szTextureName = pFbxTexture->GetName();
        FbxFileTexture* pFileTexture = FbxCast<FbxFileTexture>(pFbxTexture);
		if (!pFileTexture)
			continue;

        const char* szFullPath = pFileTexture->GetFileName();
		
		char szFileName[_MAX_PATH];
		char szDrive[_MAX_DRIVE];
		char szDir[_MAX_DIR];
		char szName[_MAX_FNAME];
		char szExt[_MAX_EXT];
		_splitpath(szFullPath, szDrive, szDir, szName, szExt);
		_makepath(szFileName, "", "", szName, szExt);

		
		bool bFoundDuplicate = false;
		for (size_t i = 0; i < g_importTextureList.size(); i++)
		{
			if (strlen(szFileName) > 0 && strcmp(szFileName, g_importTextureList[i]->strFileName.c_str()) == 0)
			{
				pFbxTexture->SetUserDataPtr((void*)(i+1));
				bFoundDuplicate = true;
				break;
			}
		}
		if (bFoundDuplicate)
			continue;

		ImportTexture* pTexture = new ImportTexture;		
		pTexture->nId = g_importTextureList.size();
		pTexture->strFileName = szFileName;
		pTexture->strName = szName;
		pTexture->bLayeredTexture = false;

		g_importTextureList.push_back(pTexture);
		pFbxTexture->SetUserDataPtr((void*)g_importTextureList.size());
	}




	int nNumMaterials = g_pFbxScene->GetMaterialCount();
	for (int nMaterial = 0; nMaterial < nNumMaterials; nMaterial++)
	{
		FbxSurfaceMaterial* pSurfaceMaterial = g_pFbxScene->GetMaterial(nMaterial);
		
		const char* szName = pSurfaceMaterial->GetName();
		
	    const FbxProperty propertySfxTextureBase = pSurfaceMaterial->FindProperty("SfxTextureBase");
		if (propertySfxTextureBase.IsValid())
		{
			FbxString strSfxTextureBase = propertySfxTextureBase.Get<FbxString>();
			
			AddTextureFileName(strSfxTextureBase);

			const FbxProperty propertySfxTextureDamage = pSurfaceMaterial->FindProperty("SfxTextureOverlay");
			if (propertySfxTextureDamage.IsValid())
			{
				FbxString strSfxTextureDamage = propertySfxTextureDamage.Get<FbxString>();
				AddTextureFileName(strSfxTextureDamage);
			}
			else
			{
				const FbxProperty propertySfxTextureSpecular = pSurfaceMaterial->FindProperty("SfxTextureSpecular");
				if (propertySfxTextureSpecular.IsValid())
				{
					FbxString strSfxTextureSpecular = propertySfxTextureSpecular.Get<FbxString>();
					AddTextureFileName(strSfxTextureSpecular);
				}
			}

			const FbxProperty propertySfxTextureLightMap = pSurfaceMaterial->FindProperty("SfxTextureLightmap");
			if (propertySfxTextureBase.IsValid())
			{
				FbxString strTextureLightMap = propertySfxTextureLightMap.Get<FbxString>();					
				AddTextureFileName(strTextureLightMap);
			}
			
			continue;

		}
	}

	for (int nTexture = 0; nTexture < nNumTextures; nTexture++)
	{
		FbxTexture* pFbxTexture = g_pFbxScene->GetTexture(nTexture);
		FbxString strTextureType =  pFbxTexture->GetTextureType();
		const char* szTextureType = strTextureType;
		const char* szTextureName = pFbxTexture->GetName();
		FbxLayeredTexture* pLayeredTexture = FbxCast<FbxLayeredTexture>(pFbxTexture);
		if (!pLayeredTexture)
			continue;
		

		ImportTexture* pTexture = new ImportTexture;		
		pTexture->nId = -1;
		pTexture->strFileName = "";
		pTexture->strName = "";
		pTexture->bLayeredTexture = true;

		int nLayerCount = pLayeredTexture->GetSrcObjectCount();
		for (int nLayer = 0; nLayer < nLayerCount; nLayer++)
		{
			FbxFileTexture* pFileTexture = FbxCast<FbxFileTexture>(pLayeredTexture->GetSrcObject(FbxCriteria::ObjectTypeStrict(FbxFileTexture::ClassId), nLayer));
			const char* szFullPath = pFileTexture->GetFileName();
			size_t nOtherTexture;
			// hack sls_signage_universal_c
			/*if (nLayer == 1 && strstr(szFullPath, "sls_signage_universal"))
			{
				nOtherTexture = 0;
			}
			else*/
			{
				nOtherTexture = (size_t)pFileTexture->GetUserDataPtr() - 1;

				if (nOtherTexture < 0 || nOtherTexture >= g_importTextureList.size())
				{
					TA_ASSERT(0);
					FatalError("Couldn't find texture %s", szFullPath);
				}
			}

			pTexture->textureArray.push_back(nOtherTexture);
		}
	
		pTexture->nId = g_importTextureList[pTexture->textureArray[0]]->nId;
		g_importTextureList.push_back(pTexture);
		pFbxTexture->SetUserDataPtr((void*)g_importTextureList.size());

	}

	


	nNumMaterials = g_pFbxScene->GetMaterialCount();
	for (int nMaterial = 0; nMaterial < nNumMaterials; nMaterial++)
	{
		FbxSurfaceMaterial* pSurfaceMaterial = g_pFbxScene->GetMaterial(nMaterial);
		
		const char* szName = pSurfaceMaterial->GetName();
		
	    const FbxProperty propertySfxTextureBase = pSurfaceMaterial->FindProperty("SfxTextureBase");
		if (propertySfxTextureBase.IsValid())
		{
			FbxString strSfxTextureBase = propertySfxTextureBase.Get<FbxString>();

			ImportTexture* pTexture = new ImportTexture;		
			pTexture->nId = -1;
			pTexture->strFileName = "";
			pTexture->strName = "";
			pTexture->bLayeredTexture = true;

			pTexture->textureArray.push_back(FindTextureFileName(strSfxTextureBase));

			const FbxProperty propertySfxTextureDamage = pSurfaceMaterial->FindProperty("SfxTextureOverlay");
			if (propertySfxTextureDamage.IsValid())
			{
				FbxString strSfxTextureDamage = propertySfxTextureDamage.Get<FbxString>();
				pTexture->textureArray.push_back(FindTextureFileName(strSfxTextureDamage));
			}
			else
			{
				const FbxProperty propertySfxTextureSpecular = pSurfaceMaterial->FindProperty("SfxTextureSpecular");
				if (propertySfxTextureSpecular.IsValid())
				{
					FbxString strSfxTextureSpecular = propertySfxTextureSpecular.Get<FbxString>();
					pTexture->textureArray.push_back(FindTextureFileName(strSfxTextureSpecular));
				}
			}

			const FbxProperty propertySfxTextureLightMap = pSurfaceMaterial->FindProperty("SfxTextureLightmap");

			if (propertySfxTextureBase.IsValid())
			{
				FbxString strTextureLightMap = propertySfxTextureLightMap.Get<FbxString>();
				pTexture->textureArray.push_back(FindTextureFileName(strTextureLightMap));
			}

#define GetFloatProperty(name) \
	const FbxProperty property##name = pSurfaceMaterial->FindProperty(#name);\
	if (property##name.IsValid())\
	{\
		pTexture->f##name = property##name.Get<FbxFloat>();\
	}

#define GetColorProperty(name) \
	const FbxProperty property##name = pSurfaceMaterial->FindProperty(#name);\
	if (property##name.IsValid())\
	{\
		FbxColor color = property##name.Get<FbxColor>();\
		pTexture->pf##name[0] = color.mRed;\
		pTexture->pf##name[1] = color.mGreen;\
		pTexture->pf##name[2] = color.mBlue;\
		pTexture->pf##name[3] = color.mAlpha;\
	}
			
			GetColorProperty(Color);
			GetFloatProperty(Specular);

			GetFloatProperty(BlendSharpness_G);
			GetFloatProperty(BlendLevel_G);
			GetFloatProperty(BlendMode_G);
			GetColorProperty(ShadowColor_G);
			GetColorProperty(HighlightColor_G);
			GetFloatProperty(IgnoreBaseColor_G);
			GetFloatProperty(Specular_G);

			GetFloatProperty(BlendSharpness_B);
			GetFloatProperty(BlendLevel_B);
			GetFloatProperty(BlendMode_B);
			GetColorProperty(ShadowColor_B);
			GetColorProperty(HighlightColor_B);
			GetFloatProperty(IgnoreBaseColor_B);
			GetFloatProperty(Specular_B);


#undef GetFloatProperty
	

			
			pTexture->nId = g_importTextureList[pTexture->textureArray[0]]->nId;
			g_importTextureList.push_back(pTexture);
			pSurfaceMaterial->SetUserDataPtr((void*)g_importTextureList.size());
			continue;

		}


		

		int nTextureIndex = 0;
		const FbxDouble3 lDiffuse = GetMaterialProperty(
			pSurfaceMaterial,
			FbxSurfaceMaterial::sDiffuse, 
			FbxSurfaceMaterial::sDiffuseFactor, 
			&nTextureIndex);
		
	//	TA_ASSERT(nTextureIndex);
		pSurfaceMaterial->SetUserDataPtr((void*)(size_t)nTextureIndex);
	}

	RecurseFbxNodes(pRootNode);
	UpdateDecalLightMapping();

	int nTotalNumVerticies = 0;
	for (int i = 0; i < (int)g_importMeshList.size(); i++)
	{
		ImportMesh& importMesh = *g_importMeshList[i];
		nTotalNumVerticies += (int)importMesh.positionList.size() / 3;
	}
	
	SetStatus("Process vis FBX", 1.0f);
	int nCurentVertex = 0;
	SetStatus("Resort vis vertices", 0.0f);
	if (g_importTextureList.size())
	{
		bool bFirstResort = true;
		for (int nTexture = 0; nTexture < (int)g_importTextureList.size(); nTexture++)
		{
			Mesh* pMesh = new Mesh;
//			pMesh->nUvSetCount = g_importTextureList[nTexture]->bLayeredTexture ? 2 : 1;
			pMesh->nUvSetCount = 1;
			pMesh->bDecal = false;
			pMesh->nPrimitiveType = PRIMITIVE_TYPE_STRIP;
			g_meshList.push_back(pMesh);
			
			//const u32 nHashSize = 1031;
			const u32 nHashSize = 16661;
			//const u32 nHashSize = 65327;
			std::vector<int> vertexHashTable[nHashSize];

			for (int i = 0; i < (int)g_importMeshList.size(); i++)
			{
				ImportMesh& importMesh = *g_importMeshList[i];
				if (importMesh.nTextureIndex != nTexture)
					continue;
				
				pMesh->nColourSetCount = importMesh.nColourSetCount;
				pMesh->nUvSetCount = importMesh.nUvSetCount;
				break;
			}
			
			SetListsOnly(false);
			if (!g_bPrecalculateTransparancyOrder)
			{
				ImportTexture& importTexture = *g_importTextureList[nTexture];
				if (!importTexture.bDecal)
				{
					if (strstr(importTexture.strFileName.c_str(), "Alpha") || 
						strstr(importTexture.strFileName.c_str(), "alpha") ||
						(importTexture.textureArray.size() && strstr(g_importTextureList[importTexture.textureArray[0]]->strFileName.c_str(), "Alpha")) || 
						(importTexture.textureArray.size() && strstr(g_importTextureList[importTexture.textureArray[0]]->strFileName.c_str(), "alpha")))
					{
						SetListsOnly(true);
						pMesh->nPrimitiveType = PRIMITIVE_TYPE_TRIANGLES;
					}
				}
			}

			for (int i = 0; i < (int)g_importMeshList.size(); i++)
			{
				ImportMesh& importMesh = *g_importMeshList[i];
				if (importMesh.nTextureIndex != nTexture)
					continue;

				int nIndexStart = (int)pMesh->triagnleIndexList.size();

				bool bHasReportedMissingUVSet = false;
				bool bHasReportedMissingColorSet = false;

				ImportTexture& importTexture = *g_importTextureList[nTexture];
				bool bHasLightmapTexture = false;
				if (importMesh.nUvSetCount > 1)
				{
					if (strstr(importTexture.strFileName.c_str(), "Lightmap") || 
						strstr(importTexture.strFileName.c_str(), "lightmap") ||
						(importTexture.textureArray.size() && strstr(g_importTextureList[importTexture.textureArray[importTexture.textureArray.size()-1]]->strFileName.c_str(), "Lightmap")) || 
						(importTexture.textureArray.size() && strstr(g_importTextureList[importTexture.textureArray[importTexture.textureArray.size()-1]]->strFileName.c_str(), "lightmap")))
					{
						bHasLightmapTexture = true;
					}
				}		
		
				
				int nLastProgress = 0;
				if (bFirstResort)
					bFirstResort = false;
				else
					printf("\n");
				SetStatus("Resort vis vertices", 0);
				for (int nVertexIn = 0; nVertexIn < (int)importMesh.positionList.size() / 3; nVertexIn++)
				{
					int nProgress = (int)(100 * (float)nCurentVertex / (float)nTotalNumVerticies);
					if (nLastProgress != nProgress)
					{
						nLastProgress = nProgress;
						SetStatus("Resort vis vertices", (float)nCurentVertex / (float)nTotalNumVerticies);
					}
					nCurentVertex++;

					int nHash = CalculateHash(importMesh.positionList[nVertexIn * 3 + 0], importMesh.positionList[nVertexIn * 3 + 1],importMesh.positionList[nVertexIn * 3 + 2], nHashSize);				
					int nVertexOut =  pMesh->positionList.size() / 3;
					for (int nHashEntry = 0; nHashEntry < (int)vertexHashTable[nHash].size(); nHashEntry++)
					{
						int nHashVertex = vertexHashTable[nHash][nHashEntry];
						if (nHashEntry > 8)
							int nTest = 0;



						if (nHashVertex != nVertexIn &&
							pMesh->positionList[nHashVertex * 3 + 0] == importMesh.positionList[nVertexIn * 3 + 0] &&
							pMesh->positionList[nHashVertex * 3 + 1] == importMesh.positionList[nVertexIn * 3 + 1] &&
							pMesh->positionList[nHashVertex * 3 + 2] == importMesh.positionList[nVertexIn * 3 + 2] &&
							pMesh->nColourSetCount == importMesh.nColourSetCount &&
							pMesh->nUvSetCount == importMesh.nUvSetCount &&
							pMesh->normalList[nHashVertex * 3 + 0] == importMesh.normalList[nVertexIn * 3 + 0] &&
							pMesh->normalList[nHashVertex * 3 + 1] == importMesh.normalList[nVertexIn * 3 + 1] &&
							pMesh->normalList[nHashVertex * 3 + 2] == importMesh.normalList[nVertexIn * 3 + 2])
						{
							int nColourLayer;
							for (nColourLayer = 0; nColourLayer < pMesh->nColourSetCount; ++nColourLayer)
							{
								if (importMesh.colourSetList[nColourLayer].size() == 0 ||
									(nHashVertex * 4 + 3) > pMesh->colourSetList[nColourLayer].size() ||
									pMesh->colourSetList[nColourLayer][nHashVertex * 4 + 0] != importMesh.colourSetList[nColourLayer][nVertexIn * 4 + 0] ||
									pMesh->colourSetList[nColourLayer][nHashVertex * 4 + 1] != importMesh.colourSetList[nColourLayer][nVertexIn * 4 + 1] ||
									pMesh->colourSetList[nColourLayer][nHashVertex * 4 + 2] != importMesh.colourSetList[nColourLayer][nVertexIn * 4 + 2] ||
									pMesh->colourSetList[nColourLayer][nHashVertex * 4 + 3] != importMesh.colourSetList[nColourLayer][nVertexIn * 4 + 3])
								{
									break;
								}
							}
							if (nColourLayer != pMesh->nColourSetCount)
								break;
							int nUvLayer;
							for (nUvLayer = 0; nUvLayer < pMesh->nUvSetCount; ++nUvLayer)
							{
								if (importMesh.uvSetList[nUvLayer].size() == 0 ||
									pMesh->uvSetList[nUvLayer][nHashVertex * 2 + 0] != importMesh.uvSetList[nUvLayer][nVertexIn * 2 + 0] ||
									pMesh->uvSetList[nUvLayer][nHashVertex * 2 + 1] != importMesh.uvSetList[nUvLayer][nVertexIn * 2 + 1])
								{
									break;
								}
							}
							if (nUvLayer != pMesh->nUvSetCount)
								break;
							nVertexOut = nHashVertex;
							break;
						}
					}


					if (nVertexOut == pMesh->positionList.size() / 3)
					{
						//u32 nHash = CalculateHash(importMesh.positionList[nVertexIn * 3 + 0], importMesh.positionList[nVertexIn * 3 + 1],importMesh.positionList[nVertexIn * 3 + 2], nHashSize);
						vertexHashTable[nHash].push_back(nVertexOut);
						pMesh->positionList.push_back(importMesh.positionList[nVertexIn * 3 + 0]);
						pMesh->positionList.push_back(importMesh.positionList[nVertexIn * 3 + 1]);
						pMesh->positionList.push_back(importMesh.positionList[nVertexIn * 3 + 2]);

						for (int nUvLayer = 0; nUvLayer < pMesh->nUvSetCount; ++nUvLayer)
						{
							if (importMesh.uvSetList[nUvLayer].size() > 0)
							{
								pMesh->uvSetList[nUvLayer].push_back(importMesh.uvSetList[nUvLayer][nVertexIn * 2 + 0]);
								pMesh->uvSetList[nUvLayer].push_back(importMesh.uvSetList[nUvLayer][nVertexIn * 2 + 1]);
							}
							else
							{
								if (!bHasReportedMissingUVSet)
								{
									bHasReportedMissingUVSet = true;
									ReportError("Mesh %s is missing uv sets", importMesh.strName.c_str());
								}
								pMesh->uvSetList[nUvLayer].push_back(0.0f);
								pMesh->uvSetList[nUvLayer].push_back(0.0f);
							}
						}
						
						for (int nColourLayer = 0; nColourLayer < pMesh->nColourSetCount; ++nColourLayer)
						{
							if (importMesh.colourSetList[nColourLayer].size() > 0)
							{
								pMesh->colourSetList[nColourLayer].push_back(importMesh.colourSetList[nColourLayer][nVertexIn * 4 + 0]);
								pMesh->colourSetList[nColourLayer].push_back(importMesh.colourSetList[nColourLayer][nVertexIn * 4 + 1]);
								pMesh->colourSetList[nColourLayer].push_back(importMesh.colourSetList[nColourLayer][nVertexIn * 4 + 2]);
								pMesh->colourSetList[nColourLayer].push_back(importMesh.colourSetList[nColourLayer][nVertexIn * 4 + 3]);
							}
							else
							{
								if (!bHasReportedMissingColorSet)
								{
									bHasReportedMissingColorSet = true;
									ReportError("Mesh %s is missing color sets", importMesh.strName.c_str());
									pMesh->colourSetList[nColourLayer].push_back(1.0f);
									pMesh->colourSetList[nColourLayer].push_back(1.0f);
									pMesh->colourSetList[nColourLayer].push_back(1.0f);
									pMesh->colourSetList[nColourLayer].push_back(1.0f);
								}
							}
						}

						pMesh->normalList.push_back(importMesh.normalList[nVertexIn * 3 + 0]);
						pMesh->normalList.push_back(importMesh.normalList[nVertexIn * 3 + 1]);
						pMesh->normalList.push_back(importMesh.normalList[nVertexIn * 3 + 2]);



					}
					TA_ASSERT(nVertexOut <= 0xFFFF);
					if (nVertexOut > 0xFFFF)
						FatalError("Index out of bounds, over 65535 limit");
		
					pMesh->triagnleIndexList.push_back((u16)nVertexOut);
				}
				SetStatus("Resort vis vertices", 1.0f);

				int nIndexEnd = (int)pMesh->triagnleIndexList.size();
	
				if (!importTexture.bDecal &&  (g_bPrecalculateTransparancyOrder || g_bOrderTransparancy) && (strstr(g_importTextureList[nTexture]->strFileName.c_str(), "Alpha") || strstr(g_importTextureList[nTexture]->strFileName.c_str(), "alpha")))
				{
					PrimitiveGroup* pPrimitiveGroups;
					u16 nNumPrimitiveGroups = 0;
					DisableRestart();
					SetStitchStrips(false);
					bool bGenerateStripsResult = GenerateStrips(&pMesh->triagnleIndexList[nIndexStart], nIndexEnd - nIndexStart, &pPrimitiveGroups, &nNumPrimitiveGroups, true);
					SetStitchStrips(true);
					TA_ASSERT(bGenerateStripsResult);
					
					float xDistanceFrom = 0.0f;
					float yDistanceFrom = 0.0f;
					float zDistanceFrom = 0.0f;

					std::vector<SortPrimitives> sortPrimitivesList;
					for (int nGroup = 0; nGroup < nNumPrimitiveGroups; nGroup++)
					{
						SortPrimitives sortPrimitives;
						sortPrimitives.pPrimitiveGroup = &pPrimitiveGroups[nGroup];
						float fMax = 10000000000000000000.0f;
						float fMaxX = -fMax;
						float fMinX = fMax;
						float fMaxY = -fMax;
						float fMinY = fMax;
						float fMaxZ = -fMax;
						float fMinZ = fMax;
						for (size_t i = 0; i < sortPrimitives.pPrimitiveGroup->numIndices; i++)
						{
							int nIndex = sortPrimitives.pPrimitiveGroup->indices[i];
							float x = pMesh->positionList[nIndex * 3 + 0];
							float y = pMesh->positionList[nIndex * 3 + 1];
							float z = pMesh->positionList[nIndex * 3 + 2];
							if (fMaxX < x)
								fMaxX = x;
							if (fMinX > x)
								fMinX = x;
							if (fMaxY < y)
								fMaxY = y;
							if (fMinY > y)
								fMinY = y;
							if (fMaxZ < z)
								fMaxZ = z;
							if (fMinZ > z)
								fMinZ = z;
						}
						float xDistance = (fMinX + fMaxX) * 0.5f - xDistanceFrom;
						float yDistance = (fMinY + fMaxY) * 0.5f - yDistanceFrom;
						float zDistance = (fMinZ + fMaxZ) * 0.5f - zDistanceFrom;
						sortPrimitives.fDistanceSqrd = xDistance * xDistance + yDistance * yDistance + zDistance * zDistance;
						sortPrimitivesList.push_back(sortPrimitives);
					}

					std::sort(sortPrimitivesList.begin(), sortPrimitivesList.end(), SortPrimitivesSort);
					

					for (int nGroup = 0; nGroup < nNumPrimitiveGroups; nGroup++)
					{
						PrimitiveGroup& primitiveGroup = *sortPrimitivesList[nGroup].pPrimitiveGroup;
						if (primitiveGroup.numIndices)
						{
						//if (pMesh->stripIndexList.size() == 0 && primitiveGroup.numIndices)
							if (primitiveGroup.type == PT_STRIP)
								pMesh->stripIndexList.push_back(primitiveGroup.indices[0]);
							for (size_t i = 0; i < primitiveGroup.numIndices; i++)
							{
								pMesh->stripIndexList.push_back(primitiveGroup.indices[i]);
							}			
							if (primitiveGroup.type == PT_STRIP)
								pMesh->stripIndexList.push_back(primitiveGroup.indices[primitiveGroup.numIndices - 1]);
						}
					}
				}
				else
				{

					PrimitiveGroup* pPrimitiveGroups;
					u16 nNumPrimitiveGroups = 0;
					DisableRestart();
					bool bGenerateStripsResult = GenerateStrips(&pMesh->triagnleIndexList[nIndexStart], nIndexEnd - nIndexStart, &pPrimitiveGroups, &nNumPrimitiveGroups, true);
					TA_ASSERT(bGenerateStripsResult);
					for (int nGroup = 0; nGroup < nNumPrimitiveGroups; nGroup++)
					{
						PrimitiveGroup& primitiveGroup = pPrimitiveGroups[nGroup];
						//TA_ASSERT(primitiveGroup.type == PT_STRIP);
						//if (pMesh->stripIndexList.size() == 0 && primitiveGroup.numIndices)
						if (primitiveGroup.type == PT_STRIP)
							pMesh->stripIndexList.push_back(primitiveGroup.indices[0]);
						for (size_t i = 0; i < primitiveGroup.numIndices; i++)
						{
							pMesh->stripIndexList.push_back(primitiveGroup.indices[i]);
						}		
						if (primitiveGroup.type == PT_STRIP)
							if (primitiveGroup.numIndices)
								pMesh->stripIndexList.push_back(primitiveGroup.indices[primitiveGroup.numIndices - 1]);
					}
				}
			}
			
			if (pMesh->nPrimitiveType != PRIMITIVE_TYPE_TRIANGLES)
			{
				// strips always have one extra index added to terminate the current strip with a degenearge triange so a new strip can be appended, but it isn't necessary to the last strip
				if (pMesh->stripIndexList.size() != 0)
					pMesh->stripIndexList.pop_back();
			}
		}
	}
	else
	{
		Mesh* pMesh = new Mesh;
		pMesh->bDecal = false;
		g_meshList.push_back(pMesh);
		pMesh->nPrimitiveType = PRIMITIVE_TYPE_STRIP;
		const u32 nHashSize = 16661;
		
		std::vector<int> vertexHashTable[nHashSize];
		
		if (g_importMeshList.size() <= 0)
		{
			ReportError("No meshes to import");
		}

		for (int i = 0; i < (int)g_importMeshList.size(); i++)
		{
			ImportMesh& importMesh = *g_importMeshList[i];

			

	
			SetStatus("Resort vis vertices", 0);
			for (int nVertexIn = 0; nVertexIn < (int)importMesh.positionList.size() / 3; nVertexIn++)
			{				
				SetStatus("Resort vis vertices", (float)nCurentVertex / (float)nTotalNumVerticies);
				nCurentVertex++;

	
				int nHash = CalculateHash(importMesh.positionList[nVertexIn * 3 + 0], importMesh.positionList[nVertexIn * 3 + 1],importMesh.positionList[nVertexIn * 3 + 2], nHashSize);
				
				int nVertexOut =  pMesh->positionList.size() / 3;
				for (int nHashEntry = 0; nHashEntry < (int)vertexHashTable[nHash].size(); nHashEntry++)
				{
					int nHashVertex = vertexHashTable[nHash][nHashEntry];

					

					if (nHashVertex != nVertexIn &&
						pMesh->positionList[nHashVertex * 3 + 0] == importMesh.positionList[nVertexIn * 3 + 0] &&
						pMesh->positionList[nHashVertex * 3 + 1] == importMesh.positionList[nVertexIn * 3 + 1] &&
						pMesh->positionList[nHashVertex * 3 + 2] == importMesh.positionList[nVertexIn * 3 + 2] &&
						pMesh->nColourSetCount == importMesh.nColourSetCount &&
						pMesh->nUvSetCount == importMesh.nUvSetCount &&
						pMesh->normalList[nHashVertex * 3 + 0] == importMesh.normalList[nVertexIn * 3 + 0] &&
						pMesh->normalList[nHashVertex * 3 + 1] == importMesh.normalList[nVertexIn * 3 + 1] &&
						pMesh->normalList[nHashVertex * 3 + 2] == importMesh.normalList[nVertexIn * 3 + 2])
					{
						int nColourLayer;
						for (nColourLayer = 0; nColourLayer < pMesh->nColourSetCount; ++nColourLayer)
						{
							if (pMesh->colourSetList[nColourLayer][nHashVertex * 4 + 0] != importMesh.colourSetList[nColourLayer][nVertexIn * 4 + 0] ||
								pMesh->colourSetList[nColourLayer][nHashVertex * 4 + 1] != importMesh.colourSetList[nColourLayer][nVertexIn * 4 + 1] ||
								pMesh->colourSetList[nColourLayer][nHashVertex * 4 + 2] != importMesh.colourSetList[nColourLayer][nVertexIn * 4 + 2] ||
								pMesh->colourSetList[nColourLayer][nHashVertex * 4 + 3] != importMesh.colourSetList[nColourLayer][nVertexIn * 4 + 3])
							{
								break;
							}
						}
						if (nColourLayer != pMesh->nColourSetCount)
							break;
						int nUvLayer;
						for (nUvLayer = 0; nUvLayer < pMesh->nUvSetCount; ++nUvLayer)
						{
							if (pMesh->uvSetList[nUvLayer][nHashVertex * 2 + 0] != importMesh.uvSetList[nUvLayer][nVertexIn * 2 + 0] ||
								pMesh->uvSetList[nUvLayer][nHashVertex * 2 + 1] != importMesh.uvSetList[nUvLayer][nVertexIn * 2 + 1])
							{
								break;
							}
						}
						if (nUvLayer != pMesh->nUvSetCount)
							break;
						nVertexOut = nHashVertex;
						break;
					}

		
				}

				if (nVertexOut == pMesh->positionList.size() / 3)
				{
					
					u32 nHash = CalculateHash(importMesh.positionList[nVertexIn * 3 + 0], importMesh.positionList[nVertexIn * 3 + 1],importMesh.positionList[nVertexIn * 3 + 2], nHashSize);
					vertexHashTable[nHash].push_back(nVertexOut);

					pMesh->positionList.push_back(importMesh.positionList[nVertexIn * 3 + 0]);
					pMesh->positionList.push_back(importMesh.positionList[nVertexIn * 3 + 1]);
					pMesh->positionList.push_back(importMesh.positionList[nVertexIn * 3 + 2]);
					for (int nUvLayer = 0; nUvLayer < pMesh->nUvSetCount; ++nUvLayer)
					{
						pMesh->uvSetList[nUvLayer].push_back(importMesh.uvSetList[nUvLayer][nVertexIn * 2 + 0]);
						pMesh->uvSetList[nUvLayer].push_back(importMesh.uvSetList[nUvLayer][nVertexIn * 2 + 1]);
					}					
					for (int nColourLayer = 0; nColourLayer < pMesh->nColourSetCount; ++nColourLayer)
					{
						pMesh->colourSetList[nColourLayer].push_back(importMesh.colourSetList[nColourLayer][nVertexIn * 4 + 0]);
						pMesh->colourSetList[nColourLayer].push_back(importMesh.colourSetList[nColourLayer][nVertexIn * 4 + 1]);
						pMesh->colourSetList[nColourLayer].push_back(importMesh.colourSetList[nColourLayer][nVertexIn * 4 + 2]);
						pMesh->colourSetList[nColourLayer].push_back(importMesh.colourSetList[nColourLayer][nVertexIn * 4 + 3]);
					}

					pMesh->normalList.push_back(importMesh.normalList[nVertexIn * 3 + 0]);
					pMesh->normalList.push_back(importMesh.normalList[nVertexIn * 3 + 1]);
					pMesh->normalList.push_back(importMesh.normalList[nVertexIn * 3 + 2]);
				}
				TA_ASSERT(nVertexOut <= 0xFFFF);
				if (nVertexOut > 0xFFFF)
					FatalError("Index out of bounds, over 65535 limit");

				pMesh->triagnleIndexList.push_back((u16)nVertexOut);
			}
			SetStatus("Resort vis vertices", 1);
		}

		if (pMesh->triagnleIndexList.size() <= 0)
		{
			g_meshList.pop_back();
			delete pMesh;
		}
		else
		{
			int nMaxBucketSize = 0;
			for (int i = 0; i < nHashSize; i++)
			{
				if (nMaxBucketSize < (int)vertexHashTable[i].size())
					nMaxBucketSize = (int)vertexHashTable[i].size();
			}
			PrimitiveGroup* pPrimitiveGroups;
			u16 nNumPrimitiveGroups = 0;
			DisableRestart();
			bool bGenerateStripsResult = GenerateStrips(&pMesh->triagnleIndexList[0], (int)pMesh->triagnleIndexList.size(), &pPrimitiveGroups, &nNumPrimitiveGroups, true);
			TA_ASSERT(bGenerateStripsResult);
			for (int nGroup = 0; nGroup < nNumPrimitiveGroups; nGroup++)
			{
				PrimitiveGroup& primitiveGroup = pPrimitiveGroups[nGroup];
				TA_ASSERT(primitiveGroup.type == PT_STRIP);
				if (pMesh->stripIndexList.size() == 0 && primitiveGroup.numIndices)
					pMesh->stripIndexList.push_back(primitiveGroup.indices[0]);
				for (size_t i = 0; i < primitiveGroup.numIndices; i++)
				{
					pMesh->stripIndexList.push_back(primitiveGroup.indices[i]);
				}
				if (primitiveGroup.numIndices)
					pMesh->stripIndexList.push_back(primitiveGroup.indices[primitiveGroup.numIndices - 1]);
			}
			if (pMesh->stripIndexList.size() != 0)
				pMesh->stripIndexList.pop_back();
		}
	}
	

	// The file has been imported; we can get rid of the importer.
	pFbxImporter->Destroy();
	g_pFbxScene->Destroy();
	

	
	// Create a new scene so it can be populated by the imported file.
	g_pFbxScene = FbxScene::Create(g_pFbxManager, "myScene");
	//g_pFbxScene->Clear();

	// Load separate gap file if it exists
	if (szFbxGapFileNameIn[0] != 0)
	{
		// Use a separate importer
		FbxImporter* pFbxImporterGap = FbxImporter::Create(g_pFbxManager, "gaps");

		// Initialize the importer.
		bImportStatus = pFbxImporterGap->Initialize(szFbxGapFileNameIn, -1, g_pFbxManager->GetIOSettings());

		if (!bImportStatus) 
			FatalError("Call to FbxImporter::Initialize() for gap file failed.\nError returned: \"%s\"", pFbxImporterGap->GetStatus().GetErrorString());

		// Import the contents of the file into the scene.
		pFbxImporterGap->Import(g_pFbxScene, true);

		bImportSucceded = false;
		while (pFbxImporterGap->IsImporting(bImportSucceded))
		{
			float fStatus = pFbxImporterGap->GetProgress() / 100.0f;
			SetStatus("Load Gap FBX", fStatus);			
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		SetStatus("Load Gap FBX", 1.0f);

		if (!bImportSucceded) 
		{
			FatalError("Call to FbxImporter::Import() for gap file failed.\nError returned: %s\n\n", pFbxImporterGap->GetStatus().GetErrorString());
		}

		pRootNode = g_pFbxScene->GetRootNode();
		RecurseFbxNodesForCollision(pRootNode);

		pFbxImporterGap->Destroy();
	}


	pFbxImporter = FbxImporter::Create(g_pFbxManager, "collision");

	szFilename = szFbxCollisionFileNameIn;

	bImportStatus = pFbxImporter->Initialize(szFilename, -1, g_pFbxManager->GetIOSettings());

	if (!bImportStatus) 
		FatalError("Call to FbxImporter::Initialize() for collision file failed.\nError returned: \"%s\"", pFbxImporter->GetStatus().GetErrorString());

	pFbxImporter->Import(g_pFbxScene, true);

	bImportSucceded = false;
	while (pFbxImporter->IsImporting(bImportSucceded))
	{
		float fStatus = pFbxImporter->GetProgress() / 100.0f;
		SetStatus("Load Collision FBX", fStatus);		
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	SetStatus("Load Collision FBX", 1.0f);

	if (!bImportSucceded) 
	{
		FatalError("Call to FbxImporter::Import() for collision file failed.\nError returned: %s\n\n", pFbxImporter->GetStatus().GetErrorString());
	}

	nNumMaterials = g_pFbxScene->GetMaterialCount();
	for (int nMaterial = 0; nMaterial < nNumMaterials; nMaterial++)
	{
		FbxSurfaceMaterial* pSurfaceMaterial = g_pFbxScene->GetMaterial(nMaterial);


		const char* szNameIn = pSurfaceMaterial->GetName();
		const int nMaxSize = 256;
		char szName[nMaxSize];
		int i;
		for (i = 0; i <szNameIn[i] != 0 && i < nMaxSize - 1; i++)
			szName[i] = tolower(szNameIn[i]);
		szName[i] = 0;

		bool bMaterialFound = false;
		u32 nAttribute = 0;

		if (strstr(szName, "touchtheskyachievment") != 0)
			nAttribute |= ATTRIBUTE_FLAG_SKY_ACHEIVEMENT;
		else
		if (strstr(szName, "concrete") != 0)
			nAttribute = ATTRIBUTE_FLAG_CONCRETE;
		else
		if (strstr(szName, "wood") != 0)
			nAttribute = ATTRIBUTE_FLAG_WOOD;
		else
		if (strstr(szName, "metal") != 0)
			nAttribute = ATTRIBUTE_FLAG_METAL;
		else
		if (strstr(szName, "grass") != 0)
			nAttribute = ATTRIBUTE_FLAG_GRASS;
		else
		if (strstr(szName, "softpadding") != 0)
			nAttribute = ATTRIBUTE_FLAG_PADDING;
		else
		if (strstr(szName, "delayedrespawn") != 0 || strstr(szName, "respawndelayed") != 0)
			nAttribute = ATTRIBUTE_FLAG_RESPAWN_DELAYED;
		else
		if (strstr(szName, "respawn") != 0)
			nAttribute = ATTRIBUTE_FLAG_RESPAWN;
		if (nAttribute)
			bMaterialFound = true;

		if (strstr(szName, "grindendgerail") != 0)
			FatalError("Old attribute spelling mistake mistake of GrindEndgeRail");
		if (strstr(szName, "libguard") != 0)
			FatalError("Old attribute spelling mistake mistake of LibGuard");


		if (strstr(szName, "grindedge") != 0)
		{
			if (strstr(szName, "wide") != 0)
				nAttribute |= ATTRIBUTE_GRIND_EDGE_WIDE;
			if (strstr(szName, "rail") != 0)
				nAttribute |= ATTRIBUTE_GRIND_EDGE_RAIL;
			if (strstr(szName, "popoff") != 0)
				nAttribute |= ATTRIBUTE_GRIND_POP_OFF;
		}
		else
		if (strstr(szName, "lipguard") != 0)

		{		
			nAttribute |= ATTRIBUTE_FLAG_LIP_GUARDS;
			if (strstr(szName, "inner") != 0)
				nAttribute |= ATTRIBUTE_GRIND_LIP;
		}
		else
		{
			if (strstr(szName, "transitiontop") != 0)
				nAttribute |= ATTRIBUTE_FLAG_TRANSITION_TOP;
			else
			if (strstr(szName, "transition") != 0)
				nAttribute |= ATTRIBUTE_FLAG_TRANSITION;

			if (strstr(szName, "bowltop") != 0)
				nAttribute = ATTRIBUTE_FLAG_BOWL | ATTRIBUTE_FLAG_TRANSITION_TOP;
			else
			if (strstr(szName, "bowl") != 0)
				nAttribute |= ATTRIBUTE_FLAG_BOWL;

			if (strstr(szName, "rail") != 0)
			{
				nAttribute = ATTRIBUTE_FLAG_RAIL;
				if (!bMaterialFound )
					nAttribute |= ATTRIBUTE_FLAG_METAL; // default material for a rail is metal rather then concrete
			}
			if (strstr(szName, "incline") != 0)
				nAttribute |= ATTRIBUTE_FLAG_INCLINE;
			if (strstr(szName, "reversetransition") != 0)
				nAttribute |= ATTRIBUTE_FLAG_REVERSE_TRANSITION;
			if (strstr(szName, "stairs") != 0)
				nAttribute |= ATTRIBUTE_FLAG_STAIRS;

			if (strstr(szName, "lip") != 0)
			{
				nAttribute |= ATTRIBUTE_FLAG_LIP;
				if (!bMaterialFound )
					nAttribute |= ATTRIBUTE_FLAG_METAL; // default material for a lip is metal rather then concrete
			}
			if (strstr(szName, "vert") != 0)
				nAttribute |= ATTRIBUTE_FLAG_VERT;
			if (strstr(szName, "fenceside") != 0)
			{
				nAttribute |= ATTRIBUTE_FLAG_FENCE_SIDE;
				if (!bMaterialFound )
					nAttribute |= ATTRIBUTE_FLAG_METAL; // default material for a fence side is metal rather then concrete
			}
			if (strstr(szName, "slow") != 0)
				nAttribute |= ATTRIBUTE_FLAG_SLOW;
		}
					
			
		pSurfaceMaterial->SetUserDataPtr((void*)(size_t)nAttribute);
	}

	pRootNode = g_pFbxScene->GetRootNode();
	RecurseFbxNodesForCollision(pRootNode);
	

	
	//---------------------------------------------------------------------------------
	// Export
	//---------------------------------------------------------------------------------
	{

		FILE* pFile;
		pFile = fopen(szVisFileNameOut, "wt");	


		if (!pFile)
		{
			FatalError("Couldn't create output file");
			return 1;
		}

		u32 nVersion = 1003;
		WriteU8('T', pFile);
		WriteU8('A', pFile);
		WriteU8('S', pFile);
		WriteU8('K', pFile);
		WriteU32(nVersion, pFile, "Version");		

		{	
			FileChunk fileChunk(pFile, "VIS ", "Visible Geometry");

			u32 nFlags = 0x1; // has normals
			nFlags |= 0x10; // necessary

			WriteU32(nFlags, pFile); // flags

			int nTextureCount = 0;
			for (size_t nTexture = 0; nTexture < g_importTextureList.size(); nTexture++)
			{
				ImportTexture* pTexture = g_importTextureList[nTexture];
				if (!pTexture->bLayeredTexture)
					nTextureCount++;
			}

			WriteU32(nTextureCount, pFile, "Num Textures"); // Num Textures
			for (size_t nTexture = 0; nTexture < g_importTextureList.size(); nTexture++)
			{
				ImportTexture* pTexture = g_importTextureList[nTexture];
				if (!pTexture->bLayeredTexture)
				{
					WriteString(pTexture->strName.c_str(), pFile);

				}
			}

			int nNumMaterials = 0;
			TA_ASSERT(g_importTextureList.size() == g_meshList.size());
			for (size_t nTexture = 0; nTexture < g_importTextureList.size(); nTexture++)
			{
				Mesh& mesh = *g_meshList[nTexture];
				int nNumIndicies = (int)mesh.stripIndexList.size();
				if (!nNumIndicies)
					continue;
				nNumMaterials++;
			}

			WriteU32(nNumMaterials, pFile, "Num Materials"); // Num Materials
			for (int nTexture = 0; nTexture < (int)g_importTextureList.size(); nTexture++)
			{
				Mesh& mesh = *g_meshList[nTexture];
				int nNumIndicies = (int)mesh.stripIndexList.size();
				if (!nNumIndicies)
					continue;
				WriteComment(pFile, "Material");
				ImportTexture* pTexture = g_importTextureList[nTexture];
				if (pTexture->bLayeredTexture)
				{
					int nTexture = pTexture->textureArray[0];
					if (mesh.bDecal ||pTexture->bDecal || stristr(g_importTextureList[nTexture]->strFileName.c_str(), "signage") || stristr(g_importTextureList[nTexture]->strFileName.c_str(), "decal"))
					{
					//	if (pTexture->textureArray.size() != 2)
					//		FatalError("decal shader with number of textures other then 2 are not supported");
							
						WriteU32(TECH2_DECAL, pFile, "Material Type (Decal)"); // textue mode
					}
					else
					if (stristr(g_importTextureList[nTexture]->strFileName.c_str(), "alpha"))
					{
						if (pTexture->textureArray.size() != 3)
							FatalError("Apha shader with number of textures other then 3 are not supported");
							
						WriteU32(TECH2_TRANPARANT, pFile, "Material Type (Transparent)"); // textue mode
						WriteComment(pFile, "Color");
						WriteU32ColourFromFloatArray(pTexture->pfColor, pFile);
						WriteFloat(pTexture->fSpecular, pFile, "Specular");		
					}
					else
					{
						if (pTexture->textureArray.size() != 3)
							FatalError("Composit Shader with number of textures other then 3 are not supported: \"%d\"", pTexture->strName.c_str());
							

						WriteU32(TECH2_SOLID, pFile, "Material Type (Solid)"); // textue mode
						WriteComment(pFile, "Color");
						WriteU32ColourFromFloatArray(pTexture->pfColor, pFile);
						WriteFloat(pTexture->fSpecular, pFile, "Specular");
						
						WriteFloat(pTexture->fBlendSharpness_G, pFile, "G Blend Sharpness");
						WriteFloat(pTexture->fBlendLevel_G, pFile, "G Blend Level");
						WriteFloat(pTexture->fBlendMode_G, pFile, "G Blend Mode");
						WriteComment(pFile, "G Shadow Color");
						WriteU32ColourFromFloatArray(pTexture->pfShadowColor_G, pFile);
						WriteComment(pFile, "G Highlight Color");
						WriteU32ColourFromFloatArray(pTexture->pfHighlightColor_G, pFile);
						WriteFloat(pTexture->fIgnoreBaseColor_G, pFile, "G Ignore Base Color");
						WriteFloat(pTexture->fSpecular_G, pFile, "G Specular");

						WriteFloat(pTexture->fBlendSharpness_B, pFile, "B Blend Sharpness");
						WriteFloat(pTexture->fBlendLevel_B, pFile, "B Blend Level");
						WriteFloat(pTexture->fBlendMode_B, pFile, "B Blend Mode");
						WriteComment(pFile, "B Shadow Color");
						WriteU32ColourFromFloatArray(pTexture->pfShadowColor_B, pFile);
						WriteComment(pFile, "B Highlight Color");
						WriteU32ColourFromFloatArray(pTexture->pfHighlightColor_B, pFile);
						WriteFloat(pTexture->fIgnoreBaseColor_B, pFile, "B Ignore Base Color");
						WriteFloat(pTexture->fSpecular_B, pFile, "B Specular");				
					}
			
					WriteU32((u32)pTexture->textureArray.size(), pFile, "Num Layers"); // num layers
					for (size_t i = 0; i < (u32)pTexture->textureArray.size(); i++)
						WriteU32(pTexture->textureArray[i], pFile, i == 0 ? "Texture index" : 0); // if num layers is greater then 2, then write an index into another texture
				}
				else
				{
					WriteU32(TECH2_DECAL, pFile, "Material Type (Decal)"); // textue mode
						
					WriteU32(1, pFile, "Num Layers"); // num layers
					TA_ASSERT(nTexture >= 0);
					TA_ASSERT(nTexture < nTextureCount);
					WriteU32(nTexture, pFile, "Texture index");
				}
			}



			int nNumVerticies = 0;
			for (size_t nMesh = 0; nMesh < g_meshList.size(); nMesh++)
			{
				Mesh& mesh = *g_meshList[nMesh];
				int nNumIndicies = (int)mesh.stripIndexList.size();
				if (!nNumIndicies)
					continue;
				nNumVerticies += mesh.positionList.size() / 3;

				
				if (mesh.nPrimitiveType == PRIMITIVE_TYPE_TRIANGLES) // will be alpha sorted
				{
					for (int i = 0; i < (int)mesh.positionList.size() / 3; i++)
						mesh.fadeDistanceList.push_back(1.0f);

					for (int i = 0; i < (int)mesh.triagnleIndexList.size() / 3; i++)

					{
						int n1 = mesh.triagnleIndexList[i * 3 + 0];
						int n2 = mesh.triagnleIndexList[i * 3 + 1];
						int n3 = mesh.triagnleIndexList[i * 3 + 2];
						FbxVector4 v3A(mesh.positionList[n1 * 3 + 0], mesh.positionList[n1 * 3 + 1], mesh.positionList[n1 * 3 + 2]);
						FbxVector4 v3B(mesh.positionList[n2 * 3 + 0], mesh.positionList[n2 * 3 + 1], mesh.positionList[n2 * 3 + 2]);
						FbxVector4 v3C(mesh.positionList[n3 * 3 + 0], mesh.positionList[n3 * 3 + 1], mesh.positionList[n3 * 3 + 2]);
						float fLength = MaxF(MaxF((v3A - v3B).SquareLength(), (v3B - v3C).SquareLength()), (v3C - v3A).SquareLength());
						if (mesh.fadeDistanceList[n1] < fLength)
							mesh.fadeDistanceList[n1] = fLength;
						if (mesh.fadeDistanceList[n2] < fLength)
							mesh.fadeDistanceList[n2] = fLength;
						if (mesh.fadeDistanceList[n3] < fLength)
							mesh.fadeDistanceList[n3] = fLength;
					}
					 
					
					for (int i = 0; i < (int)mesh.fadeDistanceList.size(); i++)
						mesh.fadeDistanceList[i] = sqrtf(mesh.fadeDistanceList[i]);
				}
			}
			
			WriteU32(nNumVerticies, pFile, "Num Vertices");
			
			// write index lists lenghts
			int nIndexBuffers = nNumMaterials;//(u32)g_meshList.size();
			WriteU32(nIndexBuffers, pFile);
			int nOffset = 0;

			u32* pMeshFlags = new u32[g_meshList.size()];
			
			for (size_t nMesh = 0; nMesh < g_meshList.size(); nMesh++)
			{
				Mesh& mesh = *g_meshList[nMesh];
				int nNumIndicies = (int)mesh.stripIndexList.size();

				if (!nNumIndicies)
					continue;
				
				WriteComment(pFile, "Mesh");

				WriteU32(nNumIndicies, pFile, "Num Indices");
 				WriteU32((u32)mesh.positionList.size() / 3, pFile, "Num Vertices");

				u32 nMeshFlags = 0x1; // has normals
				WriteComment(pFile, "Normals (Flags |= 0x1)");
				/*if (mesh.nUvSetCount > 1)
				{
					nMeshFlags |= 0x8; // uv2
				}*/

				/*if (mesh.colourSetList.size() > 0)
				{
					nMeshFlags |= 0x4; // Colours
				}*/

				if (mesh.nPrimitiveType == PRIMITIVE_TYPE_TRIANGLES)
				{
					WriteComment(pFile, "Triangle List (Flags |= 0x20)");
					nMeshFlags |= 0x20; // Triangle list
				}

				pMeshFlags[nMesh] = nMeshFlags;
				WriteU32(nMeshFlags, pFile, "Flags");
				
				WriteU8(mesh.nColourSetCount, pFile, "Num Colour Sets");
				WriteU8(mesh.nUvSetCount, pFile, "Num Uv Sets");
		
				int nVertexSize = 
					sizeof (float) * 3 +	// position
					sizeof (float) * 3;	    // normal

				nVertexSize += sizeof (float) * 2 * mesh.nUvSetCount; // uv layers
				nVertexSize += sizeof (u32) * mesh.nColourSetCount; // uv layers

				
				if (mesh.nPrimitiveType == PRIMITIVE_TYPE_TRIANGLES) // will be sorted front to back for alpha
					nVertexSize += sizeof (float);

				nOffset += (mesh.positionList.size() / 3) * nVertexSize;
			}
			

			size_t start = ftell(pFile);
			// write vertex data
			for (int nMesh = 0; nMesh < (int)g_meshList.size(); nMesh++)
			{
				Mesh& mesh = *g_meshList[nMesh];
				int nNumIndicies = (int)mesh.stripIndexList.size();
				if (!nNumIndicies)
					continue;

				WriteComment(pFile, "Mesh Vertices");

				u32 nMeshFlags = pMeshFlags[nMesh];

				bool bFirst = true;
				for (int i = 0; i < (int)mesh.positionList.size() / 3; i++)
				{
					WriteFloat(mesh.positionList[i * 3 + 0], pFile, bFirst ? "x" : 0);
					WriteFloat(mesh.positionList[i * 3 + 1], pFile, bFirst ? "y" : 0);
					WriteFloat(mesh.positionList[i * 3 + 2], pFile, bFirst ? "z" : 0);

					for (int nUvLayer = 0; nUvLayer < mesh.nUvSetCount; ++nUvLayer)
					{
						WriteFloat(mesh.uvSetList[nUvLayer][i * 2 + 0], pFile, bFirst ? "u" : 0);
						WriteFloat(mesh.uvSetList[nUvLayer][i * 2 + 1], pFile, bFirst ? "v" : 0);
					}
					
					for (int nColourLayerMain = 0; nColourLayerMain < mesh.nColourSetCount; ++nColourLayerMain)
					{
						int nColourLayer = nColourLayerMain;
						if (g_bReverseColourLayers)
							nColourLayer = mesh.nColourSetCount - nColourLayer - 1;

						int r = 255;
						int g = 255;
						int b = 255;
						int a = 255;
						if ((i * 4 + 3) < mesh.colourSetList[nColourLayer].size())
						{
							r = (int)(mesh.colourSetList[nColourLayer][i * 4 + 0] * 256.0f);
							g = (int)(mesh.colourSetList[nColourLayer][i * 4 + 1] * 256.0f);
							b = (int)(mesh.colourSetList[nColourLayer][i * 4 + 2] * 256.0f);
							a = (int)(mesh.colourSetList[nColourLayer][i * 4 + 3] * 256.0f);
						}
						
						if (r < 0) r = 0; else if (r > 255) r = 255;
						if (g < 0) g = 0; else if (g > 255) g = 255;
						if (b < 0) b = 0; else if (b > 255) b = 255;
						if (a < 0) a = 0; else if (a > 255) a = 255;
						
						//u32 nColour = (a << 24) + (b << 16) + (g << 8) + r;
						WriteU8(r, pFile, bFirst ? "r" : 0);
						WriteU8(g, pFile, bFirst ? "g" : 0);
						WriteU8(b, pFile, bFirst ? "b" : 0);
						WriteU8(a, pFile, bFirst ? "a" : 0);
					}

					WriteFloat(mesh.normalList[i * 3 + 0], pFile, bFirst ? "normal x" : 0);
					WriteFloat(mesh.normalList[i * 3 + 1], pFile, bFirst ? "normal y" : 0);
					WriteFloat(mesh.normalList[i * 3 + 2], pFile, bFirst ? "normal y" : 0);
					
					if (mesh.nPrimitiveType == PRIMITIVE_TYPE_TRIANGLES) // will be sorted front to back for alpha
					{
						WriteFloat(mesh.fadeDistanceList[i], pFile, bFirst ? "fade distance" : 0);
					}
					bFirst = false;
				}
			}

			delete [] pMeshFlags;
			size_t end = ftell(pFile);

			// write indicies
			nOffset = 0;
			for (size_t nMesh = 0; nMesh < g_meshList.size(); nMesh++)
			{
				Mesh& mesh = *g_meshList[nMesh];
				int nNumIndicies = (int)mesh.stripIndexList.size();
				if (!nNumIndicies)
					continue;

				WriteComment(pFile, "Mesh Indices");
				for (size_t i = 0; i < mesh.stripIndexList.size(); i++)
				{
					int nIndex = mesh.stripIndexList[i] + nOffset;
					TA_ASSERT(nIndex < 0x10000);
					WriteU16(nIndex, pFile);
				}
				//nOffset += mesh.positionList.size() / 3;
			}

			
		}

		{	
			FileChunk fileChunk(pFile, "COL ", "Collision Geometry");

			CollisionMeshRemoveUnusedVerticies(g_importCollisionMesh);

			WriteS32(g_importCollisionMesh.positionList.size() / 3, pFile, "Num Vertices");
			WriteS32(g_importCollisionMesh.polygonNumVerticesList.size(), pFile, "Num Polygon Vertices");
			WriteS32(g_importCollisionMesh.polygonIndiciesList.size(), pFile, "Num Polygon Indices");

			WriteComment(pFile, "Vertices");
			// write positions
			char* pXYZ[3] = { "x", "y", "z" };
			for (size_t i = 0; i < g_importCollisionMesh.positionList.size(); i++)
				WriteFloat(g_importCollisionMesh.positionList[i], pFile, i >= 3 ? 0 : pXYZ[i]);
			
			WriteComment(pFile, "Polygons");
			// write polygons
			int nIndex = 0;
			for (size_t i = 0; i < g_importCollisionMesh.polygonNumVerticesList.size(); i++)
			{
				int nNumSides = g_importCollisionMesh.polygonNumVerticesList[i];
				WriteU8(nNumSides, pFile, i == 0 ? "Num Sides" : 0);
				WriteU32(g_importCollisionMesh.polygonAttirbuteList[i], pFile, i == 0 ? "Atttribute" : 0); // attribute

				for (int j = 0; j < nNumSides; j++)
				{
					int nVertex = g_importCollisionMesh.polygonIndiciesList[nIndex++];
					WriteU16(nVertex, pFile, i == 0 ? "Vertex Index" : 0);
				}
			}
		}

		{	
			FileChunk fileChunk(pFile, "EDGE", "Grind Geometry");
			WriteS32(g_importCollisionMesh.grindEdgeAttributeList.size(), pFile, "Num Edges");
			for (size_t i = 0; i < g_importCollisionMesh.grindEdgeAttributeList.size(); i++)
			{
				WriteU32(g_importCollisionMesh.grindEdgeAttributeList[i], pFile, i == 0 ? "Attribute" : 0);
				WriteFloat(g_importCollisionMesh.grindEdgeList[i * 6 + 0], pFile, i == 0 ? "x 1" : 0);
				WriteFloat(g_importCollisionMesh.grindEdgeList[i * 6 + 1], pFile, i == 0 ? "y 1" : 0);
				WriteFloat(g_importCollisionMesh.grindEdgeList[i * 6 + 2], pFile, i == 0 ? "z 1" : 0);
				WriteFloat(g_importCollisionMesh.grindEdgeList[i * 6 + 3], pFile, i == 0 ? "x 2" : 0);
				WriteFloat(g_importCollisionMesh.grindEdgeList[i * 6 + 4], pFile, i == 0 ? "y 2" : 0);
				WriteFloat(g_importCollisionMesh.grindEdgeList[i * 6 + 5], pFile, i == 0 ? "z 2" : 0);
			}
		}

		{	
			FileChunk fileChunk(pFile, "VOLU", "Volume Geometry");
			WriteS32(g_importOobList.size(), pFile);
			for (size_t i = 0; i < g_importOobList.size(); i++)
			{
				ImportOob* pOob = g_importOobList[i];
				WriteString(pOob->szName, pFile);
				
				WriteFloat((float)pOob->fbxMatrix[3][0], pFile, i == 0 ? "x" : 0);
				WriteFloat((float)pOob->fbxMatrix[3][1], pFile, i == 0 ? "y" : 0);
				WriteFloat((float)pOob->fbxMatrix[3][2], pFile, i == 0 ? "z" : 0);
				WriteFloat((float)pOob->fbxMatrix[0][0], pFile, i == 0 ? "xx" : 0);
				WriteFloat((float)pOob->fbxMatrix[0][1], pFile, i == 0 ? "xy" : 0);
				WriteFloat((float)pOob->fbxMatrix[0][2], pFile, i == 0 ? "xz" : 0);
				WriteFloat((float)pOob->fbxMatrix[1][0], pFile, i == 0 ? "yx" : 0);
				WriteFloat((float)pOob->fbxMatrix[1][1], pFile, i == 0 ? "yy" : 0);
				WriteFloat((float)pOob->fbxMatrix[1][2], pFile, i == 0 ? "yz" : 0);
				WriteFloat((float)pOob->fbxMatrix[2][0], pFile, i == 0 ? "zx" : 0);
				WriteFloat((float)pOob->fbxMatrix[2][1], pFile, i == 0 ? "zy" : 0);
				WriteFloat((float)pOob->fbxMatrix[2][2], pFile, i == 0 ? "zz" : 0);
			}
		}
		
		fclose(pFile);
	}


    return 0; 
}




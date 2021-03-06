/*==================================================================================
    Copyright (c) 2008, DAVA Consulting, LLC
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the DAVA Consulting, LLC nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE DAVA CONSULTING, LLC AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL DAVA CONSULTING, LLC BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    Revision History:
        * Created by Vitaliy Borodovsky 
=====================================================================================*/
#ifndef __DAVAENGINE_SCENEFILEV2_H__
#define __DAVAENGINE_SCENEFILEV2_H__

#include "Base/BaseObject.h"
#include "Base/BaseMath.h"
#include "Scene3D/Scene.h"
#include "Render/3D/StaticMesh.h"
#include "Render/3D/PolygonGroup.h"
#include "Utils/Utils.h"
#include "FileSystem/File.h"

namespace DAVA
{
    
/*
     Scene serialization thoughts: 
     
     There are 2 types of nodes: 
     1. Data Object
     - Texture - WILL BE REMOVED
     - Material - textures is instanced by names
     - Polygroup - group of polygons
     - Static Mesh - several polygroups combined
     - Animated Mesh - mesh with weights and bones heirarhy information
     - Animations - animation for the specific mesh / bones
     - Tiled Textures - in FUTURE for dynamic loading of landscapes when traveling over the world
     
     2. Hierarchy Node
     
     * Object nodes 
     - Light - light in the scene
     - Camera - camera in the scene
     - MeshInstance - instance of the specific mesh in the scene
     - AnimatedMeshInstance - instance of the specific animated mesh in the scene / animated mesh instances should store data inside.
     - Landscape - terrain object
     - Skeleton - root of the skeletal animated mesh
     - Bone - node for skeletal animation
     - Dummy - dummy object that can be used for additional client logic
     - ParticleEmitter - emitter node
     - Occluder - for occlusion culling
     
     * Clusterization nodes 
     - BSPTree - tbd later
     - QuadTree - tbd later 
     - OctTree - tbd later
     - Portal - tbd later
     
     General thoughts
     - Format should support uploading of data nodes on the fly
     - Format should support versioning for nodes
     - Keyed archive
     
 
    3. Required types
    -   int32, float32, 
    -   Vector2, Vector3, Vector4
    -   Matrix3, Matrix4
    -   Quaternion
 
 
     Scene * scene = ...;
     scene->Load("filename
*/

class SceneFileV2 : public BaseObject
{
public: 
    enum eError{
        ERROR_NO_ERROR = 0,
        ERROR_VERSION_IS_TOO_OLD = 1,
        ERROR_FAILED_TO_CREATE_FILE = 2,
        ERROR_FILE_WRITE_ERROR = 3,
    };
    
    
    SceneFileV2();
    virtual ~SceneFileV2();
    
    eError SaveScene(const String & filename, Scene * _scene);
    eError LoadScene(const String & filename, Scene * _scene);

    void EnableDebugLog(bool _isDebugLogEnabled);
    bool DebugLogEnabled();
    void EnableSaveForGame(bool _isSaveForGame);
    
    const String & GetScenePath();
    const String & GetSceneFilename();
    
    String AbsoluteToRelative(const String & absolutePathname);
    String RelativeToAbsolute(const String & relativePathname);
    Material * GetMaterial(int32 index);
    StaticMesh * GetStaticMesh(int32 index);
    
    DataNode * GetNodeByPointer(uint64 pointer);
    
    int32 GetVersion();
    void SetError(eError error);
    eError GetError();
    
    void OptimizeScene(Entity * rootNode);
    bool RemoveEmptySceneNodes(Entity * rootNode);
    bool RemoveEmptyHierarchy(Entity * currentNode);
	void ConvertShadows(Entity * rootNode);
    int32 removedNodeCount;
private:
    void AddToNodeMap(DataNode * node);

    struct Header
    {
        char    signature[4];
        int32   version;
        int32   nodeCount;
    };
    Header header;
    Map<uint64, DataNode*> dataNodes;
    Vector<Material*> materials;
    Vector<StaticMesh*> staticMeshes;
    
    bool SaveDataHierarchy(DataNode * node, File * file, int32 level);
    void LoadDataHierarchy(Scene * scene, DataNode * node, File * file, int32 level);
    bool SaveDataNode(DataNode * node, File * file);
    void LoadDataNode(DataNode * parent, File * file);

    bool SaveHierarchy(Entity * node, File * file, int32 level);
    void LoadHierarchy(Scene * scene, Entity * node, File * file, int32 level);

    bool ReplaceNodeAfterLoad(Entity * node);
	void ReplaceOldNodes(Entity * currentNode);

    
    bool isDebugLogEnabled;
    bool isSaveForGame;
    String rootNodePathName, rootNodePath, rootNodeName; 
    Scene * scene;
    eError lastError;
};
  
}; // namespace DAVA

#endif // __DAVAENGINE_SCENEFILEV2_H__





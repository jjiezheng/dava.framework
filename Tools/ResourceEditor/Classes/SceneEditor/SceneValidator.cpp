#include "SceneValidator.h"
#include "EditorSettings.h"

#include "Render/LibPVRHelper.h"
#include "Render/TextureDescriptor.h"

#include "../Qt/Main/QtUtils.h"
#include "../Qt/Scene/SceneDataManager.h"
#include "../Qt/Scene/SceneData.h"
#include "../EditorScene.h"

#include "../LandscapeEditor/EditorLandscape.h"
#include "../ParticlesEditorQT/Helpers/ParticlesEditorSceneDataHelper.h"

#include "Scene3D/Components/ComponentHelpers.h"

SceneValidator::SceneValidator()
{
    sceneTextureCount = 0;
    sceneTextureMemory = 0;

    pathForChecking = String("");
}

SceneValidator::~SceneValidator()
{
}

bool SceneValidator::ValidateSceneAndShowErrors(Scene *scene)
{
    errorMessages.clear();

    ValidateScene(scene, errorMessages);

    ShowErrorDialog(errorMessages);
    return (!errorMessages.empty());
}


void SceneValidator::ValidateScene(Scene *scene, Set<String> &errorsLog)
{
    if(scene) 
    {
        ValidateSceneNode(scene, errorsLog);

        for (Set<Entity*>::iterator it = emptyNodesForDeletion.begin(); it != emptyNodesForDeletion.end(); ++it)
        {
            Entity * node = *it;
            if (node->GetParent())
            {
                node->GetParent()->RemoveNode(node);
            }
        }
        

		for (Set<Entity *>::iterator it = emptyNodesForDeletion.begin(); it != emptyNodesForDeletion.end(); ++it)
		{
			Entity *node = *it;
			SafeRelease(node);
		}

        emptyNodesForDeletion.clear();
    }
    else 
    {
        errorsLog.insert(String("Scene in NULL!"));
    }
}

void SceneValidator::ValidateScales(Scene *scene, Set<String> &errorsLog)
{
	if(scene) 
	{
		ValidateScalesInternal(scene, errorsLog);
	}
	else 
	{
		errorsLog.insert(String("Scene in NULL!"));
	}
}

void SceneValidator::ValidateScalesInternal(Entity *sceneNode, Set<String> &errorsLog)
{
//  Basic algorithm is here
// 	Matrix4 S, T, R; //Scale Transpose Rotation
// 	S.CreateScale(Vector3(1.5, 0.5, 2.0));
// 	T.CreateTranslation(Vector3(100, 50, 20));
// 	R.CreateRotation(Vector3(0, 1, 0), 2.0);
// 
// 	Matrix4 t = R*S*T; //Calculate complex matrix
// 
//	//Calculate Scale components from complex matrix
// 	float32 sx = sqrt(t._00 * t._00 + t._10 * t._10 + t._20 * t._20);
// 	float32 sy = sqrt(t._01 * t._01 + t._11 * t._11 + t._21 * t._21);
// 	float32 sz = sqrt(t._02 * t._02 + t._12 * t._12 + t._22 * t._22);
// 	Vector3 sCalculated(sx, sy, sz);

	if(!sceneNode) return;

	const Matrix4 & t = sceneNode->GetLocalTransform();
	float32 sx = sqrt(t._00 * t._00 + t._10 * t._10 + t._20 * t._20);
	float32 sy = sqrt(t._01 * t._01 + t._11 * t._11 + t._21 * t._21);
	float32 sz = sqrt(t._02 * t._02 + t._12 * t._12 + t._22 * t._22);

	if ((!FLOAT_EQUAL(sx, 1.0f)) 
		|| (!FLOAT_EQUAL(sy, 1.0f))
		|| (!FLOAT_EQUAL(sz, 1.0f)))
	{
 		errorsLog.insert(Format("Node %s: has scale (%.3f, %.3f, %.3f) ! Re-design level.", sceneNode->GetName().c_str(), sx, sy, sz));
	}

	int32 count = sceneNode->GetChildrenCount();
	for(int32 i = 0; i < count; ++i)
	{
		ValidateScalesInternal(sceneNode->GetChild(i), errorsLog);
	}
}


void SceneValidator::ValidateSceneNode(Entity *sceneNode, Set<String> &errorsLog)
{
    if(!sceneNode) return;
    
    int32 count = sceneNode->GetChildrenCount();
    for(int32 i = 0; i < count; ++i)
    {
        Entity *node = sceneNode->GetChild(i);
        
        ValidateRenderComponent(node, errorsLog);
        ValidateLodComponent(node, errorsLog);
        ValidateParticleEmitterComponent(node, errorsLog);
        
        

        KeyedArchive *customProperties = node->GetCustomProperties();
        if(customProperties->IsKeyExists("editor.referenceToOwner"))
        {
            String dataSourcePath = EditorSettings::Instance()->GetDataSourcePath();
            if(1 < dataSourcePath.length())
            {
                if('/' == dataSourcePath[0])
                {
                    dataSourcePath = dataSourcePath.substr(1);
                }
                
                String referencePath = customProperties->GetString("editor.referenceToOwner");
                String::size_type pos = referencePath.rfind(dataSourcePath);
                if((String::npos != pos) && (1 != pos))
                {
                    referencePath.replace(pos, dataSourcePath.length(), "");
                    customProperties->SetString("editor.referenceToOwner", referencePath);
                    
                    errorsLog.insert(Format("Node %s: referenceToOwner isn't correct. Re-save level.", node->GetName().c_str()));
                }
            }
        }
        
        ValidateSceneNode(node, errorsLog);
    }
}


void SceneValidator::ValidateRenderComponent(Entity *ownerNode, Set<String> &errorsLog)
{
    RenderComponent *rc = static_cast<RenderComponent *>(ownerNode->GetComponent(Component::RENDER_COMPONENT));
    if(!rc) return;
    
    RenderObject *ro = rc->GetRenderObject();
    if(!ro) return;
    
    uint32 count = ro->GetRenderBatchCount();
    for(uint32 b = 0; b < count; ++b)
    {
        RenderBatch *renderBatch = ro->GetRenderBatch(b);
        ValidateRenderBatch(ownerNode, renderBatch, errorsLog);
    }
    
    Landscape *landscape = dynamic_cast<Landscape *>(ro);
    if(landscape)
    {
        ValidateLandscape(landscape, errorsLog);
    }
}


void SceneValidator::ValidateLodComponent(Entity *ownerNode, Set<String> &errorsLog)
{
    LodComponent *lodComponent = GetLodComponent(ownerNode);
    if(!lodComponent) return;


    int32 layersCount = lodComponent->GetLodLayersCount();
    for(int32 layer = 0; layer < layersCount; ++layer)
    {
        float32 distance = lodComponent->GetLodLayerDistance(layer);
        if(LodComponent::INVALID_DISTANCE == distance)
        {
            //TODO: why this function isn't realized for lodcomponent?
            lodComponent->SetLodLayerDistance(layer, LodComponent::GetDefaultDistance(layer));
            errorsLog.insert(Format("Node %s: lod distances weren't correct. Re-save.", ownerNode->GetName().c_str()));
        }
    }
    
    Vector<LodComponent::LodData *>lodLayers;
    lodComponent->GetLodData(lodLayers);
    
    Vector<LodComponent::LodData *>::const_iterator endIt = lodLayers.end();
    int32 layer = 0;
    for(Vector<LodComponent::LodData *>::iterator it = lodLayers.begin(); it != endIt; ++it, ++layer)
    {
        LodComponent::LodData * ld = *it;
        
        if(ld->layer != layer)
        {
            ld->layer = layer;
            errorsLog.insert(Format("Node %s: lod layers weren't correct. Rename childs. Re-save.", ownerNode->GetName().c_str()));
        }
    }
}

void SceneValidator::ValidateParticleEmitterComponent(DAVA::Entity *ownerNode, Set<String> &errorsLog)
{
	ParticleEmitter * emitter = GetEmitter(ownerNode);
    if(!emitter) 
		return;

    String validationMsg;
    if (!ParticlesEditorSceneDataHelper::ValidateParticleEmitter(emitter, validationMsg))
    {
        errorsLog.insert(validationMsg);
    }
}



void SceneValidator::ValidateRenderBatch(Entity *ownerNode, RenderBatch *renderBatch, Set<String> &errorsLog)
{
    ownerNode->RemoveFlag(Entity::NODE_INVALID);
    
    
    Material *material = renderBatch->GetMaterial();
    if(material)
    {
        ValidateMaterial(material, errorsLog);
    }
    
    InstanceMaterialState *materialState = renderBatch->GetMaterialInstance();
    if(materialState)
    {
        ValidateInstanceMaterialState(materialState, errorsLog);
    }
    
    
    PolygonGroup *polygonGroup = renderBatch->GetPolygonGroup();
    if(polygonGroup)
    {
        if(material)
        {
            if (material->Validate(polygonGroup) == Material::VALIDATE_INCOMPATIBLE)
            {
                ownerNode->AddFlag(Entity::NODE_INVALID);
                errorsLog.insert(Format("Material: %s incompatible with node:%s.", material->GetName().c_str(), ownerNode->GetFullName().c_str()));
                errorsLog.insert("For lightmapped objects check second coordinate set. For normalmapped check tangents, binormals.");
            }
        }
    }
}

void SceneValidator::ValidateMaterial(Material *material, Set<String> &errorsLog)
{
    for(int32 iTex = 0; iTex < Material::TEXTURE_COUNT; ++iTex)
    {
        Texture *texture = material->GetTexture((Material::eTextureLevel)iTex);
        if(texture)
        {
            ValidateTexture(texture, material->GetTextureName((Material::eTextureLevel)iTex), Format("Material: %s. TextureLevel %d.", material->GetName().c_str(), iTex), errorsLog);
            
            String matTexName = material->GetTextureName((Material::eTextureLevel)iTex);
            if(!IsTextureDescriptorPath(matTexName))
            {
                material->SetTexture((Material::eTextureLevel)iTex, TextureDescriptor::GetDescriptorPathname(matTexName));
            }
        }
    }
}


void SceneValidator::ValidateInstanceMaterialState(InstanceMaterialState *materialState, Set<String> &errorsLog)
{
    if(materialState->GetLightmap())
    {
        ValidateTexture(materialState->GetLightmap(), materialState->GetLightmapName(), "InstanceMaterialState, lightmap", errorsLog);
    }
    
    String lightmapName = materialState->GetLightmapName();
    if(!IsTextureDescriptorPath(lightmapName))
    {
        Texture *lightmap = SafeRetain(materialState->GetLightmap());
        materialState->SetLightmap(lightmap, TextureDescriptor::GetDescriptorPathname(lightmapName));
        SafeRelease(lightmap);
    }
}


void SceneValidator::ValidateLandscape(Landscape *landscape, Set<String> &errorsLog)
{
    if(!landscape) return;
    
    EditorLandscape *editorLandscape = dynamic_cast<EditorLandscape *>(landscape);
    if(editorLandscape)
    {
        return;
    }
    
    
    for(int32 i = 0; i < Landscape::TEXTURE_COUNT; ++i)
    {
        if(		(Landscape::TEXTURE_DETAIL == (Landscape::eTextureLevel)i)
           ||	(Landscape::TEXTURE_TILE_FULL == (Landscape::eTextureLevel)i
                 &&	(landscape->GetTiledShaderMode() == Landscape::TILED_MODE_TILEMASK
                 || landscape->GetTiledShaderMode() == Landscape::TILED_MODE_TILE_DETAIL_MASK)))
        {
            continue;
        }
        
		// TODO:
		// new texture path
		DAVA::String landTexName = landscape->GetTextureName((Landscape::eTextureLevel)i);
		if(!IsTextureDescriptorPath(landTexName))
		{
			landscape->SetTextureName((Landscape::eTextureLevel)i, TextureDescriptor::GetDescriptorPathname(landTexName));
		}
        
        ValidateTexture(landscape->GetTexture((Landscape::eTextureLevel)i), landscape->GetTextureName((Landscape::eTextureLevel)i), Format("Landscape. TextureLevel %d", i), errorsLog);
    }
    
    bool pathIsCorrect = ValidatePathname(landscape->GetHeightmapPathname(), String("Landscape. Heightmap."));
    if(!pathIsCorrect)
    {
        String path = FileSystem::AbsoluteToRelativePath(EditorSettings::Instance()->GetDataSourcePath(), landscape->GetHeightmapPathname());
        errorsLog.insert("Wrong path of Heightmap: " + path);
    }
}




bool SceneValidator::NodeRemovingDisabled(Entity *node)
{
    KeyedArchive *customProperties = node->GetCustomProperties();
    return (customProperties && customProperties->IsKeyExists("editor.donotremove"));
}


void SceneValidator::ValidateTextureAndShowErrors(Texture *texture, const String &textureName, const String &validatedObjectName)
{
    errorMessages.clear();

    ValidateTexture(texture, textureName, validatedObjectName, errorMessages);
    ShowErrorDialog(errorMessages);
}

void SceneValidator::ValidateTexture(Texture *texture, const String &texturePathname, const String &validatedObjectName, Set<String> &errorsLog)
{
	if(!texture) return;
	
	String path = FileSystem::AbsoluteToRelativePath(EditorSettings::Instance()->GetProjectPath(), texturePathname);
	String textureInfo = path + " for object: " + validatedObjectName;

	if(texture == Texture::GetPinkPlaceholder())
	{
		errorsLog.insert("Can't load texture: " + textureInfo);
	}

	bool pathIsCorrect = ValidatePathname(texturePathname, validatedObjectName);
	if(pathIsCorrect)
	{
		if(!IsFBOTexture(texture))
		{
			// if there is no descriptor file for this texture - generate it
			CreateDescriptorIfNeed(texturePathname);
		}
	}
	else
	{
		errorsLog.insert("Wrong path of: " + textureInfo);
	}
	
	if(!IsPowerOf2(texture->GetWidth()) || !IsPowerOf2(texture->GetHeight()))
	{
		errorsLog.insert("Wrong size of " + textureInfo);
	}
    
    if(texture->GetWidth() > 2048 || texture->GetHeight() > 2048)
	{
		errorsLog.insert("Texture is too big. " + textureInfo);
	}
}



void SceneValidator::EnumerateSceneTextures()
{
    SceneData *sceneData = SceneDataManager::Instance()->SceneGetActive();
    DVASSERT_MSG(sceneData, "Illegal situation");
    
    Map<String, Texture *> textureMap;
    SceneDataManager::EnumerateTextures(sceneData->GetScene(), textureMap);
    sceneTextureCount = textureMap.size();

    KeyedArchive *settings = EditorSettings::Instance()->GetSettings();
    String projectPath = settings->GetString("ProjectPath");

    sceneTextureMemory = 0;
	for(Map<String, Texture *>::const_iterator it = textureMap.begin(); it != textureMap.end(); ++it)
	{
		Texture *t = it->second;
        if(String::npos == t->GetPathname().find(projectPath))
        {   // skip all textures that are not related the scene
            continue;
        }

        
        if(String::npos != t->GetPathname().find(projectPath))
        {   //We need real info about textures size. In Editor on desktop pvr textures are decompressed to RGBA8888, so they have not real size.
            String imageFileName = TextureDescriptor::GetPathnameForFormat(t->GetPathname(), t->GetSourceFileFormat());
            switch (t->GetSourceFileFormat())
            {
                case DAVA::PVR_FILE:
                {
                    sceneTextureMemory += LibPVRHelper::GetDataLength(imageFileName);
                    break;
                }
                    
                case DAVA::DXT_FILE:
                {
                    sceneTextureMemory += (int32)LibDxtHelper::GetDataSize(imageFileName);
                    break;
                }
                    
                default:
                    sceneTextureMemory += t->GetDataSize();
                    break;
            }
        }
	}
}

bool SceneValidator::WasTextureChanged(Texture *texture, ImageFileFormat fileFormat)
{
    if(IsFBOTexture(texture))
    {
        return false;
    }
    
    String texturePathname = texture->GetPathname();
    return (IsPathCorrectForProject(texturePathname) && IsTextureChanged(texturePathname, fileFormat));
}

bool SceneValidator::IsFBOTexture(Texture *texture)
{
    if(texture->isRenderTarget)
    {
        return true;
    }

    String::size_type textTexturePos = texture->GetPathname().find("Text texture");
    if(String::npos != textTexturePos)
    {
        return true; //is text texture
    }
    
    return false;
}



String SceneValidator::SetPathForChecking(const String &pathname)
{
    String oldPath = pathForChecking;
    pathForChecking = pathname;
    return oldPath;
}


bool SceneValidator::ValidateTexturePathname(const String &pathForValidation, Set<String> &errorsLog)
{
	DVASSERT(!pathForChecking.empty() && "Need to set pathname for DataSource folder");

	bool pathIsCorrect = IsPathCorrectForProject(pathForValidation);
	if(pathIsCorrect)
	{
		String textureExtension = FileSystem::Instance()->GetExtension(pathForValidation);
		String::size_type extPosition = TextureDescriptor::GetSupportedTextureExtensions().find(textureExtension);
		if(String::npos == extPosition)
		{
			errorsLog.insert(Format("Path %s has incorrect extension", pathForValidation.c_str()));
			return false;
		}

		CreateDescriptorIfNeed(pathForValidation);
	}
	else
	{
		errorsLog.insert(Format("Path %s is incorrect for project %s", pathForValidation.c_str(), pathForChecking.c_str()));
	}

	return pathIsCorrect;
}

bool SceneValidator::ValidateHeightmapPathname(const String &pathForValidation, Set<String> &errorsLog)
{
	DVASSERT(!pathForChecking.empty() && "Need to set pathname for DataSource folder");

	bool pathIsCorrect = IsPathCorrectForProject(pathForValidation);
	if(pathIsCorrect)
	{
		String::size_type posPng = pathForValidation.find(".png");
		String::size_type posHeightmap = pathForValidation.find(Heightmap::FileExtension());
        
        pathIsCorrect = ((String::npos != posPng) || (String::npos != posHeightmap));
        if(!pathIsCorrect)
        {
            errorsLog.insert(Format("Heightmap path %s is wrong", pathForValidation.c_str()));
            return false;
        }
        
        Heightmap *heightmap = new Heightmap();
        if(String::npos != posPng)
        {
            Image *image = CreateTopLevelImage(pathForValidation);
            pathIsCorrect = heightmap->BuildFromImage(image);
            SafeRelease(image);
        }
        else
        {
            pathIsCorrect = heightmap->Load(pathForValidation);
        }

        
        if(!pathIsCorrect)
        {
            SafeRelease(heightmap);
            errorsLog.insert(Format("Can't load Heightmap from path %s", pathForValidation.c_str()));
            return false;
        }
        
        
        pathIsCorrect = IsPowerOf2(heightmap->Size() - 1);
        if(!pathIsCorrect)
        {
            errorsLog.insert(Format("Heightmap %s has wrong size", pathForValidation.c_str()));
        }
        
        SafeRelease(heightmap);
		return pathIsCorrect;
	}
	else
	{
		errorsLog.insert(Format("Path %s is incorrect for project %s", pathForValidation.c_str(), pathForChecking.c_str()));
	}

	return pathIsCorrect;
}


void SceneValidator::CreateDescriptorIfNeed(const String &forPathname)
{
    String descriptorPathname = TextureDescriptor::GetDescriptorPathname(forPathname);
    if(! FileSystem::Instance()->IsFile(descriptorPathname))
    {
		Logger::Warning("[SceneValidator::CreateDescriptorIfNeed] Need descriptor for file %s", forPathname.c_str());
        
		TextureDescriptor *descriptor = new TextureDescriptor();
		descriptor->textureFileFormat = PNG_FILE;
        
        String descriptorPathname = TextureDescriptor::GetDescriptorPathname(forPathname);
		descriptor->Save(descriptorPathname);

        SafeRelease(descriptor);
    }
}


bool SceneValidator::ValidatePathname(const String &pathForValidation, const String &validatedObjectName)
{
    DVASSERT(0 < pathForChecking.length()); 
    //Need to set path to DataSource/3d for path correction  
    //Use SetPathForChecking();
    
    String pathname = FileSystem::GetCanonicalPath(pathForValidation);
    
    String::size_type fboFound = pathname.find(String("FBO"));
    String::size_type resFound = pathname.find(String("~res:"));
    if((String::npos != fboFound) || (String::npos != resFound))
    {
        return true;   
    }
    
    return IsPathCorrectForProject(pathForValidation);
}

bool SceneValidator::IsPathCorrectForProject(const String &pathname)
{
    String normalizedPath = FileSystem::GetCanonicalPath(pathname);
    String::size_type foundPos = normalizedPath.find(pathForChecking);
    return (String::npos != foundPos);
}


void SceneValidator::EnumerateNodes(DAVA::Scene *scene)
{
    int32 nodesCount = 0;
    if(scene)
    {
        for(int32 i = 0; i < scene->GetChildrenCount(); ++i)
        {
            nodesCount += EnumerateSceneNodes(scene->GetChild(i));
        }
    }
}

int32 SceneValidator::EnumerateSceneNodes(DAVA::Entity *node)
{
    //TODO: lode node can have several nodes at layer
    
    int32 nodesCount = 1;
    for(int32 i = 0; i < node->GetChildrenCount(); ++i)
    {
        nodesCount += EnumerateSceneNodes(node->GetChild(i));
    }
    
    return nodesCount;
}


bool SceneValidator::IsTextureChanged(const String &texturePathname, ImageFileFormat fileFormat)
{
    bool isChanged = false;

	TextureDescriptor *descriptor = TextureDescriptor::CreateFromFile(texturePathname);
    if(descriptor)
    {
        isChanged = descriptor->IsSourceChanged(fileFormat);
        SafeRelease(descriptor);
    }

    return isChanged;
}

bool SceneValidator::IsTextureDescriptorPath(const String &path)
{
	String ext = FileSystem::GetExtension(path);
	return (ext == TextureDescriptor::GetDescriptorExtension());
}



void SceneValidator::CreateDefaultDescriptors(const String &folderPathname)
{
	FileList * fileList = new FileList(folderPathname);
    if(!fileList) return;
    
	for (int32 fi = 0; fi < fileList->GetCount(); ++fi)
	{
		if (fileList->IsDirectory(fi))
		{
            if(0 != CompareCaseInsensitive(String(".svn"), fileList->GetFilename(fi))
               && 0 != CompareCaseInsensitive(String("."), fileList->GetFilename(fi))
                && 0 != CompareCaseInsensitive(String(".."), fileList->GetFilename(fi)))
            {
                CreateDefaultDescriptors(fileList->GetPathname(fi));
            }
		}
        else
        {
            const String pathname = fileList->GetPathname(fi);
            const String extension = FileSystem::Instance()->GetExtension(pathname);
            if(0 == CompareCaseInsensitive(String(".png"), extension))
            {
                CreateDescriptorIfNeed(pathname);
            }
        }
	}

	SafeRelease(fileList);
}





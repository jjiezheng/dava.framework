#include "SceneEditorScreenMain.h"

#include "EditorBodyControl.h"

#include "ControlsFactory.h"
#include "../EditorScene.h"
#include "MaterialEditor.h"

#include "EditorSettings.h"
#include "SceneValidator.h"

#include "TextureTrianglesDialog.h"

#include "PropertyControlCreator.h"

#include "HintManager.h"

#include "SceneExporter.h"
#include "SceneSaver.h"

#include "../Qt/Scene/SceneData.h"
#include "../Qt/Scene/SceneDataManager.h"
#include "../Qt/Main/ScenePreviewDialog.h"
#include "../Qt/Main/QtUtils.h"
#include "FileSystem/FileSystem.h"

#include "../Commands/SceneEditorScreenMainCommands.h"
#include "../Commands/CommandsManager.h"
#include "../Commands/FileCommands.h"
#include "../Commands/ToolsCommands.h"

#include "CommandLineTool.h"

SceneEditorScreenMain::SceneEditorScreenMain()
	:	UIScreen()
{
}

void SceneEditorScreenMain::LoadResources()
{
    new HintManager();
    new PropertyControlCreator();
    
    ControlsFactory::CustomizeScreenBack(this);

    font12 = ControlsFactory::GetFont12();
	font12Color = ControlsFactory::GetColorLight();

    focusedControl = NULL;

    InitializeNodeDialogs();

    Rect fullRect = GetRect();
    settingsDialog = new SettingsDialog(fullRect, this);
    textureTrianglesDialog = new TextureTrianglesDialog();
    materialEditor = new MaterialEditor();
    
    InitControls();
    
    InitializeBodyList();
    SetupAnimation();
}



void SceneEditorScreenMain::InitControls()
{
    // add line after menu
    Rect fullRect = GetRect();
    AddLineControl(Rect(0, ControlsFactory::BUTTON_HEIGHT, fullRect.dx, LINE_HEIGHT));
    
    scenePreviewDialog = new ScenePreviewDialog();
}

void SceneEditorScreenMain::UnloadResources()
{
    SafeRelease(scenePreviewDialog);

    SafeRelease(textureTrianglesDialog);
    SafeRelease(settingsDialog);

    ReleaseNodeDialogs();
    ReleaseBodyList();

    HintManager::Instance()->Release();
    PropertyControlCreator::Instance()->Release();
}


void SceneEditorScreenMain::WillAppear()
{
#if defined (__DAVAENGINE_WIN32__)
	Vector<String> & commandLine = Core::Instance()->GetCommandLine();
	if(CommandLineTool::Instance()->CommandIsFound(String("-beast")))
	{
		int32 inPosition = CommandLineTool::Instance()->CommandPosition(String("-file"));
		if(CommandLineTool::Instance()->CheckPosition(inPosition))
		{
			String filepathname = commandLine[inPosition + 1];
			printf("[Beast] file %s", filepathname.c_str());

			CommandsManager::Instance()->ExecuteAndRelease(new CommandOpenScene(filepathname));
		}
		else
		{
			printf("[Beast] Error: wrong command line.\n");
		}
	}
#endif //#if defined (__DAVAENGINE_WIN32__)
}

void SceneEditorScreenMain::DidAppear()
{
#if defined (__DAVAENGINE_WIN32__)
	if(CommandLineTool::Instance()->CommandIsFound(String("-beast")))
	{
		Update(0.1f);
		CommandsManager::Instance()->ExecuteAndRelease(new CommandBeast());
	}
#endif //#if defined (__DAVAENGINE_WIN32__)
}

void SceneEditorScreenMain::WillDisappear()
{
}

void SceneEditorScreenMain::Update(float32 timeElapsed)
{
    focusedControl = UIControlSystem::Instance()->GetFocusedControl();
    UIScreen::Update(timeElapsed);
}

void SceneEditorScreenMain::Draw(const UIGeometricData &geometricData)
{
    UIScreen::Draw(geometricData);
}

void SceneEditorScreenMain::AddLineControl(DAVA::Rect r)
{
    UIControl *lineControl = ControlsFactory::CreateLine(r);
    AddControl(lineControl);
    SafeRelease(lineControl);
}



void SceneEditorScreenMain::InitializeBodyList()
{
    AddBodyItem(LocalizedString(L"panel.level"), false);
}

void SceneEditorScreenMain::ReleaseBodyList()
{
    for(Vector<BodyItem*>::iterator it = bodies.begin(); it != bodies.end(); ++it)
    {
        BodyItem *iBody = *it;

        RemoveControl(iBody->headerButton);
        RemoveControl(iBody->bodyControl);
        
        SafeRelease(iBody->headerButton);
        SafeRelease(iBody->closeButton);
        SafeRelease(iBody->bodyControl);

        SafeDelete(iBody);
    }
    bodies.clear();
}

void SceneEditorScreenMain::AddBodyItem(const WideString &text, bool isCloseable)
{
    HideScenePreview();
    
    EditorScene *scene = SceneDataManager::Instance()->RegisterNewScene();
    SceneDataManager::Instance()->SetActiveScene(scene);
    
    BodyItem *c = new BodyItem();
    
    int32 count = bodies.size();
    
    c->headerButton = ControlsFactory::CreateButton(
                                                    Vector2(0 + count * (ControlsFactory::BUTTON_WIDTH + 1), 0),
                                                    text);
    Rect fullRect = GetRect();
    c->bodyControl = new EditorBodyControl(Rect(0, ControlsFactory::BUTTON_HEIGHT + 1, fullRect.dx, fullRect.dy - ControlsFactory::BUTTON_HEIGHT - 1));
    
    
    c->headerButton->SetTag(count);
    c->headerButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &SceneEditorScreenMain::OnSelectBody));
    if(isCloseable)
    {
        c->closeButton = ControlsFactory::CreateCloseWindowButton(
                                                                  Rect(ControlsFactory::BUTTON_WIDTH - ControlsFactory::BUTTON_HEIGHT, 0,
                                                                       ControlsFactory::BUTTON_HEIGHT, ControlsFactory::BUTTON_HEIGHT));
        c->closeButton->SetTag(count);
        c->closeButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &SceneEditorScreenMain::OnCloseBody));
        c->headerButton->AddControl(c->closeButton);
    }
    else 
    {
        c->closeButton = NULL;
    }

    
    SceneData *sceneData = SceneDataManager::Instance()->SceneGetActive();
    c->bodyControl->SetScene(sceneData->GetScene());
    c->bodyControl->SetCameraController(sceneData->GetCameraController());
    c->bodyControl->SetTag(count);
    
    AddControl(c->headerButton);    
    bodies.push_back(c);
    
    //set as current
    c->headerButton->PerformEvent(UIControl::EVENT_TOUCH_UP_INSIDE);
}

void SceneEditorScreenMain::ActivateLevelBodyItem()
{
	// "Level" body item is always first.
	static const int32 LEVEL_BODY_ITEM_INDEX = 0;
	if (bodies.empty() || !bodies[LEVEL_BODY_ITEM_INDEX]->headerButton)
	{
		return;
	}
	
	OnSelectBody(bodies[LEVEL_BODY_ITEM_INDEX]->headerButton, NULL, NULL);
}

void SceneEditorScreenMain::OnSelectBody(BaseObject * owner, void *, void *)
{
    UIButton *btn = (UIButton *)owner;
    
    for(int32 i = 0; i < (int32)bodies.size(); ++i)
    {
        if(bodies[i]->bodyControl->GetParent())
        {
            if(btn == bodies[i]->headerButton)
            {
                // click on selected body - nothing to do
                return;
            }
            
            RemoveControl(bodies[i]->bodyControl);
            bodies[i]->headerButton->SetSelected(false, false);
        }
    }

	ActivateBodyItem(bodies[btn->GetTag()], false);
}

void SceneEditorScreenMain::OnCloseBody(BaseObject * owner, void *, void *)
{
    UIButton *btn = (UIButton *)owner;
    int32 tag = btn->GetTag();
    
    Vector<BodyItem*>::iterator it = bodies.begin();
    for(int32 i = 0; i < (int32)bodies.size(); ++i, ++it)
    {
        if(btn == bodies[i]->closeButton)
        {
            if(bodies[i]->bodyControl->GetParent())
            {
                RemoveControl(bodies[i]->bodyControl);
            }
            RemoveControl(bodies[i]->headerButton);
            
            SceneDataManager::Instance()->ReleaseScene(bodies[i]->bodyControl->GetScene());

            SafeRelease(bodies[i]->headerButton);
            SafeRelease(bodies[i]->closeButton);
            SafeRelease(bodies[i]->bodyControl);
            
            SafeDelete(*it);

            bodies.erase(it);
            break;
        }
    }

    for(int32 i = 0; i < (int32)bodies.size(); ++i)
    {
        bodies[i]->headerButton->SetRect(Rect(i * (ControlsFactory::BUTTON_WIDTH + 1), 0,
                                                ControlsFactory::BUTTON_WIDTH, ControlsFactory::BUTTON_HEIGHT));
        bodies[i]->headerButton->SetTag(i);
        if(bodies[i]->closeButton)
        {
            bodies[i]->closeButton->SetTag(i);
        }
        bodies[i]->bodyControl->SetTag(i);
    }
    
    if(bodies.size())
    {
        if(tag)
        {
            --tag;
        }

        //set as current
		ActivateBodyItem(bodies[tag], true);
    }
}

SceneEditorScreenMain::BodyItem * SceneEditorScreenMain::FindCurrentBody()
{
    for(int32 i = 0; i < (int32)bodies.size(); ++i)
    {
        if(bodies[i]->bodyControl->GetParent())
        {
            return bodies[i];
        }
    }
    
    return NULL;
}


void SceneEditorScreenMain::DialogClosed(int32 retCode)
{
    RemoveControl(nodeDialog);
    RemoveControl(dialogBack);
    
    if(CreateNodesDialog::RCODE_OK == retCode)
    {
		CommandsManager::Instance()->ExecuteAndRelease(new CommandCreateNodeSceneEditor(nodeDialog->GetSceneNode()));
    }
}

void SceneEditorScreenMain::InitializeNodeDialogs()
{
    Rect rect = GetRect();
    dialogBack = ControlsFactory::CreatePanelControl(rect);
    ControlsFactory::CustomizeDialogFreeSpace(dialogBack);
    
    Rect r;
    r.dx = rect.dx / 2;
    r.dy = rect.dy / 2;
    
    r.x = rect.x + r.dx / 2;
    r.y = rect.y + r.dy / 2;

    nodeDialog = new CreateNodesDialog(r);
    nodeDialog->SetDelegate(this);
}

void SceneEditorScreenMain::ReleaseNodeDialogs()
{
    SafeRelease(nodeDialog);
    SafeRelease(dialogBack);
}

void SceneEditorScreenMain::EditMaterial(Material *material)
{
    BodyItem *iBody = FindCurrentBody();
    if (!materialEditor->GetParent())
    {
        materialEditor->EditMaterial(iBody->bodyControl->GetScene(), material);
        
        AddControl(materialEditor);
    }
}


void SceneEditorScreenMain::AutoSaveLevel(BaseObject *, void *, void *)
{
    time_t now = time(0);
    tm* utcTime = localtime(&now);
    
    String folderPath = EditorSettings::Instance()->GetDataSourcePath() + "Autosave";
    bool folderExcists = FileSystem::Instance()->IsDirectory(folderPath);
    if(!folderExcists)
    {
        FileSystem::Instance()->CreateDirectory(folderPath);
    }

    
    
    String pathToFile = folderPath + Format("/AutoSave_%04d.%02d.%02d_%02d_%02d.sc2",   
                                            utcTime->tm_year + 1900, utcTime->tm_mon + 1, utcTime->tm_mday, 
                                            utcTime->tm_hour, utcTime->tm_min);
    
    BodyItem *iBody = bodies[0];
    Scene * scene = iBody->bodyControl->GetScene();
    SceneFileV2 * file = new SceneFileV2();
    file->EnableDebugLog(false);
    file->SaveScene(pathToFile, scene);
    SafeRelease(file);
    
    SetupAnimation();
}

void SceneEditorScreenMain::SetupAnimation()
{
    float32 minutes = EditorSettings::Instance()->GetAutosaveTime();
    Animation * anim = WaitAnimation(minutes * 60.f); 
    anim->AddEvent(Animation::EVENT_ANIMATION_END, Message(this, &SceneEditorScreenMain::AutoSaveLevel));
}



void SceneEditorScreenMain::SettingsChanged()
{
    for(int32 i = 0; i < (int32)bodies.size(); ++i)
    {
        EditorScene *scene = bodies[i]->bodyControl->GetScene();
        scene->SetDrawGrid(EditorSettings::Instance()->GetDrawGrid());
    }
}

void SceneEditorScreenMain::Input(DAVA::UIEvent *event)
{
    if(UIEvent::PHASE_KEYCHAR == event->phase)
    {
        if(IsKeyModificatorPressed(DVKEY_ALT))
        {
            int32 key = event->tid - DVKEY_1;
            if(0 <= key && key < 8)
            {
                BodyItem *iBody = FindCurrentBody();
                Entity *node = iBody->bodyControl->GetSelectedSGNode();
                EditorScene *editorScene = iBody->bodyControl->GetScene();
                editorScene->SetForceLodLayer(node, key);
            }
            else if(DVKEY_0 == event->tid)
            {
                EditorSettings::Instance()->Save();
            }
        }
        
        
        //ckecking help
        UITextField *tf = dynamic_cast<UITextField *>(UIControlSystem::Instance()->GetFocusedControl());
        UITextField *tf1 = dynamic_cast<UITextField *>(focusedControl);
        if(!tf && !tf1)
        {
            if(DVKEY_ESCAPE == event->tid)
            {
                if(materialEditor && materialEditor->GetParent())
                {
                    MaterialsTriggered();
                }
            }

        }
    }
}

void SceneEditorScreenMain::OpenFileAtScene(const String &pathToFile)
{
	// In case the current scene isn't the "level" one, switch to it firstly.
	if (SceneDataManager::Instance()->SceneGetActive() != SceneDataManager::Instance()->SceneGetLevel())
	{
		ActivateLevelBodyItem();
	}

    //опен всегда загружает только уровень, но не отдельные части сцены
    SceneDataManager::Instance()->EditLevelScene(pathToFile);
}

void SceneEditorScreenMain::ShowTextureTriangles(PolygonGroup *polygonGroup)
{
    ReleaseResizedControl(textureTrianglesDialog);
    textureTrianglesDialog = new TextureTrianglesDialog();
    
    if(textureTrianglesDialog)
    {
        textureTrianglesDialog->Show(polygonGroup);
    }
}

void SceneEditorScreenMain::RecreteFullTilingTexture()
{
    for(int32 i = 0; i < (int32)bodies.size(); ++i)
    {
        bodies[i]->bodyControl->RecreteFullTilingTexture();
    }
}

void SceneEditorScreenMain::NewScene()
{
	SceneData *levelScene = SceneDataManager::Instance()->CreateNewScene();
    
    bodies[0]->bodyControl->SetScene(levelScene->GetScene());
    bodies[0]->bodyControl->Refresh();
}


bool SceneEditorScreenMain::SaveIsAvailable()
{
    if(FindCurrentBody()->bodyControl->LandscapeEditorActive())
    {
        ShowErrorDialog(String("Can't save level at Landscape Editor Mode."));
        return false;
    }

    return true;
}

String SceneEditorScreenMain::CurrentScenePathname()
{
    SceneData *sceneData = SceneDataManager::Instance()->SceneGetActive();
    String pathname = sceneData->GetScenePathname();
    if (0 < pathname.length())
    {
        pathname = FileSystem::Instance()->ReplaceExtension(pathname, ".sc2");
    }

    return pathname;
}


void SceneEditorScreenMain::SaveSceneToFile(const String &pathToFile)
{
    SceneData *sceneData = SceneDataManager::Instance()->SceneGetActive();
    sceneData->SetScenePathname(pathToFile);

    BodyItem *iBody = FindCurrentBody();
    iBody->bodyControl->PushDebugCamera();
    
    Scene * scene = iBody->bodyControl->GetScene();
    
    uint64 startTime = SystemTimer::Instance()->AbsoluteMS();
    SceneFileV2 * file = new SceneFileV2();
    file->EnableDebugLog(false);
    file->SaveScene(pathToFile, scene);
    SafeRelease(file);
    uint64 endTime = SystemTimer::Instance()->AbsoluteMS();
    Logger::Info("[SAVE SCENE TIME] %d ms", (endTime - startTime));
    
    iBody->bodyControl->PopDebugCamera();			
}

void SceneEditorScreenMain::UpdateModificationPanel(void)
{
	for (int32 i = 0; i < (int32)bodies.size(); i++)
	{
		bodies[i]->bodyControl->UpdateModificationPanel();
	}
}

void SceneEditorScreenMain::SaveToFolder(const String & folder)
{
    BodyItem *iBody = FindCurrentBody();
	iBody->bodyControl->PushDebugCamera();
    
    SceneData *sceneData = SceneDataManager::Instance()->SceneGetActive();
    String filePath = sceneData->GetScenePathname();
    String dataSourcePath = EditorSettings::Instance()->GetDataSourcePath();
    String::size_type pos = filePath.find(dataSourcePath);
    if(String::npos != pos)
    {
        filePath = filePath.replace(pos, dataSourcePath.length(), "");
    }
    else
    {
        DVASSERT(0);
    }
    
	// Get project path
    KeyedArchive *keyedArchieve = EditorSettings::Instance()->GetSettings();
    String projectPath = keyedArchieve->GetString(String("ProjectPath"));
    
    if(!SceneSaver::Instance()) new SceneSaver();
    
    String inFolder = projectPath + String("DataSource/3d/");
    SceneSaver::Instance()->SetInFolder(inFolder);
    SceneSaver::Instance()->SetOutFolder(folder);
    
    Set<String> errorsLog;
    SceneSaver::Instance()->SaveScene(iBody->bodyControl->GetScene(), filePath, errorsLog);
    
	iBody->bodyControl->PopDebugCamera();
    
    ShowErrorDialog(errorsLog);
}

void SceneEditorScreenMain::ExportAs(ImageFileFormat format)
{
    String formatStr;
    switch (format) 
    {
        case DAVA::PNG_FILE:
            formatStr = String("png");
            break;
            
        case DAVA::PVR_FILE:
            formatStr = String("pvr");
            break;
            
        case DAVA::DXT_FILE:
            formatStr = String("dds");
            break;
            
        default:
			DVASSERT(0);
            return;
    }
    
    
    BodyItem *iBody = FindCurrentBody();
	iBody->bodyControl->PushDebugCamera();
    
    SceneData *sceneData = SceneDataManager::Instance()->SceneGetActive();
    String filePath = sceneData->GetScenePathname();
    String dataSourcePath = EditorSettings::Instance()->GetDataSourcePath();
    String::size_type pos = filePath.find(dataSourcePath);
    if(String::npos != pos)
    {
        filePath = filePath.replace(pos, dataSourcePath.length(), "");
    }
    else 
    {
        DVASSERT(0);
    }
    
    // Get project path
    KeyedArchive *keyedArchieve = EditorSettings::Instance()->GetSettings();
    String projectPath = keyedArchieve->GetString(String("ProjectPath"));
    
    if(!SceneExporter::Instance()) new SceneExporter();
    
    String inFolder = projectPath + String("DataSource/3d/");
    SceneExporter::Instance()->SetInFolder(inFolder);
    SceneExporter::Instance()->SetOutFolder(projectPath + String("Data/3d/"));
    
    SceneExporter::Instance()->SetExportingFormat(formatStr);
    
    //TODO: how to be with removed nodes?
    Set<String> errorsLog;
    SceneExporter::Instance()->ExportScene(iBody->bodyControl->GetScene(), filePath, errorsLog);
    
	iBody->bodyControl->PopDebugCamera();
    
    ShowErrorDialog(errorsLog);
}


void SceneEditorScreenMain::CreateNode(ResourceEditor::eNodeType nodeType)
{
	Rect rect = GetRect();

	if(dialogBack->GetSize() != rect.GetSize())
	{
		ReleaseResizedControl(dialogBack);
		ReleaseResizedControl(nodeDialog);


		dialogBack = ControlsFactory::CreatePanelControl(rect);
		ControlsFactory::CustomizeDialogFreeSpace(dialogBack);

		Rect r;
		r.dx = rect.dx / 2;
		r.dy = rect.dy / 2;

		r.x = rect.x + r.dx / 2;
		r.y = rect.y + r.dy / 2;

		nodeDialog = new CreateNodesDialog(r);
		nodeDialog->SetDelegate(this);
	}

    nodeDialog->CreateNode(nodeType);
    
    AddControl(dialogBack);
    AddControl(nodeDialog);
}


void SceneEditorScreenMain::MaterialsTriggered()
{
    BodyItem *iBody = FindCurrentBody();
    if (!materialEditor->GetParent())
    {
        ReleaseResizedControl(materialEditor);
        materialEditor = new MaterialEditor();
        
        materialEditor->SetWorkingScene(iBody->bodyControl->GetScene(), iBody->bodyControl->GetSelectedSGNode());
        
        AddControl(materialEditor);
    }
    else 
    {
        RemoveControl(materialEditor);
        SceneValidator::Instance()->EnumerateSceneTextures();
    }
}

void SceneEditorScreenMain::HeightmapTriggered()
{
    BodyItem *iBody = FindCurrentBody();
    bool ret = iBody->bodyControl->ToggleLandscapeEditor(ELEMID_HEIGHTMAP);
}

void SceneEditorScreenMain::TilemapTriggered()
{
    BodyItem *iBody = FindCurrentBody();
    bool ret = iBody->bodyControl->ToggleLandscapeEditor(ELEMID_COLOR_MAP);
}

void SceneEditorScreenMain::CustomColorsTriggered()
{
    BodyItem *iBody = FindCurrentBody();
    bool ret = iBody->bodyControl->ToggleLandscapeEditor(ELEMID_CUSTOM_COLORS);	
}

void SceneEditorScreenMain::CustomColorsSetRadius(uint32 newRadius)
{
	BodyItem *iBody = FindCurrentBody();
    iBody->bodyControl->SetBrushRadius(newRadius);
}

void SceneEditorScreenMain::CustomColorsSetColor(uint32 indexInSet)
{
	BodyItem *iBody = FindCurrentBody();
    iBody->bodyControl->SetColorIndex(indexInSet);
}

void SceneEditorScreenMain::CustomColorsSaveTexture(const String &path)
{
	BodyItem *iBody = FindCurrentBody();
    iBody->bodyControl->SaveTexture(path);
}

void SceneEditorScreenMain::CustomColorsLoadTexture(const String &path)
{
	BodyItem *iBody = FindCurrentBody();
    iBody->bodyControl->CustomColorsLoadTexture(path);
}

String SceneEditorScreenMain::CustomColorsGetCurrentSaveFileName()
{
	BodyItem *iBody = FindCurrentBody();
	return iBody->bodyControl->CustomColorsGetCurrentSaveFileName();
}

void SceneEditorScreenMain::SelectNodeQt(DAVA::Entity *node)
{
    BodyItem *iBody = FindCurrentBody();
	if (iBody)
	    iBody->bodyControl->SelectNodeQt(node);
}

void SceneEditorScreenMain::OnReloadRootNodesQt()
{
    BodyItem *iBody = FindCurrentBody();
    iBody->bodyControl->OnReloadRootNodesQt();
}

void SceneEditorScreenMain::ShowScenePreview(const String scenePathname)
{
    if(scenePreviewDialog)
    {
        scenePreviewDialog->Show(scenePathname);
    }
}

void SceneEditorScreenMain::HideScenePreview()
{
    if(scenePreviewDialog)
    {
        scenePreviewDialog->Close();
    }
}

bool SceneEditorScreenMain::LandscapeEditorModeEnabled()
{
    BodyItem *iBody = FindCurrentBody();
    return iBody->bodyControl->LandscapeEditorActive();
}

bool SceneEditorScreenMain::TileMaskEditorEnabled()
{
    BodyItem *iBody = FindCurrentBody();
    return iBody->bodyControl->TileMaskEditorEnabled();
}


void SceneEditorScreenMain::ShowSettings()
{
    if(!settingsDialog->GetParent())
    {
        AddControl(settingsDialog);
    }
}

void SceneEditorScreenMain::SetViewport(ResourceEditor::eViewportType viewportType)
{
    BodyItem *iBody = FindCurrentBody();
    iBody->bodyControl->SetViewportSize(viewportType);
}

void SceneEditorScreenMain::ProcessBeast()
{
#ifdef __DAVAENGINE_BEAST__
	bodies[0]->bodyControl->BeastProcessScene();
#endif //#ifdef __DAVAENGINE_BEAST__
}

void SceneEditorScreenMain::SetSize(const Vector2 &newSize)
{
    UIScreen::SetSize(newSize);
    
    Vector2 bodySize(newSize.x, newSize.y - ControlsFactory::BUTTON_HEIGHT - 1);
    for(int32 i = 0; i < (int32)bodies.size(); ++i)
    {
        bodies[i]->bodyControl->SetSize(bodySize);
    }
}

void SceneEditorScreenMain::ReleaseResizedControl(UIControl *control)
{
    if(control && control->GetParent())
    {
        control->GetParent()->RemoveControl(control);
    }
    
    SafeRelease(control);
}

void SceneEditorScreenMain::RulerToolTriggered()
{
    BodyItem *iBody = FindCurrentBody();
    iBody->bodyControl->RulerToolTriggered();
}

void SceneEditorScreenMain::VisibilityToolTriggered()
{
    BodyItem *iBody = FindCurrentBody();
    bool ret = iBody->bodyControl->ToggleLandscapeEditor(ELEMID_VISIBILITY_CHECK_TOOL);
}

void SceneEditorScreenMain::VisibilityToolSaveTexture(const String &path)
{
	BodyItem *iBody = FindCurrentBody();
    iBody->bodyControl->SaveTexture(path);
}

void SceneEditorScreenMain::VisibilityToolSetPoint()
{
	BodyItem *iBody = FindCurrentBody();
	iBody->bodyControl->VisibilityToolSetPoint();
}

void SceneEditorScreenMain::VisibilityToolSetArea()
{
	BodyItem *iBody = FindCurrentBody();
    iBody->bodyControl->VisibilityToolSetArea();
}

void SceneEditorScreenMain::VisibilityToolSetAreaSize(uint32 size)
{
	BodyItem *iBody = FindCurrentBody();
    iBody->bodyControl->VisibilityToolSetAreaSize(size);
}

void SceneEditorScreenMain::ProcessIsSolidChanging()
{
	BodyItem *iBody = FindCurrentBody();
    iBody->bodyControl->ProcessIsSolidChanging();
}

void SceneEditorScreenMain::ActivateBodyItem(BodyItem* activeItem, bool forceResetSelection)
{
	if (!activeItem)
	{
		return;
	}
	
	AddControl(activeItem->bodyControl);
	activeItem->headerButton->SetSelected(true, true);

	if (forceResetSelection)
	{
		for(int32 i = 0; i < (int32)bodies.size(); ++i)
		{
			if (bodies[i] == activeItem)
			{
				continue;
			}

			RemoveControl(bodies[i]->bodyControl);
			bodies[i]->headerButton->SetSelected(false, false);
		}
	}

	SceneDataManager::Instance()->SetActiveScene(activeItem->bodyControl->GetScene());
}

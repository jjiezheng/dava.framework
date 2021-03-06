#include "LandscapeToolsSelection.h"

#include "ControlsFactory.h"
#include "LandscapeTool.h"

#include "EditorSettings.h"

LandscapeToolsSelection::LandscapeToolsSelection(LandscapeToolsSelectionDelegate *newDelegate, const Rect & rect)
    :   UIControl(rect)
    ,   parentBodyControl(NULL)
    ,   selectedTool(NULL)
    ,   delegate(newDelegate)
{
    ControlsFactory::CustomizeDialog(this);
    
    Rect closeRect(rect.dx - ControlsFactory::BUTTON_HEIGHT, 0, ControlsFactory::BUTTON_HEIGHT, ControlsFactory::BUTTON_HEIGHT);
    closeButton = ControlsFactory::CreateCloseWindowButton(closeRect);
    closeButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &LandscapeToolsSelection::OnClose));
    AddControl(closeButton);
    
    toolsList = new UIList(Rect(0, 0, rect.dx - ControlsFactory::BUTTON_HEIGHT, rect.dy), UIList::ORIENTATION_VERTICAL);
    toolsList->SetDelegate(this);
    ControlsFactory::SetScrollbar(toolsList);
    AddControl(toolsList);
    
    EnumerateTools();
}


LandscapeToolsSelection::~LandscapeToolsSelection()
{
    SafeRelease(closeButton);
    SafeRelease(toolsList);
    
    ReleaseTools();
    
    selectedTool = NULL;
}


LandscapeTool * LandscapeToolsSelection::Tool()
{
    return selectedTool;
}

void LandscapeToolsSelection::OnClose(DAVA::BaseObject *, void *, void *)
{
    Close();
}

void LandscapeToolsSelection::Close()
{
    if(GetParent())
    {
        GetParent()->RemoveControl(this);
    }
}

void LandscapeToolsSelection::Show()
{
    if(!GetParent() && parentBodyControl)
    {
        parentBodyControl->AddControl(this);
    }
}

void LandscapeToolsSelection::SetBodyControl(DAVA::UIControl *parent)
{
    parentBodyControl = parent;
}

void LandscapeToolsSelection::OnToolSelected(BaseObject * , void * userData, void * )
{
    if(delegate)
    {
        selectedTool = (LandscapeTool *)userData;
        delegate->OnToolSelected(this, selectedTool);
    }
}

void LandscapeToolsSelection::WillAppear()
{
    UpdateSize();

    if(!selectedTool)
    {
        selectedTool = (tools.size()) ? tools[0] : NULL;
    }
    if(delegate)
    {
        delegate->OnToolSelected(this, selectedTool);
    }
        
    toolsList->Refresh();
    
    UIControl::WillAppear();
}

void LandscapeToolsSelection::EnumerateTools()
{
    ReleaseTools();
    
    String toolsPath = "~res:/LandscapeEditor/Tools/";
    
    FileList *fileList = new FileList(toolsPath);
    int32 toolID = 0;
    for(int32 iFile = 0; iFile < fileList->GetCount(); ++iFile)
	{
        String filename = fileList->GetFilename(iFile);
        String extension = FileSystem::Instance()->GetExtension(filename);
        
        if(".png" == extension)
        {
            LandscapeTool *tool = new LandscapeTool(toolID, LandscapeTool::TOOL_BRUSH, toolsPath + filename);
            ++toolID;
            
            tools.push_back(tool);
        }
	}
    SafeRelease(fileList);
}

void LandscapeToolsSelection::ReleaseTools()
{
    for(int32 iTool = 0; iTool < (int32)tools.size(); ++iTool)
    {
        SafeRelease(tools[iTool]);
    }
    
    tools.clear();
}

void LandscapeToolsSelection::SetDelegate(LandscapeToolsSelectionDelegate *newDelegate)
{
    delegate = newDelegate;
}



int32 LandscapeToolsSelection::ElementsCount(UIList * list)
{
    int32 countInRow = (int32)(list->GetSize().dx - ControlsFactory::BUTTON_HEIGHT) / ControlsFactory::TOOLS_HEIGHT;
    
    int32 rows = tools.size() / countInRow;
    if(tools.size() % countInRow)
    {
        ++rows;
    }
    
    return rows;
}

UIListCell *LandscapeToolsSelection::CellAtIndex(UIList *list, int32 index)
{
    UIListCell *cell = list->GetReusableCell("ToolSelection cell"); //try to get cell from the reusable cells store
    if(!cell)
    { 
        cell = new UIListCell(Rect(0, 0, (float32)list->size.x, ControlsFactory::TOOLS_HEIGHT), "ToolSelection cell");
        cell->SetInputEnabled(false, false);
    }
    
    int32 countInRow = (int32)(list->GetSize().dx - ControlsFactory::BUTTON_HEIGHT) / ControlsFactory::TOOLS_HEIGHT;
    int32 toolIndex = countInRow * index;
    for(int32 i = 0; i < countInRow; ++i, ++toolIndex)
    {
        UIControl *toolControl = GetToolControl(i, cell);
        toolControl->RemoveAllEvents();
        
        if(toolIndex < (int32)tools.size())
        {
            toolControl->SetSprite(tools[toolIndex]->sprite, 0);
            toolControl->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, 
                                  Message(this, &LandscapeToolsSelection::OnToolSelected, tools[toolIndex]));
            toolControl->SetVisible(this->GetVisible());
        }
        else 
        {
            toolControl->SetVisible(this->GetVisible());
        }
    }
    
    return cell;
}

UIControl * LandscapeToolsSelection::GetToolControl(int32 indexAtRow, DAVA::UIListCell *cell)
{
    String controlName = Format("toolcontrol_%d", indexAtRow);
    
    UIControl *c = cell->FindByName(controlName);
    if(!c)
    {
        c = new UIControl(Rect((float32)(indexAtRow * ControlsFactory::TOOLS_HEIGHT), 0.f, 
                               (float32)ControlsFactory::TOOLS_HEIGHT, (float32)ControlsFactory::TOOLS_HEIGHT));
        c->GetBackground()->SetDrawType(UIControlBackground::DRAW_SCALE_PROPORTIONAL);
        
        c->SetName(controlName);
        cell->AddControl(c);
    }
    
    return c;
}

int32 LandscapeToolsSelection::CellHeight(UIList *, int32)
{
    return ControlsFactory::TOOLS_HEIGHT;
}

void LandscapeToolsSelection::UpdateSize()
{
    if(parentBodyControl)
    {
        Rect parentRect = parentBodyControl->GetRect();

        Rect controlRect(0, parentRect.dy - ControlsFactory::OUTPUT_PANEL_HEIGHT,
                         parentRect.dx - EditorSettings::Instance()->GetRightPanelWidth(), ControlsFactory::OUTPUT_PANEL_HEIGHT);
        
        this->SetRect(controlRect);
        
        closeButton->SetPosition(Vector2(controlRect.dx - ControlsFactory::BUTTON_HEIGHT, 0));
        
        if(toolsList && toolsList->GetParent())
        {
            toolsList->GetParent()->RemoveControl(toolsList);
        }
        SafeRelease(toolsList);
        
        toolsList = new UIList(Rect(0, 0, controlRect.dx - ControlsFactory::BUTTON_HEIGHT, controlRect.dy), UIList::ORIENTATION_VERTICAL);
        toolsList->SetDelegate(this);
        ControlsFactory::SetScrollbar(toolsList);
        AddControl(toolsList);
    }
    
}

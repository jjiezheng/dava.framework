#ifndef __RESOURCE_EDITOR_CUSTOM_COLOR_COMMANDS_H__
#define __RESOURCE_EDITOR_CUSTOM_COLOR_COMMANDS_H__

#include "Command.h"
#include "DAVAEngine.h"
#include "../Constants.h"

using namespace DAVA;

class CommandToggleCustomColors: public Command
{
public:
    CommandToggleCustomColors();

protected:
    virtual void Execute();
};

class CommandSaveTextureCustomColors: public Command
{
public:
    CommandSaveTextureCustomColors();
protected:
    virtual void Execute();
};

class CommandChangeBrushSizeCustomColors: public Command
{
public:
    CommandChangeBrushSizeCustomColors(uint32 newSize);
protected:
    uint32 size;

    virtual void Execute();
};

class CommandChangeColorCustomColors: public Command
{
public:
    CommandChangeColorCustomColors(uint32 newColorIndex);
protected:
    uint32 colorIndex;
    
    virtual void Execute();
};




#endif // #ifndef __RESOURCE_EDITOR_CUSTOM_COLOR_COMMANDS_H__
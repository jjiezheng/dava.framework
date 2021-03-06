#ifndef __RESOURCE_EDITOR_LANDSCAPE_EDITOR_VISIBILITY_CHECK_TOOL_H__
#define __RESOURCE_EDITOR_LANDSCAPE_EDITOR_VISIBILITY_CHECK_TOOL_H__

#include "DAVAEngine.h"
#include "LandscapeEditorBase.h"
#include "LandscapeToolsPanel.h"
#include "LandscapeEditorPropertyControl.h"

using namespace DAVA;

enum eVisibilityCheckToolState
{
	VCT_STATE_NORMAL = 0,
	VCT_STATE_SET_POINT,
	VCT_STATE_SET_AREA,
	
	VCT_STATES_COUNT
};

class EditorHeightmap;
class LandscapeEditorVisibilityCheckTool
    :   public LandscapeEditorBase
    ,   public LandscapeEditorPropertyControlDelegate

{
    
public:
    
    LandscapeEditorVisibilityCheckTool(LandscapeEditorDelegate *newDelegate, EditorBodyControl *parentControl, const Rect &toolsRect);
    virtual ~LandscapeEditorVisibilityCheckTool();
    
	virtual void Draw(const UIGeometricData &geometricData);

    virtual NodesPropertyControl *GetPropertyControl(const Rect &rect);
    
    virtual bool SetScene(EditorScene *newScene);
	virtual void SaveTexture()
	{ Close(); };
    
    //LE property control delegate
    virtual void LandscapeEditorSettingsChanged(LandscapeEditorSettings *settings);
    virtual void TextureWillChanged(const String &forKey);
    virtual void TextureDidChanged(const String &forKey);

	void SetState(eVisibilityCheckToolState newState);
	void SetVisibilityAreaSize(uint32 size);
	void SaveColorLayer(const String &pathName);
	void ClearSceneResources();

	void StorePointState(Vector2* point, bool* pointIsSet, Image** image);
	void RestorePointState(const Vector2& point, bool pointIsSet, Image* image);
	void SetVisibilityPoint(const Vector2& point);

	Image* StoreAreaState();
	void RestoreAreaState(Image* image);
	void SetVisibilityArea(const Vector2& visibilityAreaCenter, uint32 visibilityAreaSize);

	virtual void UpdateLandscapeTilemap(Texture* texture);

protected:
	void CreatePointUndoAction();
	void CreateAreaUndoAction();

    virtual void InputAction(int32 phase, bool intersects);
    virtual void HideAction();
    virtual void ShowAction();
    virtual void SaveTextureAction(const String &pathToFile);
	virtual void UpdateCursor();
	
    virtual void RecreateHeightmapNode();
	
	bool SetPointInputAction(int32 phase);
	bool SetAreaInputAction(int32 phase);

	void PrepareConfig();
	void RecreateVisibilityAreaSprite();
	void DrawVisibilityAreaPoints(const Vector<Vector3>& points);

	Rect2i FitRectToImage(Image* image, const Rect2i& rect);
	void CopyImageRectToImage(Image* imageFrom, const Rect2i& rectFrom, Image* imageTo, const Point2i& pos);

	void PerformHightTest(Vector3 spectatorCoords, Vector2 circleCentr, float cirecleRadius, float density, const Vector<float>& hightValues, Vector<Vector3>* colorizedPoints);
	bool GetIntersectionPoint(const DAVA::Vector2 &touchPoint, DAVA::Vector3 &pointOnLandscape);
	bool CheckIsInCircle(Vector2 circleCentre, float radius, Vector2 targetCoord);

	static Vector2 TranslatePoint(const Vector2& point, const Rect& fromRect, const Rect& toRect);
	float32 GetLandscapeHeightFromCursorPos(const Vector2& point);
	Vector2 ConvertToLanscapeSystem(const Vector2& point);

    void PerformLandscapeDraw();
	
	eVisibilityCheckToolState state;

	float32 pointsDensity;
	float32 visibilityPointHeight;
	Vector<float32> areaPointHeights;
	Vector<Color> areaPointColors;

	bool wasTileMaskToolUpdate;
    
    LandscapeEditorSettings *settings;
	
	bool isVisibilityPointSet;
	Vector2 visibilityPoint;

    bool editingIsEnabled;

	Texture* pointCursorTexture;
	Texture* areaCursorTexture;
	Texture* texSurf;
	Sprite* visibilityAreaSprite;

	uint32		visibilityAreaSize;

	bool		isCursorTransparent;
	bool isFogEnabled;
};


#endif //__RESOURCE_EDITOR_LANDSCAPE_EDITOR_VISIBILITY_CHECK_TOOL_H__
